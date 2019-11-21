#pragma once
#include "SwitchUSBEndpoint.h"
#include "IUSBInterface.h"
#include <memory>
#include <vector>

class SwitchUSBInterface : public IUSBInterface
{
private:
    UsbHsClientIfSession m_session{};
    UsbHsInterface m_interface{};

    std::array<std::unique_ptr<IUSBEndpoint>, 15> m_inEndpoints;
    std::array<std::unique_ptr<IUSBEndpoint>, 15> m_outEndpoints;

public:
    //Pass the specified interface to allow for opening the session
    SwitchUSBInterface(UsbHsInterface &interface);
    ~SwitchUSBInterface();

    // Open and close the interface
    virtual Result Open() override;
    virtual void Close() override;

    virtual Result ControlTransfer(u8 bmRequestType, u8 bmRequest, u16 wValue, u16 wIndex, u16 wLength, void *buffer) override;
    virtual Result ControlTransfer(u8 bmRequestType, u8 bmRequest, u16 wValue, u16 wIndex, u16 wLength, const void *buffer) override;

    // There are a total of 15 endpoints on a switch interface for each direction, get them by passing the desired parameters
    virtual IUSBEndpoint *GetEndpoint(IUSBEndpoint::Direction direction, uint8_t index) override;

    // Reset the device
    virtual Result Reset() override;

    //Get the unique session ID for this interface
    inline s32 GetID() { return m_session.ID; }
    //Get the raw interface
    inline UsbHsInterface &GetInterface() { return m_interface; }
    //Get the raw session
    inline UsbHsClientIfSession &GetSession() { return m_session; }

    virtual InterfaceDescriptor *GetDescriptor() override { return reinterpret_cast<InterfaceDescriptor *>(&m_interface.inf.interface_desc); }
};