#include "Controllers/Dualshock4Controller.h"
#include <cmath>
#include "../../source/log.h"

static ControllerConfig _dualshock4ControllerConfig{};

Dualshock4Controller::Dualshock4Controller(std::unique_ptr<IUSBDevice> &&interface)
    : IController(std::move(interface))
{
}

Dualshock4Controller::~Dualshock4Controller()
{
    Exit();
}
/*
uint32_t ComputeDualshock4Checksum(const uint8_t *report_data, uint16_t length)
{
    constexpr uint8_t bt_header = 0xa2;
    uint32_t crc = crc32(0xffffffff, &bt_header, 1);
    return unaligned_crc32(crc, report_data, length);
}
*/

Result Dualshock4Controller::SendInitBytes()
{

    constexpr uint8_t init_bytes[32] = {
        0x05, 0x07, 0x00, 0x00,
        0x7f, 0x7f,
        0x00, 0x00, 0x40,
        0x00, 0x00};

    return m_outPipe->Write(init_bytes, sizeof(init_bytes));
}

Result Dualshock4Controller::Initialize()
{
    Result rc;

    rc = OpenInterfaces();
    if (R_FAILED(rc))
        return rc;

    rc = SendInitBytes();
    if (R_FAILED(rc))
        return rc;

    WriteToLog("Max IN packet size: ", m_inPipe->GetDescriptor()->wMaxPacketSize);
    WriteToLog("Max OUT packet size: ", m_outPipe->GetDescriptor()->wMaxPacketSize);

    return GetInput();

    return rc;
}
void Dualshock4Controller::Exit()
{
    CloseInterfaces();
}

Result Dualshock4Controller::OpenInterfaces()
{
    Result rc;
    rc = m_device->Open();
    if (R_FAILED(rc))
        return rc;

    //This will open each interface and try to acquire Dualshock 4 controller's in and out endpoints, if it hasn't already
    std::vector<std::unique_ptr<IUSBInterface>> &interfaces = m_device->GetInterfaces();
    for (auto &&interface : interfaces)
    {
        rc = interface->Open();
        if (R_FAILED(rc))
            return rc;

        if (interface->GetDescriptor()->bInterfaceProtocol != 0)
            continue;

        if (interface->GetDescriptor()->bNumEndpoints < 2)
            continue;

        m_interface = interface.get();

        if (!m_inPipe)
        {
            for (int i = 0; i != 15; ++i)
            {
                IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, i);
                if (inEndpoint)
                {
                    rc = inEndpoint->Open(100);
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
                    rc = outEndpoint->Open(100);
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
void Dualshock4Controller::CloseInterfaces()
{
    //m_device->Reset();
    m_device->Close();
}

Result Dualshock4Controller::GetInput()
{
    uint8_t input_bytes[64];
    Result rc = m_inPipe->Read(input_bytes, sizeof(input_bytes));
    if (R_FAILED(rc))
    {
        m_inputData[0] = static_cast<uint8_t>(rc);
        return rc;
    }
    uint8_t type = input_bytes[0];

    if (type == 0x01)
    {
        m_buttonData = *reinterpret_cast<Dualshock4USBButtonData *>(input_bytes);
    }

    return rc;
}

float Dualshock4Controller::NormalizeTrigger(uint8_t value)
{
    uint16_t deadzone = (UINT8_MAX * _dualshock4ControllerConfig.triggerDeadzonePercent) / 100;
    //If the given value is below the trigger zone, save the calc and return 0, otherwise adjust the value to the deadzone
    return value < deadzone
               ? 0
               : static_cast<float>(value - deadzone) / (UINT8_MAX - deadzone);
}

void Dualshock4Controller::NormalizeAxis(uint8_t x,
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
NormalizedButtonData Dualshock4Controller::GetNormalizedButtonData()
{
    NormalizedButtonData normalData;

    normalData.triggers[0] = NormalizeTrigger(m_buttonData.l2_pressure);
    normalData.triggers[1] = NormalizeTrigger(m_buttonData.r2_pressure);

    NormalizeAxis(m_buttonData.stick_left_x, m_buttonData.stick_left_y, _dualshock4ControllerConfig.leftStickDeadzonePercent,
                  &normalData.sticks[0].axis_x, &normalData.sticks[0].axis_y);
    NormalizeAxis(m_buttonData.stick_right_x, m_buttonData.stick_right_y, _dualshock4ControllerConfig.rightStickDeadzonePercent,
                  &normalData.sticks[1].axis_x, &normalData.sticks[1].axis_y);

    bool buttons[NUM_CONTROLLERBUTTONS] = {
        m_buttonData.triangle,
        m_buttonData.circle,
        m_buttonData.cross,
        m_buttonData.square,
        m_buttonData.l3,
        m_buttonData.r3,
        m_buttonData.l1,
        m_buttonData.r1,
        m_buttonData.l2,
        m_buttonData.r2,
        m_buttonData.share,
        m_buttonData.options,
        (m_buttonData.dpad == DS4_UP) || (m_buttonData.dpad == DS4_UPRIGHT) || (m_buttonData.dpad == DS4_UPLEFT),
        (m_buttonData.dpad == DS4_RIGHT) || (m_buttonData.dpad == DS4_UPRIGHT) || (m_buttonData.dpad == DS4_DOWNRIGHT),
        (m_buttonData.dpad == DS4_DOWN) || (m_buttonData.dpad == DS4_DOWNRIGHT) || (m_buttonData.dpad == DS4_DOWNLEFT),
        (m_buttonData.dpad == DS4_LEFT) || (m_buttonData.dpad == DS4_UPLEFT) || (m_buttonData.dpad == DS4_DOWNLEFT),
        false, //m_buttonData.touchpad_press,
        false, //m_buttonData.psbutton,
    };

    for (int i = 0; i != NUM_CONTROLLERBUTTONS; ++i)
    {
        ControllerButton button = _dualshock4ControllerConfig.buttons[i];
        normalData.buttons[(button != NOT_SET ? button : i)] = buttons[i];
    }

    return normalData;
}

Result Dualshock4Controller::SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
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