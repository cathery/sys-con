#pragma once

#include "IController.h"
#include "Controllers/XboxOneController.h"

//References used:
//https://github.com/quantus/xbox-one-controller-protocol
//https://cs.chromium.org/chromium/src/device/gamepad/xbox_controller_mac.mm

enum VendorRequest
{
    MT_VEND_DEV_MODE = 0x1,
    MT_VEND_WRITE = 0x2,
    MT_VEND_MULTI_WRITE = 0x6,
    MT_VEND_MULTI_READ = 0x7,
    MT_VEND_READ_EEPROM = 0x9,
    MT_VEND_WRITE_FCE = 0x42,
    MT_VEND_WRITE_CFG = 0x46,
    MT_VEND_READ_CFG = 0x47,
};

class XboxOneAdapter : public IController
{
private:
    IUSBEndpoint *m_inPipePacket = nullptr;
    IUSBEndpoint *m_inPipe = nullptr;
    IUSBEndpoint *m_outPipe = nullptr;
    IUSBInterface *m_interface = nullptr;

public:
    XboxOneAdapter(std::unique_ptr<IUSBDevice> &&interface);
    virtual ~XboxOneAdapter();

    virtual Status Initialize();
    virtual void Exit();

    Status OpenInterfaces();
    void CloseInterfaces();

    virtual ControllerType GetType() { return CONTROLLER_XBOXONEW; }

    Status SendInitBytes();
    Status ControlWrite(IUSBInterface *interface, uint16_t address, uint32_t value, VendorRequest request = MT_VEND_MULTI_WRITE);
};