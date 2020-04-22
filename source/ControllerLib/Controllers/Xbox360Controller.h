#pragma once

#include "IController.h"

//References used:
//https://cs.chromium.org/chromium/src/device/gamepad/xbox_controller_mac.mm

struct Xbox360ButtonData
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

struct Xbox360RumbleData
{
    uint8_t command;
    uint8_t size;
    uint8_t dummy1;
    uint8_t big;
    uint8_t little;
    uint8_t dummy2[3];
};

enum Xbox360InputPacketType : uint8_t
{
    XBOX360INPUT_BUTTON = 0,
    XBOX360INPUT_LED = 1,
    XBOX360INPUT_RUMBLE = 3,
};

enum Xbox360LEDValue : uint8_t
{
    XBOX360LED_OFF,
    XBOX360LED_ALLBLINK,
    XBOX360LED_TOPLEFTBLINK,
    XBOX360LED_TOPRIGHTBLINK,
    XBOX360LED_BOTTOMLEFTBLINK,
    XBOX360LED_BOTTOMRIGHTBLINK,
    XBOX360LED_TOPLEFT,
    XBOX360LED_TOPRIGHT,
    XBOX360LED_BOTTOMLEFT,
    XBOX360LED_BOTTOMRIGHT,
    XBOX360LED_ROTATE,
    XBOX360LED_BLINK,
    XBOX360LED_SLOWBLINK,
    XBOX360LED_ROTATE_2,
    XBOX360LED_ALLSLOWBLINK,
    XBOX360LED_BLINKONCE,
};

class Xbox360Controller : public IController
{
private:
    IUSBEndpoint *m_inPipe = nullptr;
    IUSBEndpoint *m_outPipe = nullptr;

    Xbox360ButtonData m_buttonData{};

public:
    Xbox360Controller(std::unique_ptr<IUSBDevice> &&interface);
    virtual ~Xbox360Controller() override;

    virtual Result Initialize() override;
    virtual void Exit() override;

    Result OpenInterfaces();
    void CloseInterfaces();

    virtual Result GetInput() override;

    virtual NormalizedButtonData GetNormalizedButtonData() override;

    virtual ControllerType GetType() override { return CONTROLLER_XBOX360; }

    inline const Xbox360ButtonData &GetButtonData() { return m_buttonData; };

    float NormalizeTrigger(uint8_t deadzonePercent, uint8_t value);
    void NormalizeAxis(int16_t x, int16_t y, uint8_t deadzonePercent, float *x_out, float *y_out);

    Result SendInitBytes();
    Result SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude);

    Result SetLED(Xbox360LEDValue value);

    static void LoadConfig(const ControllerConfig *config);
    virtual ControllerConfig *GetConfig() override;
};