#pragma once

#include "switch.h"
#include "IController.h"

class SwitchControllerHandler
{
private:
    std::unique_ptr<IController> m_controller;

public:
    //Initialize the class with specified controller
    SwitchControllerHandler(std::unique_ptr<IController> &&controller);
    ~SwitchControllerHandler();

    //Initialize controller and open a separate thread for input
    Result Initialize();
    //Clean up everything associated with the controller, reset device, close threads, etc
    void Exit();

    void ConvertAxisToSwitchAxis(float x, float y, float deadzone, s32 *x_out, s32 *y_out);

    Result SetControllerVibration(float strong_mag, float weak_mag);

    //Get the raw controller pointer
    inline IController *GetController() { return m_controller.get(); }
};