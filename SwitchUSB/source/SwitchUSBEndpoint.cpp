#include "SwitchUSBEndpoint.h"
#include <cstring>
#include <cstdio>
#include <malloc.h>

SwitchUSBEndpoint::SwitchUSBEndpoint(UsbHsClientIfSession &if_session, usb_endpoint_descriptor &desc)
    : m_ifSession(if_session),
      m_descriptor(desc)
{
}

SwitchUSBEndpoint::~SwitchUSBEndpoint()
{
    Close();
}

Result SwitchUSBEndpoint::Open()
{
    Result rc = usbHsIfOpenUsbEp(&m_ifSession, &m_epSession, 1, m_descriptor.wMaxPacketSize, &m_descriptor);
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
    void *tmpbuf = memalign(0x1000, bufferSize);
    if (tmpbuf != nullptr)
    {
        u32 transferredSize = 0;
        memset(tmpbuf, 0, bufferSize);

        for (size_t byte = 0; byte != bufferSize; ++byte)
        {
            static_cast<uint8_t *>(tmpbuf)[byte] = static_cast<uint8_t *>(inBuffer)[byte];
        }

        rc = usbHsEpPostBuffer(&m_epSession, tmpbuf, bufferSize, &transferredSize);
        if (rc == 0xcc8c)
            rc = 0;

        free(tmpbuf);
    }
    return rc;
}

Result SwitchUSBEndpoint::Read(void *outBuffer, size_t bufferSize)
{
    void *tmpbuf = memalign(0x1000, bufferSize);
    if (tmpbuf == nullptr)
        return -1;

    u32 transferredSize;
    Result rc = usbHsEpPostBuffer(&m_epSession, tmpbuf, bufferSize, &transferredSize);

    if (rc == 0xcc8c)
        rc = 0;

    if (R_SUCCEEDED(rc))
    {
        for (size_t byte = 0; byte != bufferSize; ++byte)
        {
            static_cast<uint8_t *>(outBuffer)[byte] = static_cast<uint8_t *>(tmpbuf)[byte];
        }
    }

    free(tmpbuf);
    return rc;
}

IUSBEndpoint::Direction SwitchUSBEndpoint::GetDirection()
{
    return ((m_descriptor.bEndpointAddress & USB_ENDPOINT_IN) ? USB_ENDPOINT_IN : USB_ENDPOINT_OUT);
}

IUSBEndpoint::EndpointDescriptor *SwitchUSBEndpoint::GetDescriptor()
{
    return reinterpret_cast<IUSBEndpoint::EndpointDescriptor *>(&m_descriptor);
}