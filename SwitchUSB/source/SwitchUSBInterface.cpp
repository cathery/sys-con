#include "SwitchUSBInterface.h"
#include "SwitchUSBEndpoint.h"
#include <malloc.h>
#include <cstring>

SwitchUSBInterface::SwitchUSBInterface(UsbHsInterface &interface)
    : m_interface(interface)
{
    m_controlTransferBuffer = memalign(0x1000, 64);
}

SwitchUSBInterface::~SwitchUSBInterface()
{
    free(m_controlTransferBuffer);
    Close();
}

Result SwitchUSBInterface::Open()
{
    UsbHsClientIfSession temp;
    Result rc = usbHsAcquireUsbIf(&temp, &m_interface);
    if (R_FAILED(rc))
        return 11037;

    m_session = temp;

    if (R_SUCCEEDED(rc))
    {

        for (int i = 0; i != 15; ++i)
        {
            usb_endpoint_descriptor &epdesc = m_session.inf.inf.input_endpoint_descs[i];
            if (epdesc.bLength != 0)
            {
                m_inEndpoints[i] = std::make_unique<SwitchUSBEndpoint>(m_session, epdesc);
            }
        }

        for (int i = 0; i != 15; ++i)
        {
            usb_endpoint_descriptor &epdesc = m_session.inf.inf.output_endpoint_descs[i];
            if (epdesc.bLength != 0)
            {
                m_outEndpoints[i] = std::make_unique<SwitchUSBEndpoint>(m_session, epdesc);
            }
        }
    }
    return rc;
}
void SwitchUSBInterface::Close()
{
    for (auto &&endpoint : m_inEndpoints)
    {
        if (endpoint)
        {
            endpoint->Close();
        }
    }
    for (auto &&endpoint : m_outEndpoints)
    {
        if (endpoint)
        {
            endpoint->Close();
        }
    }
    usbHsIfClose(&m_session);
}

Result SwitchUSBInterface::ControlTransfer(u8 bmRequestType, u8 bmRequest, u16 wValue, u16 wIndex, u16 wLength, void *buffer)
{
    if (m_controlTransferBuffer == nullptr)
        return -1;

    u32 transferredSize;

    for (u16 byte = 0; byte != wLength; ++byte)
    {
        static_cast<uint8_t *>(m_controlTransferBuffer)[byte] = static_cast<uint8_t *>(buffer)[byte];
    }

    Result rc = usbHsIfCtrlXfer(&m_session, bmRequestType, bmRequest, wValue, wIndex, wLength, m_controlTransferBuffer, &transferredSize);

    if (R_SUCCEEDED(rc))
    {
        memset(buffer, 0, wLength);
        for (u32 byte = 0; byte != transferredSize; ++byte)
        {
            static_cast<uint8_t *>(buffer)[byte] = static_cast<uint8_t *>(m_controlTransferBuffer)[byte];
        }
    }
    return rc;
}

IUSBEndpoint *SwitchUSBInterface::GetEndpoint(IUSBEndpoint::Direction direction, uint8_t index)
{
    if (direction == IUSBEndpoint::USB_ENDPOINT_IN)
        return m_inEndpoints[index].get();
    else
        return m_outEndpoints[index].get();
}

Result SwitchUSBInterface::Reset()
{
    usbHsIfResetDevice(&m_session);
    return 0;
}