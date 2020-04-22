#include "Controllers/XboxOneController.h"
#include <cmath>
//#include "../../Sysmodule/source/log.h"

static ControllerConfig _xboxoneControllerConfig{};

#define TRIGGER_MAXVALUE 1023

//Following input packets were referenced from https://github.com/torvalds/linux/blob/master/drivers/input/joystick/xpad.c
// and https://github.com/360Controller/360Controller/blob/master/360Controller/_60Controller.cpp

//Enables LED on the PowerA controller but disables input?
static constexpr uint8_t xboxone_powerA_ledOn[] = {
    0x04, 0x20, 0x01, 0x00};

//does something maybe
static constexpr uint8_t xboxone_test_init1[] = {
    0x01, 0x20, 0x01, 0x09, 0x00, 0x04, 0x20, 0x3a,
    0x00, 0x00, 0x00, 0x98, 0x00};

//required for all xbox one controllers
static constexpr uint8_t xboxone_fw2015_init[] = {
    0x05, 0x20, 0x00, 0x01, 0x00};

static constexpr uint8_t xboxone_hori_init[] = {
    0x01, 0x20, 0x00, 0x09, 0x00, 0x04, 0x20, 0x3a,
    0x00, 0x00, 0x00, 0x80, 0x00};

static constexpr uint8_t xboxone_pdp_init1[] = {
    0x0a, 0x20, 0x00, 0x03, 0x00, 0x01, 0x14};

static constexpr uint8_t xboxone_pdp_init2[] = {
    0x06, 0x20, 0x00, 0x02, 0x01, 0x00};

static constexpr uint8_t xboxone_rumblebegin_init[] = {
    0x09, 0x00, 0x00, 0x09, 0x00, 0x0F, 0x00, 0x00,
    0x1D, 0x1D, 0xFF, 0x00, 0x00};

static constexpr uint8_t xboxone_rumbleend_init[] = {
    0x09, 0x00, 0x00, 0x09, 0x00, 0x0F, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00};

struct VendorProductPacket
{
    uint16_t VendorID;
    uint16_t ProductID;
    const uint8_t *Packet;
    uint8_t Length;
};

static constexpr VendorProductPacket init_packets[]{
    {0x0e6f, 0x0165, xboxone_hori_init, sizeof(xboxone_hori_init)},
    {0x0f0d, 0x0067, xboxone_hori_init, sizeof(xboxone_hori_init)},

    {0x0000, 0x0000, xboxone_test_init1, sizeof(xboxone_test_init1)},

    {0x0000, 0x0000, xboxone_fw2015_init, sizeof(xboxone_fw2015_init)},
    {0x0e6f, 0x0000, xboxone_pdp_init1, sizeof(xboxone_pdp_init1)},
    {0x0e6f, 0x0000, xboxone_pdp_init2, sizeof(xboxone_pdp_init2)},
    {0x24c6, 0x0000, xboxone_rumblebegin_init, sizeof(xboxone_rumblebegin_init)},
    {0x24c6, 0x0000, xboxone_rumbleend_init, sizeof(xboxone_rumbleend_init)},
};

XboxOneController::XboxOneController(std::unique_ptr<IUSBDevice> &&interface)
    : IController(std::move(interface))
{
}

XboxOneController::~XboxOneController()
{
    //Exit();
}

Result XboxOneController::Initialize()
{
    Result rc;

    rc = OpenInterfaces();
    if (R_FAILED(rc))
        return rc;

    rc = SendInitBytes();
    if (R_FAILED(rc))
        return rc;
    return rc;
}
void XboxOneController::Exit()
{
    CloseInterfaces();
}

Result XboxOneController::OpenInterfaces()
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

        if (interface->GetDescriptor()->bInterfaceProtocol != 208)
            continue;

        if (interface->GetDescriptor()->bNumEndpoints < 2)
            continue;

        if (!m_inPipe)
        {
            for (uint8_t i = 0; i != 15; ++i)
            {
                IUSBEndpoint *inEndpoint = interface->GetEndpoint(IUSBEndpoint::USB_ENDPOINT_IN, i);
                if (inEndpoint)
                {
                    rc = inEndpoint->Open();
                    if (R_FAILED(rc))
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
                    if (R_FAILED(rc))
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

Result XboxOneController::GetInput()
{
    uint8_t input_bytes[64];

    Result rc = m_inPipe->Read(input_bytes, sizeof(input_bytes));
    if (R_FAILED(rc))
        return rc;

    uint8_t type = input_bytes[0];

    if (type == XBONEINPUT_BUTTON) //Button data
    {
        m_buttonData = *reinterpret_cast<XboxOneButtonData *>(input_bytes);
    }
    else if (type == XBONEINPUT_GUIDEBUTTON) //Guide button Result
    {
        m_GuidePressed = input_bytes[4];

        //Xbox one S needs to be sent an ack report for guide buttons
        //TODO: needs testing
        if (input_bytes[1] == 0x30)
        {
            rc = WriteAckGuideReport(input_bytes[2]);
            if (R_FAILED(rc))
                return rc;
        }
    }

    return rc;
}

Result XboxOneController::SendInitBytes()
{
    Result rc;
    uint16_t vendor = m_device->GetVendor();
    uint16_t product = m_device->GetProduct();
    for (int i = 0; i != (sizeof(init_packets) / sizeof(VendorProductPacket)); ++i)
    {
        if (init_packets[i].VendorID != 0 && init_packets[i].VendorID != vendor)
            continue;
        if (init_packets[i].ProductID != 0 && init_packets[i].ProductID != product)
            continue;

        rc = m_outPipe->Write(init_packets[i].Packet, init_packets[i].Length);
        if (R_FAILED(rc))
            break;
    }
    return rc;
}

float XboxOneController::NormalizeTrigger(uint8_t deadzonePercent, uint16_t value)
{
    uint16_t deadzone = (TRIGGER_MAXVALUE * deadzonePercent) / 100;
    //If the given value is below the trigger zone, save the calc and return 0, otherwise adjust the value to the deadzone
    return value < deadzone
               ? 0
               : static_cast<float>(value - deadzone) / (TRIGGER_MAXVALUE - deadzone);
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
    NormalizedButtonData normalData{};

    normalData.triggers[0] = NormalizeTrigger(_xboxoneControllerConfig.triggerDeadzonePercent[0], m_buttonData.trigger_left);
    normalData.triggers[1] = NormalizeTrigger(_xboxoneControllerConfig.triggerDeadzonePercent[1], m_buttonData.trigger_right);

    NormalizeAxis(m_buttonData.stick_left_x, m_buttonData.stick_left_y, _xboxoneControllerConfig.stickDeadzonePercent[0],
                  &normalData.sticks[0].axis_x, &normalData.sticks[0].axis_y);
    NormalizeAxis(m_buttonData.stick_right_x, m_buttonData.stick_right_y, _xboxoneControllerConfig.stickDeadzonePercent[1],
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
        m_buttonData.sync,
        m_GuidePressed,
    };

    for (int i = 0; i != MAX_CONTROLLER_BUTTONS; ++i)
    {
        ControllerButton button = _xboxoneControllerConfig.buttons[i];
        if (button == NONE)
            continue;

        normalData.buttons[(button != DEFAULT ? button - 2 : i)] += buttons[i];
    }

    return normalData;
}

Result XboxOneController::WriteAckGuideReport(uint8_t sequence)
{
    Result rc;
    uint8_t report[] = {
        0x01, 0x20,
        sequence,
        0x09, 0x00, 0x07, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};

    rc = m_outPipe->Write(report, sizeof(report));
    return rc;
}

Result XboxOneController::SetRumble(uint8_t strong_magnitude, uint8_t weak_magnitude)
{
    const uint8_t rumble_data[]{
        0x09, 0x00, 0x00,
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

ControllerConfig *XboxOneController::GetConfig()
{
    return &_xboxoneControllerConfig;
}