#include "SwitchVirtualGamepadHandler.h"

SwitchVirtualGamepadHandler::SwitchVirtualGamepadHandler(std::unique_ptr<IController>&& controller)
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

void SwitchVirtualGamepadHandler::InputThreadLoop(void* handler)
{
    do
    {
        static_cast<SwitchVirtualGamepadHandler*>(handler)->UpdateInput();
    } while (static_cast<SwitchVirtualGamepadHandler*>(handler)->m_inputThreadIsRunning);
}

void SwitchVirtualGamepadHandler::OutputThreadLoop(void* handler)
{
    do
    {
        static_cast<SwitchVirtualGamepadHandler*>(handler)->UpdateOutput();
    } while (static_cast<SwitchVirtualGamepadHandler*>(handler)->m_outputThreadIsRunning);
}

Result SwitchVirtualGamepadHandler::InitInputThread()
{
    m_inputThreadIsRunning = true;
    R_ABORT_UNLESS(threadCreate(&m_inputThread, &SwitchVirtualGamepadHandler::InputThreadLoop, this, input_thread_stack, sizeof(input_thread_stack), 0x30, -2));
    R_ABORT_UNLESS(threadStart(&m_inputThread));
    return 0;
}

void SwitchVirtualGamepadHandler::ExitInputThread()
{
    m_inputThreadIsRunning = false;
    svcCancelSynchronization(m_inputThread.handle);
    threadWaitForExit(&m_inputThread);
    threadClose(&m_inputThread);
}

Result SwitchVirtualGamepadHandler::InitOutputThread()
{
    m_outputThreadIsRunning = true;
    R_ABORT_UNLESS(threadCreate(&m_outputThread, &SwitchVirtualGamepadHandler::OutputThreadLoop, this, output_thread_stack, sizeof(output_thread_stack), 0x30, -2));
    R_ABORT_UNLESS(threadStart(&m_outputThread));
    return 0;
}

void SwitchVirtualGamepadHandler::ExitOutputThread()
{
    m_outputThreadIsRunning = false;
    svcCancelSynchronization(m_outputThread.handle);
    threadWaitForExit(&m_outputThread);
    threadClose(&m_outputThread);
}

static_assert(JOYSTICK_MAX == 32767 && JOYSTICK_MIN == -32767,
              "JOYSTICK_MAX and/or JOYSTICK_MIN has incorrect values. Update libnx");

void SwitchVirtualGamepadHandler::ConvertAxisToSwitchAxis(float x, float y, float deadzone, s32* x_out, s32* y_out)
{
    float floatRange = 2.0f;
    // JOYSTICK_MAX is 1 above and JOYSTICK_MIN is 1 below acceptable joystick values, causing crashes on various games including Xenoblade Chronicles 2 and Resident Evil 4
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