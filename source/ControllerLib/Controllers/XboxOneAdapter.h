#pragma once

#include "IController.h"
#include "Controllers/XboxOneController.h"

//References used:
//https://github.com/quantus/xbox-one-controller-protocol
//https://cs.chromium.org/chromium/src/device/gamepad/xbox_controller_mac.mm

enum VendorRequest : uint8_t
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
    virtual ~XboxOneAdapter() override;

    virtual Result Initialize() override;
    virtual void Exit() override;

    Result OpenInterfaces();
    void CloseInterfaces();

    virtual ControllerType GetType() override { return CONTROLLER_XBOXONEW; }

    Result LoadFirmwarePart(uint32_t offset, uint8_t *start, uint8_t *end);
    Result SendInitBytes();
    Result ControlWrite(IUSBInterface *interface, uint16_t address, uint32_t value, VendorRequest request = MT_VEND_MULTI_WRITE);

    static void LoadConfig(const ControllerConfig *config, const char *path);
    virtual ControllerConfig *GetConfig() override;
};