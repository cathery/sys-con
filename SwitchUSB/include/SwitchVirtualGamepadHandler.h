#pragma once
#include "switch.h"
#include "IController.h"
#include "SwitchControllerHandler.h"
#include <thread>

//This class is a base class for SwitchHDLHandler and SwitchAbstractedPaadHandler.
class SwitchVirtualGamepadHandler
{
protected:
    SwitchControllerHandler m_controllerHandler;

    bool m_keepInputThreadRunning;
    std::thread m_inputThread;

    bool m_keepOutputThreadRunning;
    std::thread m_outputThread;

public:
    SwitchVirtualGamepadHandler(std::unique_ptr<IController> &&controller);
    virtual ~SwitchVirtualGamepadHandler();

    //Don't allow to move this class in any way because it holds a thread member that reads from the instance's address
    SwitchVirtualGamepadHandler(SwitchVirtualGamepadHandler &&other) = delete;
    SwitchVirtualGamepadHandler(SwitchVirtualGamepadHandler &other) = delete;
    SwitchVirtualGamepadHandler &operator=(SwitchVirtualGamepadHandler &&other) = delete;
    SwitchVirtualGamepadHandler &operator=(SwitchVirtualGamepadHandler &other) = delete;

    //Override this if you want a custom init procedure
    virtual Result Initialize();
    //Override this if you want a custom exit procedure
    virtual void Exit();

    //Separately init the input-reading thread
    void InitInputThread();
    //Separately close the input-reading thread
    void ExitInputThread();

    //Separately init the rumble sending thread
    void InitOutputThread();
    //Separately close the rumble sending thread
    void ExitOutputThread();

    //The function to call indefinitely by the input thread
    virtual void UpdateInput() = 0;
    //The function to call indefinitely by the output thread
    virtual void UpdateOutput() = 0;

    //Get the raw controller handler pointer
    inline SwitchControllerHandler *GetControllerHandler() { return &m_controllerHandler; }
    inline IController *GetController() { return m_controllerHandler.GetController(); }
};