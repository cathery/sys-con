#include "SwitchUSBEndpoint.h"
#include <cstring>
#include <malloc.h>

SwitchUSBEndpoint::SwitchUSBEndpoint(UsbHsClientIfSession &if_session, usb_endpoint_descriptor &desc)
    : m_ifSession(&if_session),
      m_descriptor(&desc)
{
    m_buffer = memalign(0x1000, 64);
}

SwitchUSBEndpoint::~SwitchUSBEndpoint()
{
    free(m_buffer);
    Close();
}

Result SwitchUSBEndpoint::Open()
{
    Result rc = usbHsIfOpenUsbEp(m_ifSession, &m_epSession, 1, m_descriptor->wMaxPacketSize, m_descriptor);
    if (R_FAILED(rc))
        return 73011;
    return rc;
}

void SwitchUSBEndpoint::Close()
{
    usbHsEpClose(&m_epSession);
}

Result SwitchUSBEndpoint::Write(void *inBuffer, size_t bufferSize)
{
    Result rc = -1;
    if (m_buffer != nullptr)
    {
        u32 transferredSize = 0;
        memset(m_buffer, 0, bufferSize);

        for (size_t byte = 0; byte != bufferSize; ++byte)
        {
            static_cast<uint8_t *>(m_buffer)[byte] = static_cast<uint8_t *>(inBuffer)[byte];
        }

        rc = usbHsEpPostBuffer(&m_epSession, m_buffer, bufferSize, &transferredSize);
    }
    return rc;
}

Result SwitchUSBEndpoint::Read(void *outBuffer, size_t bufferSize)
{
    if (m_buffer == nullptr)
        return -1;

    u32 transferredSize;
    memset(m_buffer, 0, bufferSize);

    Result rc = usbHsEpPostBuffer(&m_epSession, m_buffer, bufferSize, &transferredSize);

    if (R_SUCCEEDED(rc))
    {
        for (u32 byte = 0; byte != transferredSize; ++byte)
        {
            static_cast<uint8_t *>(outBuffer)[byte] = static_cast<uint8_t *>(m_buffer)[byte];
        }
    }
    return rc;
}

IUSBEndpoint::Direction SwitchUSBEndpoint::GetDirection()
{
    return ((m_descriptor->bEndpointAddress & USB_ENDPOINT_IN) ? USB_ENDPOINT_IN : USB_ENDPOINT_OUT);
}

IUSBEndpoint::EndpointDescriptor *SwitchUSBEndpoint::GetDescriptor()
{
    return reinterpret_cast<IUSBEndpoint::EndpointDescriptor *>(m_descriptor);
}