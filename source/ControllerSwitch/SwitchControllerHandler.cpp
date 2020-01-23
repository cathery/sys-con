#include "SwitchControllerHandler.h"
#include <cmath>

SwitchControllerHandler::SwitchControllerHandler(std::unique_ptr<IController> &&controller)
    : m_controller(std::move(controller))
{
}

SwitchControllerHandler::~SwitchControllerHandler()
{
    Exit();
}

Result SwitchControllerHandler::Initialize()
{
    Result rc = m_controller->Initialize();
    if (R_FAILED(rc))
        return rc;
    return rc;
}

void SwitchControllerHandler::Exit()
{
    m_controller->Exit();
}

void SwitchControllerHandler::ConvertAxisToSwitchAxis(float x, float y, float deadzone, s32 *x_out, s32 *y_out)
{
    float floatRange = 2.0f;
    //JOYSTICK_MAX is 1 above the s16 max value, causing crashes on various games including Xenoblade Chronicles 2
    float newRange = ((JOYSTICK_MAX-1) - JOYSTICK_MIN);

    *x_out = (((x + 1.0f) * newRange) / floatRange) + JOYSTICK_MIN;
    *y_out = (((y + 1.0f) * newRange) / floatRange) + JOYSTICK_MIN;
    /*
    OldRange = (OldMax - OldMin)  
    NewRange = (NewMax - NewMin)  
    NewValue = (((OldValue - OldMin) * NewRange) / OldRange) + NewMin
    */
}

Result SwitchControllerHandler::SetControllerVibration(float strong_mag, float weak_mag)
{
    strong_mag = std::max<float>(0.0f, std::min<float>(strong_mag, 1.0f));
    weak_mag = std::max<float>(0.0f, std::min<float>(weak_mag, 1.0f));

    return m_controller->SetRumble(static_cast<uint8_t>(strong_mag * 255.0f), static_cast<uint8_t>(weak_mag * 255.0f));
}