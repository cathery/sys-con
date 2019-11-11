#include "Controllers/XboxOneAdapter.h"
#include "Controllers/XboxOneAdapter/Firmware.h"
#include <cmath>
#include "../../source/log.h"

XboxOneAdapter::XboxOneAdapter(std::unique_ptr<IUSBDevice> &&interface)
    : IController(std::move(interface))
{
}

XboxOneAdapter::~XboxOneAdapter()
{
    Exit();
}

Status XboxOneAdapter::Initialize()
{
    Status rc;

    rc = OpenInterfaces();
    if (S_FAILED(rc))
        return rc;

    rc = SendInitBytes();
    if (S_FAILED(rc))
        return rc;
    return rc;
}
void XboxOneAdapter::Exit()
{
    CloseInterfaces();
}

Status XboxOneAdapter::OpenInterfaces()
{
    Status rc;
    rc = m_device->Open();
    if (S_FAILED(rc))
        return rc;

    std::vector<std::unique_ptr<IUSBInterface>> &interfaces = m_device->GetInterfaces();
    for (auto &&interface : interfaces)
    {
        rc = interface->Open();
        if (S_FAILED(rc))
            return rc;

        if (interface->GetDescriptor()->bInterfaceProtocol != 255)
            continue;

        if (interface->GetDescriptor()->bNumEndpoints < 2)
            continue;

        m_interface = interface.get();

        if (!m_inPipePacket)
        {
            IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, 3);
            if (inEndpoint)
            {
                rc = inEndpoint->Open();
                if (S_FAILED(rc))
                    return 3333;

                m_inPipePacket = inEndpoint;
            }
        }

        if (!m_inPipe)
        {
            IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, 4);
            if (inEndpoint)
            {
                rc = inEndpoint->Open();
                if (S_FAILED(rc))
                    return 4444;

                m_inPipe = inEndpoint;
            }
        }

        if (!m_outPipe)
        {
            IUSBEndpoint *outEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_OUT, 3);
            if (outEndpoint)
            {
                rc = outEndpoint->Open();
                if (S_FAILED(rc))
                    return 5555;

                m_outPipe = outEndpoint;
            }
        }
    }

    if (!m_inPipe || !m_outPipe || !m_inPipePacket)
        return 69;

    return rc;
}

void XboxOneAdapter::CloseInterfaces()
{
    //m_device->Reset();
    m_device->Close();
}

Status XboxOneAdapter::SendInitBytes()
{
    Status rc;
    rc = ControlWrite(m_interface, 0x9018, 0, MT_VEND_WRITE_CFG);
    if (S_FAILED(rc))
        return rc;
    rc = ControlWrite(m_interface, 0x0800, 0x01);
    if (S_FAILED(rc))
        return rc;
    rc = ControlWrite(m_interface, 0x09a0, 0x400230);
    if (S_FAILED(rc))
        return rc;
    rc = ControlWrite(m_interface, 0x09a4, 0x01);
    if (S_FAILED(rc))
        return rc;
    rc = ControlWrite(m_interface, 0x09a8, 0x01);
    if (S_FAILED(rc))
        return rc;
    rc = ControlWrite(m_interface, 0x09c4, 0x44);
    if (S_FAILED(rc))
        return rc;
    rc = ControlWrite(m_interface, 0x0a6c, 0x03);
    if (S_FAILED(rc))
        return rc;

    return 0;
}

Status XboxOneAdapter::ControlWrite(IUSBInterface *interface, uint16_t address, uint32_t value, VendorRequest request)
{
    Status rc;
    if (request == MT_VEND_DEV_MODE)
    {
        rc = interface->ControlTransfer(0x42, request, address, 0, 0, nullptr);
    }
    else
    {
        rc = interface->ControlTransfer(0x42, request, address, 0, sizeof(uint32_t), &value);
    }
    return rc;
}