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

void inputThreadLoop(bool *keepThreadRunning, SwitchVirtualGamepadHandler *handler)
{
    svcSleepThread(1e+7L);
    while (*keepThreadRunning)
    {
        handler->UpdateInput();
    }
}

void outputThreadLoop(bool *keepThreadRunning, SwitchVirtualGamepadHandler *handler)
{
    svcSleepThread(1e+7L);
    while (*keepThreadRunning)
    {
        handler->UpdateOutput();
    }
}

void SwitchVirtualGamepadHandler::InitInputThread()
{
    m_keepInputThreadRunning = true;
    m_inputThread = std::thread(inputThreadLoop, &m_keepInputThreadRunning, this);
}

void SwitchVirtualGamepadHandler::ExitInputThread()
{
    m_keepInputThreadRunning = false;
    if (m_inputThread.joinable())
        m_inputThread.join();
}

void SwitchVirtualGamepadHandler::InitOutputThread()
{
    m_keepOutputThreadRunning = true;
    m_outputThread = std::thread(outputThreadLoop, &m_keepOutputThreadRunning, this);
}

void SwitchVirtualGamepadHandler::ExitOutputThread()
{
    m_keepOutputThreadRunning = false;
    if (m_outputThread.joinable())
        m_outputThread.join();
}