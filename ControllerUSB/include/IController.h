#pragma once
#include "IUSBDevice.h"
#include "ControllerTypes.h"
#include "ControllerConfig.h"
#include <memory>

struct NormalizedButtonData
{
    bool buttons[NUM_CONTROLLERBUTTONS];
    float triggers[2];
    NormalizedStick sticks[2];
};

class IController
{
protected:
    std::unique_ptr<IUSBDevice> m_device;

public:
#ifdef __APPLET__
    uint8_t m_inputData[64];
    bool m_UpdateCalled = false;
#endif
    IController(std::unique_ptr<IUSBDevice> &&interface) : m_device(std::move(interface))
    {
    }
    virtual ~IController() = default;

    virtual Result Initialize() = 0;

    //Since Exit is used to clean up resources, no Result report should be needed
    virtual void Exit() = 0;

    virtual Result GetInput() { return 1; }

    virtual NormalizedButtonData GetNormalizedButtonData() { return NormalizedButtonData(); }

    inline IUSBDevice *GetDevice() { return m_device.get(); }
    virtual ControllerType GetType() = 0;
    virtual Result SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude) { return 1; }
    virtual bool IsControllerActive() { return true; }

    virtual Result OutputBuffer() { return 1; };

    virtual ControllerConfig *GetConfig() { return nullptr; }
};