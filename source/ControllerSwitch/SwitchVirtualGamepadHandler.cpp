#include "SwitchVirtualGamepadHandler.h"

SwitchVirtualGamepadHandler::SwitchVirtualGamepadHandler(std::unique_ptr<IController> &&controller)
    : m_controller(std::move(controller))
{
}

SwitchVirtualGamepadHandler::~SwitchVirtualGamepadHandler()
{
}

Result SwitchVirtualGamepadHandler::Initialize()
{
    return m_controller->Initialize();
}

void SwitchVirtualGamepadHandler::Exit()
{
    m_controller->Exit();
}

void SwitchVirtualGamepadHandler::InputThreadLoop(void *handler)
{
    do
    {
        static_cast<SwitchVirtualGamepadHandler *>(handler)->UpdateInput();
    } while (static_cast<SwitchVirtualGamepadHandler *>(handler)->m_inputThreadIsRunning);
}

void SwitchVirtualGamepadHandler::OutputThreadLoop(void *handler)
{
    do
    {
        static_cast<SwitchVirtualGamepadHandler *>(handler)->UpdateOutput();
    } while (static_cast<SwitchVirtualGamepadHandler *>(handler)->m_outputThreadIsRunning);
}

Result SwitchVirtualGamepadHandler::InitInputThread()
{
    R_TRY(m_inputThread.Initialize(&SwitchVirtualGamepadHandler::InputThreadLoop, this, 0x30).GetValue());
    m_inputThreadIsRunning = true;
    return m_inputThread.Start().GetValue();
}

void SwitchVirtualGamepadHandler::ExitInputThread()
{
    m_inputThreadIsRunning = false;
    m_inputThread.CancelSynchronization();
    m_inputThread.Join();
}

Result SwitchVirtualGamepadHandler::InitOutputThread()
{
    R_TRY(m_outputThread.Initialize(&SwitchVirtualGamepadHandler::OutputThreadLoop, this, 0x30).GetValue());
    m_outputThreadIsRunning = true;
    return m_outputThread.Start().GetValue();
}

void SwitchVirtualGamepadHandler::ExitOutputThread()
{
    m_outputThreadIsRunning = false;
    m_outputThread.CancelSynchronization();
    m_outputThread.Join();
}

static_assert(JOYSTICK_MAX == 32767 && JOYSTICK_MIN == -32767,
              "JOYSTICK_MAX and/or JOYSTICK_MIN has incorrect values. Update libnx");

void SwitchVirtualGamepadHandler::ConvertAxisToSwitchAxis(float x, float y, float deadzone, s32 *x_out, s32 *y_out)
{
    float floatRange = 2.0f;
    //JOYSTICK_MAX is 1 above and JOYSTICK_MIN is 1 below acceptable joystick values, causing crashes on various games including Xenoblade Chronicles 2 and Resident Evil 4
    float newRange = (JOYSTICK_MAX - JOYSTICK_MIN);

    *x_out = (((x + 1.0f) * newRange) / floatRange) + JOYSTICK_MIN;
    *y_out = (((y + 1.0f) * newRange) / floatRange) + JOYSTICK_MIN;
    /*
    OldRange = (OldMax - OldMin)  
    NewRange = (NewMax - NewMin)  
    NewValue = (((OldValue - OldMin) * NewRange) / OldRange) + NewMin
    */
}

Result SwitchVirtualGamepadHandler::SetControllerVibration(float strong_mag, float weak_mag)
{
    strong_mag = std::max<float>(0.0f, std::min<float>(strong_mag, 1.0f));
    weak_mag = std::max<float>(0.0f, std::min<float>(weak_mag, 1.0f));

    return m_controller->SetRumble(static_cast<uint8_t>(strong_mag * 255.0f), static_cast<uint8_t>(weak_mag * 255.0f));
}