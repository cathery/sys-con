#include "Controllers/Dualshock3Controller.h"
#include <cmath>

static ControllerConfig _dualshock3ControllerConfig{};

Dualshock3Controller::Dualshock3Controller(std::unique_ptr<IUSBDevice> &&interface)
    : IController(std::move(interface))
{
}

Dualshock3Controller::~Dualshock3Controller()
{
    Exit();
}

Status Dualshock3Controller::Initialize()
{
    Status rc;

    rc = OpenInterfaces();
    if (S_FAILED(rc))
        return rc;

    return rc;
}
void Dualshock3Controller::Exit()
{
    CloseInterfaces();
}

Status Dualshock3Controller::OpenInterfaces()
{
    Status rc;
    rc = m_device->Open();
    if (S_FAILED(rc))
        return rc;

    //Open each interface, send it a setup packet and get the endpoints if it succeeds
    std::vector<std::unique_ptr<IUSBInterface>> &interfaces = m_device->GetInterfaces();
    for (auto &&interface : interfaces)
    {
        rc = interface->Open();
        if (S_FAILED(rc))
            return rc;
        if (interface->GetDescriptor()->bNumEndpoints >= 2)
        {
            //Send an initial control packet
            uint8_t initBytes[] = {0x42, 0x0C, 0x00, 0x00};
            rc = interface->ControlTransfer(0x21, 0x09, Ds3FeatureStartDevice, 0, sizeof(initBytes), initBytes);
            if (S_FAILED(rc))
                return 60;

            IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, 0);
            if (inEndpoint)
            {
                rc = inEndpoint->Open();
                if (S_FAILED(rc))
                    return 61;

                m_inPipe = inEndpoint;
            }

            IUSBEndpoint *outEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_OUT, 1);
            if (outEndpoint)
            {
                rc = outEndpoint->Open();
                if (S_FAILED(rc))
                    return 62;

                m_outPipe = outEndpoint;
            }

            if (!m_inPipe || !m_outPipe)
                return 69;
        }
    }
    return rc;
}
void Dualshock3Controller::CloseInterfaces()
{
    m_device->Reset();
    m_device->Close();
}

Status Dualshock3Controller::GetInput()
{

    uint8_t input_bytes[49];

    Status rc = m_inPipe->Read(input_bytes, sizeof(input_bytes));
    if (S_FAILED(rc))
        return rc;

    if (input_bytes[0] == Ds3InputPacket_Button)
    {
        m_buttonData = *reinterpret_cast<Dualshock3ButtonData *>(input_bytes);
    }
    return rc;
}

float Dualshock3Controller::NormalizeTrigger(uint8_t value)
{
    //If the given value is below the trigger zone, save the calc and return 0, otherwise adjust the value to the deadzone
    return value < _dualshock3ControllerConfig.triggerDeadzone
               ? 0
               : static_cast<float>(value - _dualshock3ControllerConfig.triggerDeadzone) /
                     (UINT8_MAX - _dualshock3ControllerConfig.triggerDeadzone);
}

void Dualshock3Controller::NormalizeAxis(uint8_t x,
                                         uint8_t y,
                                         uint8_t deadzone,
                                         float *x_out,
                                         float *y_out)
{
    float x_val = x - 127.0f;
    float y_val = 127.0f - y;
    // Determine how far the stick is pushed.
    //This will never exceed 32767 because if the stick is
    //horizontally maxed in one direction, vertically it must be neutral(0) and vice versa
    float real_magnitude = std::sqrt(x_val * x_val + y_val * y_val);
    // Check if the controller is outside a circular dead zone.
    if (real_magnitude > deadzone)
    {
        // Clip the magnitude at its expected maximum value.
        float magnitude = std::min(127.0f, real_magnitude);
        // Adjust magnitude relative to the end of the dead zone.
        magnitude -= deadzone;
        // Normalize the magnitude with respect to its expected range giving a
        // magnitude value of 0.0 to 1.0
        //ratio = (currentValue / maxValue) / realValue
        float ratio = (magnitude / (127 - deadzone)) / real_magnitude;
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
NormalizedButtonData Dualshock3Controller::GetNormalizedButtonData()
{
    NormalizedButtonData normalData;

    normalData.bottom_action = m_buttonData.cross;
    normalData.right_action = m_buttonData.circle;
    normalData.left_action = m_buttonData.square;
    normalData.top_action = m_buttonData.triangle;

    normalData.dpad_up = m_buttonData.dpad_up;
    normalData.dpad_down = m_buttonData.dpad_down;
    normalData.dpad_left = m_buttonData.dpad_left;
    normalData.dpad_right = m_buttonData.dpad_right;

    normalData.back = m_buttonData.back;
    normalData.start = m_buttonData.start;

    normalData.left_bumper = m_buttonData.bumper_left;
    normalData.right_bumper = m_buttonData.bumper_right;

    normalData.left_stick_click = m_buttonData.stick_left_click;
    normalData.right_stick_click = m_buttonData.stick_right_click;

    normalData.capture = false;
    normalData.home = false;

    normalData.guide = m_buttonData.guide;

    normalData.left_trigger = m_buttonData.trigger_left;
    normalData.right_trigger = m_buttonData.trigger_right;

    NormalizeAxis(m_buttonData.stick_left_x, m_buttonData.stick_left_y, _dualshock3ControllerConfig.leftStickDeadzone,
                  &normalData.left_stick_x, &normalData.left_stick_y);
    NormalizeAxis(m_buttonData.stick_right_x, m_buttonData.stick_right_y, _dualshock3ControllerConfig.rightStickDeadzone,
                  &normalData.right_stick_x, &normalData.right_stick_y);

    return normalData;
}

Status Dualshock3Controller::SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
{
    //Not implemented yet
    return 9;
}

void Dualshock3Controller::LoadConfig(const ControllerConfig *config)
{
    _dualshock3ControllerConfig = *config;
}