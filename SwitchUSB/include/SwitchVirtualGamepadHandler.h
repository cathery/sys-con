#pragma once
#include "switch.h"
#include "IController.h"
#include "SwitchControllerHandler.h"
#include "SwitchThread.h"

//This class is a base class for SwitchHDLHandler and SwitchAbstractedPaadHandler.
class SwitchVirtualGamepadHandler
{
protected:
    u32 m_vibrationDeviceHandle;
    SwitchControllerHandler m_controllerHandler;

    SwitchThread m_inputThread;
    SwitchThread m_outputThread;

    static void InputThreadLoop(void *argument);
    static void OutputThreadLoop(void *argument);

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

    //Get the raw controller handler pointer
    inline SwitchControllerHandler *GetControllerHandler() { return &m_controllerHandler; }
    inline IController *GetController() { return m_controllerHandler.GetController(); }
    inline u32 *GetVibrationHandle() { return &m_vibrationDeviceHandle; }
};