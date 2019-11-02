#include "SwitchUSBInterface.h"
#include "SwitchUSBEndpoint.h"

SwitchUSBInterface::SwitchUSBInterface(UsbHsInterface &interface)
    : m_interface(interface)
{
}

SwitchUSBInterface::~SwitchUSBInterface()
{
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

        m_inEndpoints.clear();
        m_outEndpoints.clear();

        m_inEndpoints.reserve(15);
        m_outEndpoints.reserve(15);

        //For some reason output_endpoint_descs contains IN endpoints and input_endpoint_descs contains OUT endpoints
        //we're adjusting appropriately
        for (int i = 0; i != 15; ++i)
        {
            m_inEndpoints.push_back(std::make_unique<SwitchUSBEndpoint>(m_session, m_session.inf.inf.input_endpoint_descs[i]));
        }

        for (int i = 0; i != 15; ++i)
        {
            m_outEndpoints.push_back(std::make_unique<SwitchUSBEndpoint>(m_session, m_session.inf.inf.output_endpoint_descs[i]));
        }
    }
    return rc;
}
void SwitchUSBInterface::Close()
{
    for (auto &&endpoint : m_inEndpoints)
    {
        endpoint->Close();
    }
    for (auto &&endpoint : m_outEndpoints)
    {
        endpoint->Close();
    }
    usbHsIfClose(&m_session);
}

IUSBEndpoint *SwitchUSBInterface::GetEndpoint(IUSBEndpoint::Direction direction, uint8_t index)
{
    //TODO: replace this with a control transfer to get endpoint descriptor
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