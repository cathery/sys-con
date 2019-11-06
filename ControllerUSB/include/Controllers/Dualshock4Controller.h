#pragma once

#include "IController.h"

//References used:
//https://cs.chromium.org/chromium/src/device/gamepad/dualshock4_controller.cc

struct Dualshock4ButtonData
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

class Dualshock4Controller : public IController
{
private:
    IUSBEndpoint *m_inPipe = nullptr;
    IUSBEndpoint *m_outPipe = nullptr;

    Dualshock4ButtonData m_buttonData;

    /*
    int16_t kLeftThumbDeadzone = 2000;  //7849;
    int16_t kRightThumbDeadzone = 2000; //8689;
    uint16_t kTriggerMax = 0;           //1023;
    uint16_t kTriggerDeadzone = 0;      //120;
    */

public:
    Dualshock4Controller(std::unique_ptr<IUSBDevice> &&interface);
    virtual ~Dualshock4Controller();

    virtual Status Initialize();
    virtual void Exit();

    Status OpenInterfaces();
    void CloseInterfaces();

    virtual Status GetInput();

    virtual NormalizedButtonData GetNormalizedButtonData();

    virtual ControllerType GetType() { return CONTROLLER_DUALSHOCK4; }

    inline const Dualshock4ButtonData &GetButtonData() { return m_buttonData; };

    float NormalizeTrigger(uint16_t value);
    void NormalizeAxis(int16_t x, int16_t y, int16_t deadzone, float *x_out, float *y_out);

    Status SendInitBytes();
    Status SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude);

    static void LoadConfig(const ControllerConfig *config);
};