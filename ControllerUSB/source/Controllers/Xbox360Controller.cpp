#include "Controllers/Xbox360Controller.h"
#include <cmath>

static ControllerConfig _xbox360ControllerConfig{};

Xbox360Controller::Xbox360Controller(std::unique_ptr<IUSBDevice> &&interface)
    : IController(std::move(interface))
{
}

Xbox360Controller::~Xbox360Controller()
{
    Exit();
}

Status Xbox360Controller::Initialize()
{
    Status rc;

    rc = OpenInterfaces();
    if (S_FAILED(rc))
        return rc;
    return rc;
}
void Xbox360Controller::Exit()
{
    CloseInterfaces();
}

Status Xbox360Controller::OpenInterfaces()
{
    Status rc;
    rc = m_device->Open();
    if (S_FAILED(rc))
        return rc;

    //This will open each interface and try to acquire Xbox One controller's in and out endpoints, if it hasn't already
    std::vector<std::unique_ptr<IUSBInterface>> &interfaces = m_device->GetInterfaces();
    for (auto &&interface : interfaces)
    {
        rc = interface->Open();
        if (S_FAILED(rc))
            return rc;

        if (interface->GetDescriptor()->bInterfaceProtocol != 1)
            continue;

        if (!m_inPipe)
        {
            IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, 0);
            if (inEndpoint)
            {
                rc = inEndpoint->Open();
                if (S_FAILED(rc))
                    return 55555;

                m_inPipe = inEndpoint;
            }
        }

        if (!m_outPipe)
        {
            IUSBEndpoint *outEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_OUT, 0);
            if (outEndpoint)
            {
                rc = outEndpoint->Open();
                if (S_FAILED(rc))
                    return 66666;

                m_outPipe = outEndpoint;
            }
        }
    }

    if (!m_inPipe || !m_outPipe)
        return 369;

    return rc;
}
void Xbox360Controller::CloseInterfaces()
{
    m_device->Reset();
    m_device->Close();
}

Status Xbox360Controller::GetInput()
{
    uint8_t input_bytes[64];

    Status rc = m_inPipe->Read(input_bytes, sizeof(input_bytes));

    uint8_t type = input_bytes[0];

    if (type == XBOX360INPUT_BUTTON) //Button data
    {
        m_buttonData = *reinterpret_cast<Xbox360ButtonData *>(input_bytes);
    }

    return rc;
}

Status Xbox360Controller::SendInitBytes()
{
    uint8_t init_bytes[]{
        0x05,
        0x20, 0x00, 0x01, 0x00};

    Status rc = m_outPipe->Write(init_bytes, sizeof(init_bytes));
    return rc;
}

float Xbox360Controller::NormalizeTrigger(uint16_t value)
{
    //If the given value is below the trigger zone, save the calc and return 0, otherwise adjust the value to the deadzone
    return value < _xbox360ControllerConfig.triggerDeadzone
               ? 0
               : static_cast<float>(value - _xbox360ControllerConfig.triggerDeadzone) /
                     (UINT16_MAX - _xbox360ControllerConfig.triggerDeadzone);
}

void Xbox360Controller::NormalizeAxis(int16_t x,
                                      int16_t y,
                                      int16_t deadzone,
                                      float *x_out,
                                      float *y_out)
{
    float x_val = x;
    float y_val = y;
    // Determine how far the stick is pushed.
    //This will never exceed 32767 because if the stick is
    //horizontally maxed in one direction, vertically it must be neutral(0) and vice versa
    float real_magnitude = std::sqrt(x_val * x_val + y_val * y_val);
    // Check if the controller is outside a circular dead zone.
    if (real_magnitude > deadzone)
    {
        // Clip the magnitude at its expected maximum value.
        float magnitude = std::min(32767.0f, real_magnitude);
        // Adjust magnitude relative to the end of the dead zone.
        magnitude -= deadzone;
        // Normalize the magnitude with respect to its expected range giving a
        // magnitude value of 0.0 to 1.0
        //ratio = (currentValue / maxValue) / realValue
        float ratio = (magnitude / (32767 - deadzone)) / real_magnitude;
        // Y is negated because xbox controllers have an opposite sign from
        // the 'standard controller' recommendations.
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
NormalizedButtonData Xbox360Controller::GetNormalizedButtonData()
{
    NormalizedButtonData normalData;

    normalData.bottom_action = m_buttonData.a;
    normalData.right_action = m_buttonData.b;
    normalData.left_action = m_buttonData.x;
    normalData.top_action = m_buttonData.y;

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

    normalData.left_trigger = NormalizeTrigger(m_buttonData.trigger_left);
    normalData.right_trigger = NormalizeTrigger(m_buttonData.trigger_right);

    NormalizeAxis(m_buttonData.stick_left_x, m_buttonData.stick_left_y, _xbox360ControllerConfig.leftStickDeadzone,
                  &normalData.left_stick_x, &normalData.left_stick_y);
    NormalizeAxis(m_buttonData.stick_right_x, m_buttonData.stick_right_y, _xbox360ControllerConfig.rightStickDeadzone,
                  &normalData.right_stick_x, &normalData.right_stick_y);

    return normalData;
}

Status Xbox360Controller::SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
{
    uint8_t rumbleData[]{0x00, sizeof(Xbox360RumbleData), 0x00, strong_magnitude, weak_magnitude, 0x00, 0x00, 0x00};
    return m_outPipe->Write(rumbleData, sizeof(rumbleData));
}

void Xbox360Controller::LoadConfig(const ControllerConfig *config)
{
    _xbox360ControllerConfig = *config;
}