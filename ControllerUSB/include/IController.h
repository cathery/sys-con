#pragma once
#include "IUSBDevice.h"
#include "ControllerTypes.h"
#include "ControllerConfig.h"
#include <memory>

struct NormalizedButtonData
{
    //ABXY; BAYX; square triangle cross circle; etc.
    bool bottom_action;
    bool right_action;
    bool left_action;
    bool top_action;

    //dpad directions
    bool dpad_up;
    bool dpad_down;
    bool dpad_left;
    bool dpad_right;

    // back start; select start; view and menu; share and options; etc.
    bool back;
    bool start;

    //bumpers
    bool left_bumper;
    bool right_bumper;

    //stick buttons
    bool left_stick_click;
    bool right_stick_click;

    //reserved for switch's capture and home buttons
    bool capture;
    bool home;

    //reserved for xbox's large led button or dualshock's PS button
    bool guide;

    //trigger values from 0.0f to 1.0f
    float left_trigger;
    float right_trigger;

    //stick position values from -1.0f to 1.0f
    float left_stick_x;
    float left_stick_y;
    float right_stick_x;
    float right_stick_y;
};

class IController
{
protected:
    std::unique_ptr<IUSBDevice> m_device;

public:
    IController(std::unique_ptr<IUSBDevice> &&interface) : m_device(std::move(interface)) {}
    virtual ~IController() = default;

    virtual Status Initialize() = 0;

    //Since Exit is used to clean up resources, no status report should be needed
    virtual void Exit() = 0;

    virtual Status GetInput() = 0;

    virtual NormalizedButtonData GetNormalizedButtonData() = 0;

    inline IUSBDevice *GetDevice() { return m_device.get(); }
    virtual ControllerType GetType() = 0;
    virtual Status SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude) = 0;
};