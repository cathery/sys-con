#pragma once
#include "switch.h"
#include "IController.h"
#include "static_thread.hpp"

//This class is a base class for SwitchHDLHandler and SwitchAbstractedPaadHandler.
class SwitchVirtualGamepadHandler
{
protected:
    u32 m_vibrationDeviceHandle;
    std::unique_ptr<IController> m_controller;

    static void InputThreadLoop(void *argument);
    static void OutputThreadLoop(void *argument);

    StaticThread<0x1000> m_inputThread;
    StaticThread<0x1000> m_outputThread;

public:
    SwitchVirtualGamepadHandler(std::unique_ptr<IController> &&controller);
    virtual ~SwitchVirtualGamepadHandler();

    //Override this if you want a custom init procedure
    virtual Result Initialize();
    //Override this if you want a custom exit procedure
    virtual void Exit();

    //Separately init the input-reading thread
    Result InitInputThread();
    //Separately close the input-reading thread
    void ExitInputThread();

    //Separately init the rumble sending thread
    Result InitOutputThread();
    //Separately close the rumble sending thread
    void ExitOutputThread();

    //The function to call indefinitely by the input thread
    virtual void UpdateInput() = 0;
    //The function to call indefinitely by the output thread
    virtual void UpdateOutput() = 0;

    void ConvertAxisToSwitchAxis(float x, float y, float deadzone, s32 *x_out, s32 *y_out);

    Result SetControllerVibration(float strong_mag, float weak_mag);

    //Get the raw controller pointer
    inline IController *GetController() { return m_controller.get(); }
    inline u32 *GetVibrationHandle() { return &m_vibrationDeviceHandle; }
};