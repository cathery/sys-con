#include "Controllers/Dualshock3Controller.h"
#include <cmath>

static ControllerConfig _dualshock3ControllerConfig{};

Dualshock3Controller::Dualshock3Controller(std::unique_ptr<IUSBDevice> &&interface)
    : IController(std::move(interface))
{
}

Dualshock3Controller::~Dualshock3Controller()
{
    //Exit();
}

Result Dualshock3Controller::Initialize()
{
    Result rc;

    rc = OpenInterfaces();
    if (R_FAILED(rc))
        return rc;

    SetLED(DS3LED_1);
    return rc;
}
void Dualshock3Controller::Exit()
{
    CloseInterfaces();
}

Result Dualshock3Controller::OpenInterfaces()
{
    Result rc;
    rc = m_device->Open();
    if (R_FAILED(rc))
        return rc;

    //Open each interface, send it a setup packet and get the endpoints if it succeeds
    std::vector<std::unique_ptr<IUSBInterface>> &interfaces = m_device->GetInterfaces();
    for (auto &&interface : interfaces)
    {
        rc = interface->Open();
        if (R_FAILED(rc))
            return rc;

        if (interface->GetDescriptor()->bInterfaceClass != 3)
            continue;

        if (interface->GetDescriptor()->bInterfaceProtocol != 0)
            continue;

        if (interface->GetDescriptor()->bNumEndpoints < 2)
            continue;

        //Send an initial control packet
        constexpr uint8_t initBytes[] = {0x42, 0x0C, 0x00, 0x00};
        rc = SendCommand(interface.get(), Ds3FeatureStartDevice, initBytes, sizeof(initBytes));
        if (R_FAILED(rc))
            return 60;

        m_interface = interface.get();

        if (!m_inPipe)
        {
            for (int i = 0; i != 15; ++i)
            {
                IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, i);
                if (inEndpoint)
                {
                    rc = inEndpoint->Open();
                    if (R_FAILED(rc))
                        return 61;

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
                        return 62;

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
void Dualshock3Controller::CloseInterfaces()
{
    //m_device->Reset();
    m_device->Close();
}

Result Dualshock3Controller::GetInput()
{

    uint8_t input_bytes[49];

    Result rc = m_inPipe->Read(input_bytes, sizeof(input_bytes));
    if (R_FAILED(rc))
        return rc;

    if (input_bytes[0] == Ds3InputPacket_Button)
    {
        m_buttonData = *reinterpret_cast<Dualshock3ButtonData *>(input_bytes);
    }
    return rc;
}

float Dualshock3Controller::NormalizeTrigger(uint8_t deadzonePercent, uint8_t value)
{
    uint8_t deadzone = (UINT8_MAX * deadzonePercent) / 100;
    //If the given value is below the trigger zone, save the calc and return 0, otherwise adjust the value to the deadzone
    return value < deadzone
               ? 0
               : static_cast<float>(value - deadzone) / (UINT8_MAX - deadzone);
}

void Dualshock3Controller::NormalizeAxis(uint8_t x,
                                         uint8_t y,
                                         uint8_t deadzonePercent,
                                         float *x_out,
                                         float *y_out)
{
    float x_val = x - 127.0f;
    float y_val = 127.0f - y;
    // Determine how far the stick is pushed.
    //This will never exceed 32767 because if the stick is
    //horizontally maxed in one direction, vertically it must be neutral(0) and vice versa
    float real_magnitude = std::sqrt(x_val * x_val + y_val * y_val);
    float real_deadzone = (127 * deadzonePercent) / 100;
    // Check if the controller is outside a circular dead zone.
    if (real_magnitude > real_deadzone)
    {
        // Clip the magnitude at its expected maximum value.
        float magnitude = std::min(127.0f, real_magnitude);
        // Adjust magnitude relative to the end of the dead zone.
        magnitude -= real_deadzone;
        // Normalize the magnitude with respect to its expected range giving a
        // magnitude value of 0.0 to 1.0
        //ratio = (currentValue / maxValue) / realValue
        float ratio = (magnitude / (127 - real_deadzone)) / real_magnitude;
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
    NormalizedButtonData normalData{};

    normalData.triggers[0] = NormalizeTrigger(_dualshock3ControllerConfig.triggerDeadzonePercent[0], m_buttonData.trigger_left_pressure);
    normalData.triggers[1] = NormalizeTrigger(_dualshock3ControllerConfig.triggerDeadzonePercent[1], m_buttonData.trigger_right_pressure);

    NormalizeAxis(m_buttonData.stick_left_x, m_buttonData.stick_left_y, _dualshock3ControllerConfig.stickDeadzonePercent[0],
                  &normalData.sticks[0].axis_x, &normalData.sticks[0].axis_y);
    NormalizeAxis(m_buttonData.stick_right_x, m_buttonData.stick_right_y, _dualshock3ControllerConfig.stickDeadzonePercent[1],
                  &normalData.sticks[1].axis_x, &normalData.sticks[1].axis_y);

    bool buttons[MAX_CONTROLLER_BUTTONS] = {
        m_buttonData.triangle,
        m_buttonData.circle,
        m_buttonData.cross,
        m_buttonData.square,
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
        ControllerButton button = _dualshock3ControllerConfig.buttons[i];
        if (button == NONE)
            continue;

        normalData.buttons[(button != DEFAULT ? button - 2 : i)] += buttons[i];
    }

    return normalData;
}

Result Dualshock3Controller::SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
{
    //Not implemented yet
    return 9;
}

Result Dualshock3Controller::SendCommand(IUSBInterface *interface, Dualshock3FeatureValue feature, const void *buffer, uint16_t size)
{
    return interface->ControlTransfer(0x21, 0x09, static_cast<uint16_t>(feature), 0, size, buffer);
}

Result Dualshock3Controller::SetLED(Dualshock3LEDValue value)
{
    const uint8_t ledPacket[]{
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        static_cast<uint8_t>(value << 1),
        LED_PERMANENT,
        LED_PERMANENT,
        LED_PERMANENT,
        LED_PERMANENT};
    return SendCommand(m_interface, Ds3FeatureUnknown1, ledPacket, sizeof(ledPacket));
}

void Dualshock3Controller::LoadConfig(const ControllerConfig *config)
{
    _dualshock3ControllerConfig = *config;
}

ControllerConfig *Dualshock3Controller::GetConfig()
{
    return &_dualshock3ControllerConfig;
}