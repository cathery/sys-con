#pragma once

#include "IController.h"
#include "Controllers/XboxOneController.h"

//References used:
//https://github.com/quantus/xbox-one-controller-protocol
//https://cs.chromium.org/chromium/src/device/gamepad/xbox_controller_mac.mm

class XboxOneAdapter : public IController
{
private:
    IUSBEndpoint *m_inPipe = nullptr;
    IUSBEndpoint *m_outPipe = nullptr;

public:
    XboxOneAdapter(std::unique_ptr<IUSBDevice> &&interface);
    virtual ~XboxOneAdapter();

    virtual Status Initialize();
    virtual void Exit();

    Status OpenInterfaces();
    void CloseInterfaces();

    virtual ControllerType GetType() { return CONTROLLER_XBOXONEW; }

    Status SendInitBytes();
};