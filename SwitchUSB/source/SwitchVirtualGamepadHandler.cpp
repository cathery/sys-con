#include "SwitchVirtualGamepadHandler.h"

SwitchVirtualGamepadHandler::SwitchVirtualGamepadHandler(std::unique_ptr<IController> &&controller)
    : m_controllerHandler(std::move(controller))
{
}

SwitchVirtualGamepadHandler::~SwitchVirtualGamepadHandler()
{
}

Result SwitchVirtualGamepadHandler::Initialize()
{
    return 0;
}

void SwitchVirtualGamepadHandler::Exit()
{
}

void SwitchVirtualGamepadHandler::InputThreadLoop(void *handler)
{
    static_cast<SwitchVirtualGamepadHandler *>(handler)->UpdateInput();
}

void SwitchVirtualGamepadHandler::OutputThreadLoop(void *handler)
{
    static_cast<SwitchVirtualGamepadHandler *>(handler)->UpdateOutput();
}

Result SwitchVirtualGamepadHandler::InitInputThread()
{
    Result rc = m_inputThread.Initialize(0x400, 0x3B);
    if (R_SUCCEEDED(rc))
        rc = m_inputThread.Start(&SwitchVirtualGamepadHandler::InputThreadLoop, this);

    return rc;
}

void SwitchVirtualGamepadHandler::ExitInputThread()
{
    m_inputThread.Exit();
}

Result SwitchVirtualGamepadHandler::InitOutputThread()
{
    Result rc = m_outputThread.Initialize(0x400, 0x3B);
    if (R_SUCCEEDED(rc))
        rc = m_outputThread.Start(&SwitchVirtualGamepadHandler::OutputThreadLoop, this);

    return rc;
}

void SwitchVirtualGamepadHandler::ExitOutputThread()
{
    m_outputThread.Exit();
}