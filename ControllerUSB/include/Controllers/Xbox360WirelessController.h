#pragma once

#include "IController.h"
#include "Xbox360Controller.h"

//References used:
//https://github.com/torvalds/linux/blob/master/drivers/input/joystick/xpad.c

struct OutputPacket
{
    const uint8_t *packet;
    uint8_t length;
};

class Xbox360WirelessController : public IController
{
private:
    IUSBEndpoint *m_inPipe = nullptr;
    IUSBEndpoint *m_outPipe = nullptr;

    Xbox360ButtonData m_buttonData;

    bool m_presence = false;

    std::vector<OutputPacket> m_outputBuffer;

public:
    Xbox360WirelessController(std::unique_ptr<IUSBDevice> &&interface);
    virtual ~Xbox360WirelessController();

    virtual Status Initialize();
    virtual void Exit();

    Status OpenInterfaces();
    void CloseInterfaces();

    virtual Status GetInput();

    virtual NormalizedButtonData GetNormalizedButtonData();

    virtual ControllerType GetType() { return CONTROLLER_XBOX360W; }

    inline const Xbox360ButtonData &GetButtonData() { return m_buttonData; };

    float NormalizeTrigger(uint8_t value);
    void NormalizeAxis(int16_t x, int16_t y, uint8_t deadzonePercent, float *x_out, float *y_out);

    Status SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude);
    Status SetLED(Xbox360LEDValue value);

    Status OnControllerConnect();
    Status OnControllerDisconnect();

    static void LoadConfig(const ControllerConfig *config);
    virtual ControllerConfig *GetConfig();

    Status WriteToEndpoint(const uint8_t *buffer, size_t size);

    virtual Status OutputBuffer();

    bool IsControllerActive() override { return m_presence; }
};