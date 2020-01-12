#include "SwitchUSBDevice.h"
#include <cstring>  //for memset
#include "malloc.h" //for memalign

SwitchUSBDevice::SwitchUSBDevice(UsbHsInterface *interfaces, int length)
//: m_interfaces(std::vector<std::unique_ptr<IUSBInterface>>())
{
    SetInterfaces(interfaces, length);
}

SwitchUSBDevice::~SwitchUSBDevice()
{
    Close();
}

SwitchUSBDevice::SwitchUSBDevice()
{
}

Result SwitchUSBDevice::Open()
{
    if (m_interfaces.size() != 0)
        return 0;
    else
        return 51;
}

void SwitchUSBDevice::Close()
{
    for (auto &&interface : m_interfaces)
    {
        interface->Close();
    }
}

void SwitchUSBDevice::Reset()
{
    //I'm expecting all interfaces to point to one device decsriptor
    // as such resetting on any of them should do the trick
    //TODO: needs testing
    if (m_interfaces.size() != 0)
        m_interfaces[0]->Reset();
}

void SwitchUSBDevice::SetInterfaces(UsbHsInterface *interfaces, int length)
{
    if (length > 0)
    {
        m_vendorID = interfaces->device_desc.idVendor;
        m_productID = interfaces->device_desc.idProduct;
        m_interfaces.clear();
        m_interfaces.reserve(length);
        for (int i = 0; i != length; ++i)
        {
            m_interfaces.push_back(std::make_unique<SwitchUSBInterface>(interfaces[i]));
        }
    }
}