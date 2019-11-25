#include "SwitchUSBEndpoint.h"
#include <cstring>
#include <malloc.h>

SwitchUSBEndpoint::SwitchUSBEndpoint(UsbHsClientIfSession &if_session, usb_endpoint_descriptor &desc)
    : m_ifSession(&if_session),
      m_descriptor(&desc)
{
}

SwitchUSBEndpoint::~SwitchUSBEndpoint()
{
    Close();
}

Result SwitchUSBEndpoint::Open(int maxPacketSize)
{
    maxPacketSize = maxPacketSize != 0 ? maxPacketSize : m_descriptor->wMaxPacketSize;

    Result rc = usbHsIfOpenUsbEp(m_ifSession, &m_epSession, 1, maxPacketSize, m_descriptor);
    if (R_FAILED(rc))
        return 73011;

    m_buffer = memalign(0x1000, maxPacketSize);
    if (m_buffer == nullptr)
        return -1;
    return rc;
}

void SwitchUSBEndpoint::Close()
{
    if (m_buffer != nullptr)
    {
        free(m_buffer);
        m_buffer = nullptr;
    }

    usbHsEpClose(&m_epSession);
}

Result SwitchUSBEndpoint::Write(const void *inBuffer, size_t bufferSize)
{
    if (m_buffer == nullptr)
        return -1;
    u32 transferredSize = 0;

    for (size_t byte = 0; byte != bufferSize; ++byte)
    {
        static_cast<uint8_t *>(m_buffer)[byte] = static_cast<const uint8_t *>(inBuffer)[byte];
    }

    Result rc = usbHsEpPostBuffer(&m_epSession, m_buffer, bufferSize, &transferredSize);

    if (R_SUCCEEDED(rc))
    {
        svcSleepThread(m_descriptor->bInterval * 1e+6L);
    }
    return rc;
}

Result SwitchUSBEndpoint::Read(void *outBuffer, size_t bufferSize)
{
    if (m_buffer == nullptr)
        return -1;

    u32 transferredSize;

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