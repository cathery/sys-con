#include "SwitchHDLHandler.h"
#include "ControllerHelpers.h"
#include <cmath>

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
    Result rc = m_controllerHandler.Initialize();
    if (R_FAILED(rc))
        return rc;

    if (DoesControllerSupport(GetController()->GetType(), SUPPORTS_NOTHING))
        return rc;

    rc = InitHdlState();
    if (R_FAILED(rc))
        return rc;

    if (DoesControllerSupport(GetController()->GetType(), SUPPORTS_PAIRING))
    {
        rc = InitOutputThread();
        if (R_FAILED(rc))
            return rc;
    }

    rc = InitInputThread();
    if (R_FAILED(rc))
        return rc;

    return rc;
}

void SwitchHDLHandler::Exit()
{
    m_controllerHandler.Exit();

    if (DoesControllerSupport(GetController()->GetType(), SUPPORTS_NOTHING))
        return;

    ExitInputThread();
    ExitOutputThread();
    ExitHdlState();
}

Result SwitchHDLHandler::InitHdlState()
{
    m_hdlHandle = 0;
    m_deviceInfo = {0};
    m_hdlState = {0};

    // Set the controller type to Pro-Controller, and set the npadInterfaceType.
    m_deviceInfo.deviceType = HidDeviceType_FullKey15;
    m_deviceInfo.npadInterfaceType = NpadInterfaceType_USB;
    // Set the controller colors. The grip colors are for Pro-Controller on [9.0.0+].
    m_deviceInfo.singleColorBody = RGBA8_MAXALPHA(107, 107, 107);
    m_deviceInfo.singleColorButtons = RGBA8_MAXALPHA(0, 0, 0);
    m_deviceInfo.colorLeftGrip = RGBA8_MAXALPHA(70, 70, 70);
    m_deviceInfo.colorRightGrip = RGBA8_MAXALPHA(70, 70, 70);

    m_hdlState.batteryCharge = 4; // Set battery charge to full.
    m_hdlState.joysticks[JOYSTICK_LEFT].dx = 0x1234;
    m_hdlState.joysticks[JOYSTICK_LEFT].dy = -0x1234;
    m_hdlState.joysticks[JOYSTICK_RIGHT].dx = 0x5678;
    m_hdlState.joysticks[JOYSTICK_RIGHT].dy = -0x5678;

    if (GetController()->IsControllerActive())
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

    if (R_SUCCEEDED(hiddbgIsHdlsVirtualDeviceAttached(m_hdlHandle, &isAttached)))
    {
        if (!isAttached)
            hiddbgAttachHdlsVirtualDevice(&m_hdlHandle, &m_deviceInfo);
    }

    return hiddbgSetHdlsState(m_hdlHandle, &m_hdlState);
}

void SwitchHDLHandler::FillHdlState(const NormalizedButtonData &data)
{
    m_hdlState.buttons = 0;

    m_hdlState.buttons |= (data.buttons[0] ? KEY_X : 0);
    m_hdlState.buttons |= (data.buttons[1] ? KEY_A : 0);
    m_hdlState.buttons |= (data.buttons[2] ? KEY_B : 0);
    m_hdlState.buttons |= (data.buttons[3] ? KEY_Y : 0);

    m_hdlState.buttons |= (data.buttons[4] ? KEY_LSTICK : 0);
    m_hdlState.buttons |= (data.buttons[5] ? KEY_RSTICK : 0);

    m_hdlState.buttons |= (data.buttons[6] ? KEY_L : 0);
    m_hdlState.buttons |= (data.buttons[7] ? KEY_R : 0);

    m_hdlState.buttons |= (data.buttons[8] ? KEY_ZL : 0);
    m_hdlState.buttons |= (data.buttons[9] ? KEY_ZR : 0);

    m_hdlState.buttons |= (data.buttons[10] ? KEY_MINUS : 0);
    m_hdlState.buttons |= (data.buttons[11] ? KEY_PLUS : 0);

    ControllerConfig *config = GetController()->GetConfig();

    if (config && config->swapDPADandLSTICK)
    {
        m_hdlState.buttons |= ((data.sticks[0].axis_y > 0.5f) ? KEY_DUP : 0);
        m_hdlState.buttons |= ((data.sticks[0].axis_x > 0.5f) ? KEY_DRIGHT : 0);
        m_hdlState.buttons |= ((data.sticks[0].axis_y < -0.5f) ? KEY_DDOWN : 0);
        m_hdlState.buttons |= ((data.sticks[0].axis_x < -0.5f) ? KEY_DLEFT : 0);

        float daxis_x{}, daxis_y{};

        daxis_y += data.buttons[12] ? 1.0f : 0.0f;  //DUP
        daxis_x += data.buttons[13] ? 1.0f : 0.0f;  //DRIGHT
        daxis_y += data.buttons[14] ? -1.0f : 0.0f; //DDOWN
        daxis_x += data.buttons[15] ? -1.0f : 0.0f; //DLEFT

        m_controllerHandler.ConvertAxisToSwitchAxis(daxis_x, daxis_y, 0, &m_hdlState.joysticks[JOYSTICK_LEFT].dx, &m_hdlState.joysticks[JOYSTICK_LEFT].dy);
    }
    else
    {
        m_hdlState.buttons |= (data.buttons[12] ? KEY_DUP : 0);
        m_hdlState.buttons |= (data.buttons[13] ? KEY_DRIGHT : 0);
        m_hdlState.buttons |= (data.buttons[14] ? KEY_DDOWN : 0);
        m_hdlState.buttons |= (data.buttons[15] ? KEY_DLEFT : 0);

        m_controllerHandler.ConvertAxisToSwitchAxis(data.sticks[0].axis_x, data.sticks[0].axis_y, 0, &m_hdlState.joysticks[JOYSTICK_LEFT].dx, &m_hdlState.joysticks[JOYSTICK_LEFT].dy);
    }

    m_controllerHandler.ConvertAxisToSwitchAxis(data.sticks[1].axis_x, data.sticks[1].axis_y, 0, &m_hdlState.joysticks[JOYSTICK_RIGHT].dx, &m_hdlState.joysticks[JOYSTICK_RIGHT].dy);

    m_hdlState.buttons |= (data.buttons[16] ? KEY_CAPTURE : 0);
    m_hdlState.buttons |= (data.buttons[17] ? KEY_HOME : 0);
}

void SwitchHDLHandler::UpdateInput()
{
    Result rc;
    rc = GetController()->GetInput();
    if (R_FAILED(rc))
        return;

    if (!GetController()->IsControllerActive())
    {
        hiddbgDetachHdlsVirtualDevice(m_hdlHandle);
    }
    else
    {
        FillHdlState(GetController()->GetNormalizedButtonData());
        rc = UpdateHdlState();
        if (R_FAILED(rc))
            return;
    }
}

void SwitchHDLHandler::UpdateOutput()
{
    if (R_SUCCEEDED(GetController()->OutputBuffer()))
        return;

    if (DoesControllerSupport(GetController()->GetType(), SUPPORTS_RUMBLE))
    {
        Result rc;
        HidVibrationValue value;
        rc = hidGetActualVibrationValue(&m_vibrationDeviceHandle, &value);
        if (R_SUCCEEDED(rc))
            GetController()->SetRumble(static_cast<uint8_t>(value.amp_high * 255.0f), static_cast<uint8_t>(value.amp_low * 255.0f));
    }

    svcSleepThread(1e+7L);
}