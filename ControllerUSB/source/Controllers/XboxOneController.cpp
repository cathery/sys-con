#include "Controllers/XboxOneController.h"
#include <cmath>

static ControllerConfig _xboxoneControllerConfig{};

XboxOneController::XboxOneController(std::unique_ptr<IUSBDevice> &&interface)
    : IController(std::move(interface))
{
}

XboxOneController::~XboxOneController()
{
    Exit();
}

Status XboxOneController::Initialize()
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
void XboxOneController::Exit()
{
    CloseInterfaces();
}

Status XboxOneController::OpenInterfaces()
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
        //TODO: check for numEndpoints before trying to open them!
        if (interface->GetDescriptor()->bNumEndpoints >= 2)
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
void XboxOneController::CloseInterfaces()
{
    //m_device->Reset();
    m_device->Close();
}

Status XboxOneController::GetInput()
{
    uint8_t input_bytes[18];

    Status rc = m_inPipe->Read(input_bytes, sizeof(input_bytes));
    if (S_FAILED(rc))
        return rc;

    uint8_t type = input_bytes[0];

    if (type == XBONEINPUT_BUTTON) //Button data
    {
        m_buttonData = *reinterpret_cast<XboxOneButtonData *>(input_bytes);
    }
    else if (type == XBONEINPUT_GUIDEBUTTON) //Guide button status
    {
        m_GuidePressed = input_bytes[4];

        //Xbox one S needs to be sent an ack report for guide buttons
        //TODO: needs testing
        if (input_bytes[1] == 0x30)
        {
            rc = WriteAckGuideReport(input_bytes[2]);
            if (S_FAILED(rc))
                return rc;
        }
    }

    return rc;
}

Status XboxOneController::SendInitBytes()
{
    uint8_t init_bytes[]{
        0x05,
        0x20, 0x00, 0x01, 0x00};

    Status rc = m_outPipe->Write(init_bytes, sizeof(init_bytes));
    return rc;
}

float XboxOneController::NormalizeTrigger(uint16_t value)
{
    uint16_t deadzone = (1023 * _xboxoneControllerConfig.triggerDeadzonePercent) / 100;
    //If the given value is below the trigger zone, save the calc and return 0, otherwise adjust the value to the deadzone
    return value < deadzone
               ? 0
               : static_cast<float>(value - deadzone) / (1023 - deadzone);
}

void XboxOneController::NormalizeAxis(int16_t x,
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
NormalizedButtonData XboxOneController::GetNormalizedButtonData()
{
    NormalizedButtonData normalData;

    normalData.triggers[0] = NormalizeTrigger(m_buttonData.trigger_left);
    normalData.triggers[1] = NormalizeTrigger(m_buttonData.trigger_right);

    NormalizeAxis(m_buttonData.stick_left_x, m_buttonData.stick_left_y, _xboxoneControllerConfig.leftStickDeadzonePercent,
                  &normalData.sticks[0].axis_x, &normalData.sticks[0].axis_y);
    NormalizeAxis(m_buttonData.stick_right_x, m_buttonData.stick_right_y, _xboxoneControllerConfig.rightStickDeadzonePercent,
                  &normalData.sticks[1].axis_x, &normalData.sticks[1].axis_y);

    bool buttons[NUM_CONTROLLERBUTTONS]{
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
        m_buttonData.sync,
        m_GuidePressed,
    };

    for (int i = 0; i != NUM_CONTROLLERBUTTONS; ++i)
    {
        ControllerButton button = _xboxoneControllerConfig.buttons[i];
        normalData.buttons[(button != NOT_SET ? button : i)] = buttons[i];
    }

    return normalData;
}

Status XboxOneController::WriteAckGuideReport(uint8_t sequence)
{
    Status rc;
    uint8_t report[] = {
        0x01, 0x20,
        sequence,
        0x09, 0x00, 0x07, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};

    rc = m_outPipe->Write(report, sizeof(report));
    return rc;
}

Status XboxOneController::SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
{
    uint8_t rumble_data[]{
        0x09, 0x00,
        m_rumbleDataCounter++,
        0x09, 0x00, 0x0f, 0x00, 0x00,
        strong_magnitude,
        weak_magnitude,
        0xff, 0x00, 0x00};
    return m_outPipe->Write(rumble_data, sizeof(rumble_data));
}

void XboxOneController::LoadConfig(const ControllerConfig *config)
{
    _xboxoneControllerConfig = *config;
}