#include "Controllers/Dualshock4Controller.h"
#include <cmath>

static ControllerConfig _dualshock4ControllerConfig{};

Dualshock4Controller::Dualshock4Controller(std::unique_ptr<IUSBDevice> &&interface)
    : IController(std::move(interface))
{
}

Dualshock4Controller::~Dualshock4Controller()
{
    Exit();
}

Status Dualshock4Controller::Initialize()
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
void Dualshock4Controller::Exit()
{
    CloseInterfaces();
}

Status Dualshock4Controller::OpenInterfaces()
{
    Status rc;
    rc = m_device->Open();
    if (S_FAILED(rc))
        return rc;

    //This will open each interface and try to acquire Dualshock 4 controller's in and out endpoints, if it hasn't already
    std::vector<std::unique_ptr<IUSBInterface>> &interfaces = m_device->GetInterfaces();
    for (auto &&interface : interfaces)
    {
        rc = interface->Open();
        if (S_FAILED(rc))
            return rc;
        //TODO: check for numEndpoints before trying to open them!
        if (interface->GetDescriptor()->bNumEndpoints >= 2)
        {
            IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, 1);
            if (inEndpoint->GetDescriptor()->bLength != 0)
            {
                rc = inEndpoint->Open();
                if (S_FAILED(rc))
                    return 5555;

                m_inPipe = inEndpoint;
            }

            IUSBEndpoint *outEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_OUT, 1);
            if (outEndpoint->GetDescriptor()->bLength != 0)
            {
                rc = outEndpoint->Open();
                if (S_FAILED(rc))
                    return 6666;

                m_outPipe = outEndpoint;
            }
        }
    }

    if (!m_inPipe || !m_outPipe)
        return 69;

    return rc;
}
void Dualshock4Controller::CloseInterfaces()
{
    //m_device->Reset();
    m_device->Close();
}

Status Dualshock4Controller::GetInput()
{
    return 9;
    /*
    uint8_t input_bytes[64];

    Status rc = m_inPipe->Read(input_bytes, sizeof(input_bytes));
    if (S_FAILED(rc))
        return rc;

    uint8_t type = input_bytes[0];

    if (type == XBONEINPUT_BUTTON) //Button data
    {
        m_buttonData = *reinterpret_cast<Dualshock4ButtonData *>(input_bytes);
    }
    else if (type == XBONEINPUT_GUIDEBUTTON) //Guide button status
    {
        m_buttonData.sync = input_bytes[4];

        //Xbox one S needs to be sent an ack report for guide buttons
        //TODO: needs testing
        if (input_bytes[1] == 0x30)
        {
            rc = WriteAckGuideReport(input_bytes[2]);
            if (S_FAILED(rc))
                return rc;
        }
        //TODO: add ack check and send ack report!
    }
    
    return rc;
    */
}

Status Dualshock4Controller::SendInitBytes()
{
    uint8_t init_bytes[]{
        0x05,
        0x20, 0x00, 0x01, 0x00};

    Status rc = m_outPipe->Write(init_bytes, sizeof(init_bytes));
    return rc;
}

float Dualshock4Controller::NormalizeTrigger(uint16_t value)
{
    uint16_t deadzone = (UINT16_MAX * _dualshock4ControllerConfig.triggerDeadzonePercent) / 100;
    //If the given value is below the trigger zone, save the calc and return 0, otherwise adjust the value to the deadzone
    return value < deadzone
               ? 0
               : static_cast<float>(value - deadzone) / (UINT16_MAX - deadzone);
}

void Dualshock4Controller::NormalizeAxis(int16_t x,
                                         int16_t y,
                                         uint8_t deadzonePercent,
                                         float *x_out,
                                         float *y_out)
{
    float x_val = x;
    float y_val = y;
    // Determine how far the stick is pushed.
    //This will never exceed 32767 because if the stick is
    //horizontally maxed in one direction, vertically it must be neutral(0) and vice versa
    float real_magnitude = std::sqrt(x_val * x_val + y_val * y_val);
    float real_deadzone = (32767 * deadzonePercent) / 100;
    // Check if the controller is outside a circular dead zone.
    if (real_magnitude > real_deadzone)
    {
        // Clip the magnitude at its expected maximum value.
        float magnitude = std::min(32767.0f, real_magnitude);
        // Adjust magnitude relative to the end of the dead zone.
        magnitude -= real_deadzone;
        // Normalize the magnitude with respect to its expected range giving a
        // magnitude value of 0.0 to 1.0
        //ratio = (currentValue / maxValue) / realValue
        float ratio = (magnitude / (32767 - real_deadzone)) / real_magnitude;

        *x_out = x_val * ratio;
        *y_out = y_val * ratio;
    }
    else
    {
        // If the controller is in the deadzone zero out the magnitude.
        *x_out = *y_out = 0.0f;
    }
}

//Pass by value should hopefully be optimized away by RVO
NormalizedButtonData Dualshock4Controller::GetNormalizedButtonData()
{
    NormalizedButtonData normalData;

    return normalData;
}

Status Dualshock4Controller::SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
{
    return 9;
    /*
    uint8_t rumble_data[]{
        0x09, 0x00,
        m_rumbleDataCounter++,
        0x09, 0x00, 0x0f, 0x00, 0x00,
        strong_magnitude,
        weak_magnitude,
        0xff, 0x00, 0x00};
    return m_outPipe->Write(rumble_data, sizeof(rumble_data));
    */
}

void Dualshock4Controller::LoadConfig(const ControllerConfig *config)
{
    _dualshock4ControllerConfig = *config;
}

ControllerConfig *Dualshock4Controller::GetConfig()
{
    return &_dualshock4ControllerConfig;
}