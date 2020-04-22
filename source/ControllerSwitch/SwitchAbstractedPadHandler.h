#pragma once

#include "switch.h"
#include "IController.h"
#include "SwitchVirtualGamepadHandler.h"

//Wrapper for AbstractedPad for switch versions [5.0.0 - 8.1.0]
class SwitchAbstractedPadHandler : public SwitchVirtualGamepadHandler
{
private:
    s8 m_abstractedPadID;
    HiddbgAbstractedPadState m_state;

public:
    //Initialize the class with specified controller
    SwitchAbstractedPadHandler(std::unique_ptr<IController> &&controller);
    ~SwitchAbstractedPadHandler();

    //Initialize controller handler, AbstractedPadState
    virtual Result Initialize() override;
    virtual void Exit() override;

    //This will be called periodically by the input threads
    virtual void UpdateInput() override;
    //This will be called periodically by the output threads
    virtual void UpdateOutput() override;

    //Separately init and close the HDL state
    Result InitAbstractedPadState();
    Result ExitAbstractedPadState();

    void FillAbstractedState(const NormalizedButtonData &data);
    Result UpdateAbstractedState();
};
