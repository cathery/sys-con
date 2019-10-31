#pragma once

#include "IController.h"

//References used:
//https://cs.chromium.org/chromium/src/device/gamepad/dualshock4_controller.cc

struct Dualshock3ButtonData
{
    uint8_t type;
    uint8_t length;

    bool dpad_up : 1;
    bool dpad_down : 1;
    bool dpad_left : 1;
    bool dpad_right : 1;

    bool start : 1;
    bool back : 1;
    bool stick_left_click : 1;
    bool stick_right_click : 1;

    bool bumper_left : 1;
    bool bumper_right : 1;
    bool guide : 1;
    bool dummy1 : 1; // Always 0.

    bool a : 1;
    bool b : 1;
    bool x : 1;
    bool y : 1;

    uint8_t trigger_left;
    uint8_t trigger_right;

    int16_t stick_left_x;
    int16_t stick_left_y;
    int16_t stick_right_x;
    int16_t stick_right_y;

    // Always 0.
    uint32_t dummy2;
    uint16_t dummy3;
};

/*

typedef enum _DS3_FEATURE_VALUE
{
    Ds3FeatureDeviceAddress = 0x03F2,
    Ds3FeatureStartDevice = 0x03F4,
    Ds3FeatureHostAddress = 0x03F5

} DS3_FEATURE_VALUE;
#define DS3_HID_COMMAND_ENABLE_SIZE             0x04
#define DS3_HID_OUTPUT_REPORT_SIZE              0x30

#define DS3_VENDOR_ID                           0x054C
#define DS3_PRODUCT_ID                          0x0268

#define DS4_HID_OUTPUT_REPORT_SIZE              0x20
#define DS4_VENDOR_ID                           0x054C
#define DS4_PRODUCT_ID                          0x05C4
#define DS4_2_PRODUCT_ID                        0x09CC
#define DS4_WIRELESS_ADAPTER_PRODUCT_ID         0x0BA0

#define PS_MOVE_NAVI_PRODUCT_ID                 0x042F

const uint8_t PS3_REPORT_BUFFER[] = {
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0xff, 0x27, 0x10, 0x00, 0x32,
		0xff, 0x27, 0x10, 0x00, 0x32,
		0xff, 0x27, 0x10, 0x00, 0x32,
		0xff, 0x27, 0x10, 0x00, 0x32,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
//Used to set the LEDs on the controllers
const uint8_t LEDS[] = {
        0x01, // LED1
        0x02, // LED2
        0x04, // LED3
        0x08, // LED4

        0x09, // LED5
        0x0A, // LED6
        0x0C, // LED7
        0x0D, // LED8
        0x0E, // LED9
        0x0F // LED10
};

//Button data
for( int i=0 ; i<3 ; i++ )	HID_PS3_Data.button[i] = data[i+0x2];
for( int i=0 ; i<4 ; i++ )	HID_PS3_Data.analog_stick[i] = data[i+0x6];
for( int i=0 ; i<12 ; i++ )	HID_PS3_Data.button_analog[i] = data[i+0x0e];

printf("Button state 0x%02x 0x%02x 0x%02x \n", data->button[0], data->button[1], data->button[2] );
printf("Analog state 0x%02x 0x%02x 0x%02x 0x%02x \n", data->analog_stick[0], data->analog_stick[1], data->analog_stick[2], data->analog_stick[3] );

printf("Analog state ");
	for( int i=0 ; i<12 ; i++ ) printf("%02x ", data->button_analog[i]);



*/

class Dualshock3Controller : public IController
{
private:
    IUSBEndpoint *m_inPipe = nullptr;
    IUSBEndpoint *m_outPipe = nullptr;

    Dualshock3ButtonData m_buttonData;

    int16_t kLeftThumbDeadzone = 0;  //7849;
    int16_t kRightThumbDeadzone = 0; //8689;
    uint16_t kTriggerMax = 0;        //1023;
    uint16_t kTriggerDeadzone = 0;   //120;

public:
    Dualshock3Controller(std::unique_ptr<IUSBDevice> &&interface);
    virtual ~Dualshock3Controller();

    virtual Status Initialize();
    virtual void Exit();

    Status OpenInterfaces();
    void CloseInterfaces();

    virtual Status GetInput();

    virtual NormalizedButtonData GetNormalizedButtonData();

    virtual ControllerType GetType() { return CONTROLLER_XBOX360; }

    inline const Dualshock3ButtonData &GetButtonData() { return m_buttonData; };

    float NormalizeTrigger(uint16_t value);
    void NormalizeAxis(int16_t x, int16_t y, int16_t deadzone, float *x_out, float *y_out);

    Status SendInitBytes();
    Status SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude);
};