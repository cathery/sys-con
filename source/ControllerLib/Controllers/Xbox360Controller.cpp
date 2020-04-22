#include "Controllers/Xbox360Controller.h"
#include <cmath>

static ControllerConfig _xbox360ControllerConfig{};

Xbox360Controller::Xbox360Controller(std::unique_ptr<IUSBDevice> &&interface)
    : IController(std::move(interface))
{
}

Xbox360Controller::~Xbox360Controller()
{
    //Exit();
}

Result Xbox360Controller::Initialize()
{
    Result rc;

    rc = OpenInterfaces();
    if (R_FAILED(rc))
        return rc;

    SetLED(XBOX360LED_TOPLEFT);
    return rc;
}
void Xbox360Controller::Exit()
{
    CloseInterfaces();
}

Result Xbox360Controller::OpenInterfaces()
{
    Result rc;
    rc = m_device->Open();
    if (R_FAILED(rc))
        return rc;

    //This will open each interface and try to acquire Xbox One controller's in and out endpoints, if it hasn't already
    std::vector<std::unique_ptr<IUSBInterface>> &interfaces = m_device->GetInterfaces();
    for (auto &&interface : interfaces)
    {
        rc = interface->Open();
        if (R_FAILED(rc))
            return rc;

        if (interface->GetDescriptor()->bInterfaceProtocol != 1)
            continue;

        if (interface->GetDescriptor()->bNumEndpoints < 2)
            continue;

        if (!m_inPipe)
        {
            for (int i = 0; i != 15; ++i)
            {
                IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, i);
                if (inEndpoint)
                {
                    rc = inEndpoint->Open();
                    if (R_FAILED(rc))
                        return 55555;

                    m_inPipe = inEndpoint;
                    break;
                }
            }
        }

        if (!m_outPipe)
        {
            for (int i = 0; i != 15; ++i)
            {
                IUSBEndpoint *outEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_OUT, i);
                if (outEndpoint)
                {
                    rc = outEndpoint->Open();
                    if (R_FAILED(rc))
                        return 66666;

                    m_outPipe = outEndpoint;
                    break;
                }
            }
        }
    }

    if (!m_inPipe || !m_outPipe)
        return 369;

    return rc;
}
void Xbox360Controller::CloseInterfaces()
{
    //m_device->Reset();
    m_device->Close();
}

Result Xbox360Controller::GetInput()
{
    uint8_t input_bytes[64];

    Result rc = m_inPipe->Read(input_bytes, sizeof(input_bytes));

    uint8_t type = input_bytes[0];

    if (type == XBOX360INPUT_BUTTON) //Button data
    {
        m_buttonData = *reinterpret_cast<Xbox360ButtonData *>(input_bytes);
    }

    return rc;
}

Result Xbox360Controller::SendInitBytes()
{
    uint8_t init_bytes[]{
        0x05,
        0x20, 0x00, 0x01, 0x00};

    Result rc = m_outPipe->Write(init_bytes, sizeof(init_bytes));
    return rc;
}

float Xbox360Controller::NormalizeTrigger(uint8_t deadzonePercent, uint8_t value)
{
    uint16_t deadzone = (UINT8_MAX * deadzonePercent) / 100;
    //If the given value is below the trigger zone, save the calc and return 0, otherwise adjust the value to the deadzone
    return value < deadzone
               ? 0
               : static_cast<float>(value - deadzone) / (UINT8_MAX - deadzone);
}

void Xbox360Controller::NormalizeAxis(int16_t x,
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
NormalizedButtonData Xbox360Controller::GetNormalizedButtonData()
{
    NormalizedButtonData normalData{};

    normalData.triggers[0] = NormalizeTrigger(_xbox360ControllerConfig.triggerDeadzonePercent[0], m_buttonData.trigger_left);
    normalData.triggers[1] = NormalizeTrigger(_xbox360ControllerConfig.triggerDeadzonePercent[1], m_buttonData.trigger_right);

    NormalizeAxis(m_buttonData.stick_left_x, m_buttonData.stick_left_y, _xbox360ControllerConfig.stickDeadzonePercent[0],
                  &normalData.sticks[0].axis_x, &normalData.sticks[0].axis_y);
    NormalizeAxis(m_buttonData.stick_right_x, m_buttonData.stick_right_y, _xbox360ControllerConfig.stickDeadzonePercent[1],
                  &normalData.sticks[1].axis_x, &normalData.sticks[1].axis_y);

    bool buttons[MAX_CONTROLLER_BUTTONS]{
        m_buttonData.y,
        m_buttonData.b,
        m_buttonData.a,
        m_buttonData.x,
        m_buttonData.stick_left_click,
        m_buttonData.stick_right_click,
        m_buttonData.bumper_left,
        m_buttonData.bumper_right,
        normalData.triggers[0] > 0,
        normalData.triggers[1] > 0,
        m_buttonData.back,
        m_buttonData.start,
        m_buttonData.dpad_up,
        m_buttonData.dpad_right,
        m_buttonData.dpad_down,
        m_buttonData.dpad_left,
        false,
        m_buttonData.guide,
    };

    for (int i = 0; i != MAX_CONTROLLER_BUTTONS; ++i)
    {
        ControllerButton button = _xbox360ControllerConfig.buttons[i];
        if (button == NONE)
            continue;

        normalData.buttons[(button != DEFAULT ? button - 2 : i)] += buttons[i];
    }

    return normalData;
}

Result Xbox360Controller::SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
{
    uint8_t rumbleData[]{0x00, sizeof(Xbox360RumbleData), 0x00, strong_magnitude, weak_magnitude, 0x00, 0x00, 0x00};
    return m_outPipe->Write(rumbleData, sizeof(rumbleData));
}

Result Xbox360Controller::SetLED(Xbox360LEDValue value)
{
    uint8_t ledPacket[]{0x01, 0x03, static_cast<uint8_t>(value)};
    return m_outPipe->Write(ledPacket, sizeof(ledPacket));
}

void Xbox360Controller::LoadConfig(const ControllerConfig *config)
{
    _xbox360ControllerConfig = *config;
}

ControllerConfig *Xbox360Controller::GetConfig()
{
    return &_xbox360ControllerConfig;
}