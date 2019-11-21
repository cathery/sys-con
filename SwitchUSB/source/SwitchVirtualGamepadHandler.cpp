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

void inputThreadLoop(void *handler)
{
    svcSleepThread(1e+7L);
    SwitchVirtualGamepadHandler *switchHandler = static_cast<SwitchVirtualGamepadHandler *>(handler);
    bool *keepThreadRunning = switchHandler->GetInputThreadBool();

    while (*keepThreadRunning)
    {
        switchHandler->UpdateInput();
    }
}

void outputThreadLoop(void *handler)
{
    svcSleepThread(1e+7L);
    SwitchVirtualGamepadHandler *switchHandler = static_cast<SwitchVirtualGamepadHandler *>(handler);
    bool *keepThreadRunning = switchHandler->GetInputThreadBool();

    while (*keepThreadRunning)
    {
        switchHandler->UpdateOutput();
    }
}

Result SwitchVirtualGamepadHandler::InitInputThread()
{
    m_keepInputThreadRunning = true;
    Result rc = threadCreate(&m_inputThread, &inputThreadLoop, this, NULL, 0x400, 0x3B, -2);
    if (R_FAILED(rc))
        return rc;
    return threadStart(&m_inputThread);
}

void SwitchVirtualGamepadHandler::ExitInputThread()
{
    m_keepInputThreadRunning = false;
    threadWaitForExit(&m_inputThread);
    threadClose(&m_inputThread);
}

Result SwitchVirtualGamepadHandler::InitOutputThread()
{
    m_keepOutputThreadRunning = true;
    Result rc = threadCreate(&m_outputThread, &outputThreadLoop, this, NULL, 0x400, 0x3B, -2);
    if (R_FAILED(rc))
        return rc;
    return threadStart(&m_outputThread);
}

void SwitchVirtualGamepadHandler::ExitOutputThread()
{
    m_keepOutputThreadRunning = false;
    threadWaitForExit(&m_outputThread);
    threadClose(&m_outputThread);
}