#pragma once

#include "IController.h"

//References used:
//https://github.com/quantus/xbox-one-controller-protocol
//https://cs.chromium.org/chromium/src/device/gamepad/xbox_controller_mac.mm

struct XboxOneButtonData
{
    uint8_t type;
    uint8_t const_0;
    uint16_t id;

    bool sync : 1;
    bool dummy1 : 1; // Always 0.
    bool start : 1;
    bool back : 1;

    bool a : 1;
    bool b : 1;
    bool x : 1;
    bool y : 1;

    bool dpad_up : 1;
    bool dpad_down : 1;
    bool dpad_left : 1;
    bool dpad_right : 1;

    bool bumper_left : 1;
    bool bumper_right : 1;
    bool stick_left_click : 1;
    bool stick_right_click : 1;

    uint16_t trigger_left;
    uint16_t trigger_right;

    int16_t stick_left_x;
    int16_t stick_left_y;
    int16_t stick_right_x;
    int16_t stick_right_y;
};

struct XboxOneGuideData
{
    uint8_t down;
    uint8_t dummy1;
};

struct XboxOneRumbleData
{
    uint8_t command;
    uint8_t dummy1;
    uint8_t counter;
    uint8_t size;
    uint8_t mode;
    uint8_t rumble_mask;
    uint8_t trigger_left;
    uint8_t trigger_right;
    uint8_t strong_magnitude;
    uint8_t weak_magnitude;
    uint8_t duration;
    uint8_t period;
    uint8_t extra;
};

enum XboxOneInputPacketType : uint8_t
{
    XBONEINPUT_BUTTON = 0x20,
    XBONEINPUT_HEARTBEAT = 0x03,
    XBONEINPUT_GUIDEBUTTON = 0x07,
    XBONEINPUT_WAITCONNECT = 0x02,
};

class XboxOneController : public IController
{
private:
    IUSBEndpoint *m_inPipe = nullptr;
    IUSBEndpoint *m_outPipe = nullptr;

    XboxOneButtonData m_buttonData{};
    bool m_GuidePressed{false};

public:
    XboxOneController(std::unique_ptr<IUSBDevice> &&interface);
    virtual ~XboxOneController() override;

    virtual Result Initialize() override;
    virtual void Exit() override;

    Result OpenInterfaces();
    void CloseInterfaces();

    virtual Result GetInput() override;

    virtual NormalizedButtonData GetNormalizedButtonData() override;

    virtual ControllerType GetType() override { return CONTROLLER_XBOXONE; }

    inline const XboxOneButtonData &GetButtonData() { return m_buttonData; };

    float NormalizeTrigger(uint8_t deadzonePercent, uint16_t value);
    void NormalizeAxis(int16_t x, int16_t y, uint8_t deadzonePercent, float *x_out, float *y_out);

    Result SendInitBytes();
    Result WriteAckGuideReport(uint8_t sequence);
    Result SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude);

    static void LoadConfig(const ControllerConfig *config);
    virtual ControllerConfig *GetConfig() override;
};