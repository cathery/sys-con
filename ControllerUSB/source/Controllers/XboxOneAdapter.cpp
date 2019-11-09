#include "Controllers/XboxOneAdapter.h"
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

        /*

        if (interface->GetDescriptor()->bInterfaceProtocol != 208)
            continue;

        if (interface->GetDescriptor()->bNumEndpoints < 2)
            continue;
        */

        if (!m_inPipe)
        {
            for (uint8_t i = 0; i != 15; ++i)
            {
                IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, i);
                if (inEndpoint)
                {
                    rc = inEndpoint->Open();
                    if (S_FAILED(rc))
                        return 5555;

                    m_inPipe = inEndpoint;
                    break;
                }
            }
        }

        if (!m_outPipe)
        {
            for (uint8_t i = 0; i != 15; ++i)
            {
                IUSBEndpoint *outEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_OUT, i);
                if (outEndpoint)
                {
                    rc = outEndpoint->Open();
                    if (S_FAILED(rc))
                        return 6666;

                    m_outPipe = outEndpoint;
                    break;
                }
            }
        }
    }

    if (!m_inPipe || !m_outPipe)
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
    return 0;
}