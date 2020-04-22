#pragma once

#include "IController.h"

//References used:
//https://cs.chromium.org/chromium/src/device/gamepad/dualshock4_controller.cc

enum Dualshock3FeatureValue : uint16_t
{
    Ds3FeatureUnknown1 = 0x0201,
    Ds3FeatureUnknown2 = 0x0301,
    Ds3FeatureDeviceAddress = 0x03F2,
    Ds3FeatureStartDevice = 0x03F4,
    Ds3FeatureHostAddress = 0x03F5,
    Ds3FeatureUnknown3 = 0x03F7,
    Ds3FeatureUnknown4 = 0x03EF,
    Ds3FeatureUnknown5 = 0x03F8,
};

enum Dualshock3InputPacketType : uint8_t
{
    Ds3InputPacket_Button = 0x01,
};

struct Dualshock3ButtonData
{
    //byte0
    uint8_t type;
    //byte1
    uint8_t pad0;

    //byte2
    bool back : 1;
    bool stick_left_click : 1;
    bool stick_right_click : 1;
    bool start : 1;

    bool dpad_up : 1;
    bool dpad_right : 1;
    bool dpad_down : 1;
    bool dpad_left : 1;

    //byte3
    bool trigger_left : 1;
    bool trigger_right : 1;
    bool bumper_left : 1;
    bool bumper_right : 1;

    bool triangle : 1;
    bool circle : 1;
    bool cross : 1;
    bool square : 1;

    //byte4
    bool guide : 1;
    uint8_t pad1 : 7;
    //byte5
    uint8_t pad2;

    //byte6-7
    uint8_t stick_left_x;
    uint8_t stick_left_y;

    //byte8-9
    uint8_t stick_right_x;
    uint8_t stick_right_y;

    //byte10-13
    uint8_t pad3[4];

    //byte14-17 These respond for how hard the corresponding button has been pressed. 0xFF completely, 0x00 not at all
    uint8_t dpad_up_pressure;
    uint8_t dpad_right_pressure;
    uint8_t dpad_down_pressure;
    uint8_t dpad_left_pressure;

    //byte18-19
    uint8_t trigger_left_pressure;
    uint8_t trigger_right_pressure;

    //byte20-21
    uint8_t bumper_left_pressure;
    uint8_t bumper_right_pressure;

    //byte22-25
    uint8_t triangle_pressure;
    uint8_t circle_pressure;
    uint8_t cross_pressure;
    uint8_t square_pressure;

    uint8_t pad4[15];

    //byte41-48
    uint16_t accelerometer_x;
    uint16_t accelerometer_y;
    uint16_t accelerometer_z;
    uint16_t gyroscope;
} __attribute__((packed));

/*
#define DS3_VENDOR_ID                           0x054C
#define DS3_PRODUCT_ID                          0x0268
#define PS_MOVE_NAVI_PRODUCT_ID                 0x042F
*/

enum Dualshock3LEDValue : uint8_t
{
    DS3LED_1 = 0x01,
    DS3LED_2 = 0x02,
    DS3LED_3 = 0x04,
    DS3LED_4 = 0x08,
    DS3LED_5 = 0x09,
    DS3LED_6 = 0x0A,
    DS3LED_7 = 0x0C,
    DS3LED_8 = 0x0D,
    DS3LED_9 = 0x0E,
    DS3LED_10 = 0x0F,
};

#define LED_PERMANENT 0xff, 0x27, 0x00, 0x00, 0x32

class Dualshock3Controller : public IController
{
private:
    IUSBEndpoint *m_inPipe = nullptr;
    IUSBEndpoint *m_outPipe = nullptr;
    IUSBInterface *m_interface = nullptr;

    Dualshock3ButtonData m_buttonData{};

public:
    Dualshock3Controller(std::unique_ptr<IUSBDevice> &&interface);
    virtual ~Dualshock3Controller() override;

    virtual Result Initialize() override;
    virtual void Exit() override;

    Result OpenInterfaces();
    void CloseInterfaces();

    virtual Result GetInput() override;

    virtual NormalizedButtonData GetNormalizedButtonData() override;

    virtual ControllerType GetType() override { return CONTROLLER_DUALSHOCK3; }

    inline const Dualshock3ButtonData &GetButtonData() { return m_buttonData; };

    float NormalizeTrigger(uint8_t deadzonePercent, uint8_t value);
    void NormalizeAxis(uint8_t x, uint8_t y, uint8_t deadzonePercent, float *x_out, float *y_out);

    Result SendInitBytes();
    Result SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude);

    static Result SendCommand(IUSBInterface *interface, Dualshock3FeatureValue feature, const void *buffer, uint16_t size);

    Result SetLED(Dualshock3LEDValue value);

    static void LoadConfig(const ControllerConfig *config);
    virtual ControllerConfig *GetConfig() override;
};