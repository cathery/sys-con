#include "SwitchHDLHandler.h"
#include "ControllerHelpers.h"
#include <cmath>

static HiddbgHdlsSessionId g_hdlsSessionId;

SwitchHDLHandler::SwitchHDLHandler(std::unique_ptr<IController> &&controller)
    : SwitchVirtualGamepadHandler(std::move(controller))
{
}

SwitchHDLHandler::~SwitchHDLHandler()
{
    Exit();
}

Result SwitchHDLHandler::Initialize()
{

    R_TRY(m_controller->Initialize());

    if (DoesControllerSupport(m_controller->GetType(), SUPPORTS_NOTHING))
        return 0;

    R_TRY(InitHdlState());

    if (DoesControllerSupport(m_controller->GetType(), SUPPORTS_PAIRING))
    {
        R_TRY(InitOutputThread());
    }

    R_TRY(InitInputThread());

    return 0;
}

void SwitchHDLHandler::Exit()
{
    if (DoesControllerSupport(m_controller->GetType(), SUPPORTS_NOTHING))
    {
        m_controller->Exit();
        return;
    }

    ExitInputThread();
    ExitOutputThread();
    m_controller->Exit();
    ExitHdlState();
}

Result SwitchHDLHandler::InitHdlState()
{
    m_hdlHandle = {0};
    m_deviceInfo = {0};
    m_hdlState = {0};

    // Set the controller type to Pro-Controller, and set the npadInterfaceType.
    m_deviceInfo.deviceType = HidDeviceType_FullKey15;
    m_deviceInfo.npadInterfaceType = HidNpadInterfaceType_USB;
    // Set the controller colors. The grip colors are for Pro-Controller on [9.0.0+].
    ControllerConfig *config = m_controller->GetConfig();
    m_deviceInfo.singleColorBody = config->bodyColor.rgbaValue;
    m_deviceInfo.singleColorButtons = config->buttonsColor.rgbaValue;
    m_deviceInfo.colorLeftGrip = config->leftGripColor.rgbaValue;
    m_deviceInfo.colorRightGrip = config->rightGripColor.rgbaValue;

    m_hdlState.battery_level = 4; // Set battery charge to full.
    m_hdlState.analog_stick_l.x = 0x1234;
    m_hdlState.analog_stick_l.y = -0x1234;
    m_hdlState.analog_stick_r.x = 0x5678;
    m_hdlState.analog_stick_r.y = -0x5678;

    if (m_controller->IsControllerActive())
        return hiddbgAttachHdlsVirtualDevice(&m_hdlHandle, &m_deviceInfo);

    return 0;
}
Result SwitchHDLHandler::ExitHdlState()
{
    return hiddbgDetachHdlsVirtualDevice(m_hdlHandle);
}

//Sets the state of the class's HDL controller to the state stored in class's hdl.state
Result SwitchHDLHandler::UpdateHdlState()
{
    //Checks if the virtual device was erased, in which case re-attach the device
    bool isAttached;

    if (R_SUCCEEDED(hiddbgIsHdlsVirtualDeviceAttached(GetHdlsSessionId(), m_hdlHandle, &isAttached)))
    {
        if (!isAttached)
            hiddbgAttachHdlsVirtualDevice(&m_hdlHandle, &m_deviceInfo);
    }

    return hiddbgSetHdlsState(m_hdlHandle, &m_hdlState);
}

void SwitchHDLHandler::FillHdlState(const NormalizedButtonData &data)
{
    // we convert the input packet into switch-specific button states
    m_hdlState.buttons = 0;

    m_hdlState.buttons |= (data.buttons[0] ? HidNpadButton_X : 0);
    m_hdlState.buttons |= (data.buttons[1] ? HidNpadButton_A : 0);
    m_hdlState.buttons |= (data.buttons[2] ? HidNpadButton_B : 0);
    m_hdlState.buttons |= (data.buttons[3] ? HidNpadButton_Y : 0);

    m_hdlState.buttons |= (data.buttons[4] ? HidNpadButton_StickL : 0);
    m_hdlState.buttons |= (data.buttons[5] ? HidNpadButton_StickR : 0);

    m_hdlState.buttons |= (data.buttons[6] ? HidNpadButton_L : 0);
    m_hdlState.buttons |= (data.buttons[7] ? HidNpadButton_R : 0);

    m_hdlState.buttons |= (data.buttons[8] ? HidNpadButton_ZL : 0);
    m_hdlState.buttons |= (data.buttons[9] ? HidNpadButton_ZR : 0);

    m_hdlState.buttons |= (data.buttons[10] ? HidNpadButton_Minus : 0);
    m_hdlState.buttons |= (data.buttons[11] ? HidNpadButton_Plus : 0);

    ControllerConfig *config = m_controller->GetConfig();

    if (config && config->swapDPADandLSTICK)
    {
        m_hdlState.buttons |= ((data.sticks[0].axis_y > 0.5f) ? HidNpadButton_Up : 0);
        m_hdlState.buttons |= ((data.sticks[0].axis_x > 0.5f) ? HidNpadButton_Right : 0);
        m_hdlState.buttons |= ((data.sticks[0].axis_y < -0.5f) ? HidNpadButton_Down : 0);
        m_hdlState.buttons |= ((data.sticks[0].axis_x < -0.5f) ? HidNpadButton_Left : 0);

        float daxis_x{}, daxis_y{};

        daxis_y += data.buttons[12] ? 1.0f : 0.0f;  //DUP
        daxis_x += data.buttons[13] ? 1.0f : 0.0f;  //DRIGHT
        daxis_y += data.buttons[14] ? -1.0f : 0.0f; //DDOWN
        daxis_x += data.buttons[15] ? -1.0f : 0.0f; //DLEFT

        // clamp lefstick values to their acceptable range of values
        float real_magnitude = std::sqrt(daxis_x * daxis_x + daxis_y * daxis_y);
        float clipped_magnitude = std::min(1.0f, real_magnitude);
        float ratio = clipped_magnitude / real_magnitude;

        daxis_x *= ratio;
        daxis_y *= ratio;

        ConvertAxisToSwitchAxis(daxis_x, daxis_y, 0, &m_hdlState.analog_stick_l.x, &m_hdlState.analog_stick_l.y);
    }
    else
    {
        m_hdlState.buttons |= (data.buttons[12] ? HidNpadButton_Up : 0);
        m_hdlState.buttons |= (data.buttons[13] ? HidNpadButton_Right : 0);
        m_hdlState.buttons |= (data.buttons[14] ? HidNpadButton_Down : 0);
        m_hdlState.buttons |= (data.buttons[15] ? HidNpadButton_Left : 0);

        ConvertAxisToSwitchAxis(data.sticks[0].axis_x, data.sticks[0].axis_y, 0, &m_hdlState.analog_stick_l.x, &m_hdlState.analog_stick_l.y);
    }

    ConvertAxisToSwitchAxis(data.sticks[1].axis_x, data.sticks[1].axis_y, 0, &m_hdlState.analog_stick_r.x, &m_hdlState.analog_stick_r.y);

    m_hdlState.buttons |= (data.buttons[16] ? HiddbgNpadButton_Capture : 0);
    m_hdlState.buttons |= (data.buttons[17] ? HiddbgNpadButton_Home : 0);

    if (data.buttons[10] && data.buttons[12])
    {
        m_hdlState.state.buttons ^= HidNpadButton_Minus;
        m_hdlState.state.buttons ^= HidNpadButton_Up;
        m_hdlState.state.buttons |= HiddbgNpadButton_Capture;
    }

    if (data.buttons[10] && data.buttons[14])
    {
        m_hdlState.state.buttons ^= HidNpadButton_Minus;
        m_hdlState.state.buttons ^= HidNpadButton_Down;
        m_hdlState.state.buttons |= HiddbgNpadButton_Home;
    }
}

void SwitchHDLHandler::UpdateInput()
{
    // We process any input packets here. If it fails, return and try again
    Result rc = m_controller->GetInput();
    if (R_FAILED(rc))
        return;

    // This is a check for controllers that can prompt themselves to go inactive - e.g. wireless Xbox 360 controllers
    if (!m_controller->IsControllerActive())
    {
        hiddbgDetachHdlsVirtualDevice(m_hdlHandle);
    }
    else
    {
        // We get the button inputs from the input packet and update the state of our controller
        FillHdlState(m_controller->GetNormalizedButtonData());
        rc = UpdateHdlState();
        if (R_FAILED(rc))
            return;
    }
}

void SwitchHDLHandler::UpdateOutput()
{
    // Process a single output packet from a buffer
    if (R_SUCCEEDED(m_controller->OutputBuffer()))
        return;

    // Process rumble values if supported
    if (DoesControllerSupport(m_controller->GetType(), SUPPORTS_RUMBLE))
    {
        Result rc;
        HidVibrationValue value;
        rc = hidGetActualVibrationValue(m_vibrationDeviceHandle, &value);
        if (R_SUCCEEDED(rc))
            m_controller->SetRumble(static_cast<uint8_t>(value.amp_high * 255.0f), static_cast<uint8_t>(value.amp_low * 255.0f));
    }

    svcSleepThread(1e+7L);
}

HiddbgHdlsSessionId &SwitchHDLHandler::GetHdlsSessionId()
{
    return g_hdlsSessionId;
}
