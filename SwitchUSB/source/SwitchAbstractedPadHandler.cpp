#include "SwitchAbstractedPadHandler.h"
#include <cmath>

SwitchAbstractedPadHandler::SwitchAbstractedPadHandler(std::unique_ptr<IController> &&controller)
    : SwitchVirtualGamepadHandler(std::move(controller))
{
}

SwitchAbstractedPadHandler::~SwitchAbstractedPadHandler()
{
    Exit();
}

Result SwitchAbstractedPadHandler::Initialize()
{
    Result rc = m_controllerHandler.Initialize();
    if (R_FAILED(rc))
        return rc;

    rc = InitAbstractedPadState();
    if (R_FAILED(rc))
        return rc;

    InitInputThread();
    return rc;
}

void SwitchAbstractedPadHandler::Exit()
{
    ExitAbstractedPadState();
    m_controllerHandler.Exit();
    ExitInputThread();
    //ExitRumbleThread();
}

Result SwitchAbstractedPadHandler::InitAbstractedPadState()
{
    Result rc;
    /*
    u64 pads[8] = {0};
    HiddbgAbstractedPadState states[8] = {0};
    s32 tmpout = 0;
    rc = hiddbgGetAbstractedPadsState(pads, states, sizeof(pads) / sizeof(u64), &tmpout);
    */
    m_state = {0};
    m_abstractedPadID = 0;
    m_state.type = BIT(0);
    m_state.npadInterfaceType = NpadInterfaceType_USB;
    m_state.flags = 0xff;
    m_state.state.batteryCharge = 4;
    m_state.singleColorBody = RGBA8_MAXALPHA(107, 107, 107);
    m_state.singleColorButtons = RGBA8_MAXALPHA(0, 0, 0);

    rc = hiddbgSetAutoPilotVirtualPadState(m_abstractedPadID, &m_state);
    if (R_FAILED(rc))
        return rc;

    return rc;
    /*
    m_hdlHandle = 0;
    m_deviceInfo = {0};
    m_hdlState = {0};

    // Set the controller type to Pro-Controller, and set the npadInterfaceType.
    m_deviceInfo.deviceType = HidDeviceType_FullKey3;
    m_deviceInfo.npadInterfaceType = NpadInterfaceType_USB;
    // Set the controller colors. The grip colors are for Pro-Controller on [9.0.0+].
    m_deviceInfo.singleColorBody = RGBA8_MAXALPHA(107, 107, 107);
    m_deviceInfo.singleColorButtons = RGBA8_MAXALPHA(0, 0, 0);
    m_deviceInfo.colorLeftGrip = RGBA8_MAXALPHA(23, 125, 62);
    m_deviceInfo.colorRightGrip = RGBA8_MAXALPHA(23, 125, 62);

    m_hdlState.batteryCharge = 4; // Set battery charge to full.
    m_hdlState.joysticks[JOYSTICK_LEFT].dx = 0x1234;
    m_hdlState.joysticks[JOYSTICK_LEFT].dy = -0x1234;
    m_hdlState.joysticks[JOYSTICK_RIGHT].dx = 0x5678;
    m_hdlState.joysticks[JOYSTICK_RIGHT].dy = -0x5678;

    return hiddbgAttachHdlsVirtualDevice(&m_hdlHandle, &m_deviceInfo);
    */
}
Result SwitchAbstractedPadHandler::ExitAbstractedPadState()
{
    return hiddbgUnsetAutoPilotVirtualPadState(m_abstractedPadID);
}

void SwitchAbstractedPadHandler::FillAbstractedState(const NormalizedButtonData &data)
{
    m_state.state.buttons = 0;
    m_state.state.buttons |= (data.right_action ? KEY_A : 0);
    m_state.state.buttons |= (data.bottom_action ? KEY_B : 0);
    m_state.state.buttons |= (data.top_action ? KEY_X : 0);
    m_state.state.buttons |= (data.left_action ? KEY_Y : 0);

    //Breaks when buttons has a value of more than 25 or more
    //if buttons is 0x2000 or more, it also calls a interface change event, breaking any possibility of disconnecting a controller properly
    //None of this happens on the main thread, this is a problem only when running from a separate thread
    /*
    m_state.state.buttons |= (data.left_stick_click ? KEY_LSTICK : 0);
    m_state.state.buttons |= (data.right_stick_click ? KEY_RSTICK : 0);

    m_state.state.buttons |= (data.left_bumper ? KEY_L : 0);
    m_state.state.buttons |= (data.right_bumper ? KEY_R : 0);

    m_state.state.buttons |= ((data.left_trigger > 0.0f) ? KEY_ZL : 0);
    m_state.state.buttons |= ((data.right_trigger > 0.0f) ? KEY_ZR : 0);

    m_state.state.buttons |= (data.start ? KEY_PLUS : 0);
    m_state.state.buttons |= (data.back ? KEY_MINUS : 0);

    m_state.state.buttons |= (data.dpad_left ? KEY_DLEFT : 0);
    m_state.state.buttons |= (data.dpad_up ? KEY_DUP : 0);
    m_state.state.buttons |= (data.dpad_right ? KEY_DRIGHT : 0);
    m_state.state.buttons |= (data.dpad_down ? KEY_DDOWN : 0);
    */
    m_controllerHandler.ConvertAxisToSwitchAxis(data.left_stick_x, data.left_stick_y, 0, &m_state.state.joysticks[JOYSTICK_LEFT].dx, &m_state.state.joysticks[JOYSTICK_LEFT].dy);
    m_controllerHandler.ConvertAxisToSwitchAxis(data.right_stick_x, data.right_stick_y, 0, &m_state.state.joysticks[JOYSTICK_RIGHT].dx, &m_state.state.joysticks[JOYSTICK_RIGHT].dy);
}

Result SwitchAbstractedPadHandler::UpdateAbstractedState()
{
    return hiddbgSetAutoPilotVirtualPadState(m_abstractedPadID, &m_state);
}

void SwitchAbstractedPadHandler::UpdateInput()
{
    Result rc;
    rc = GetController()->GetInput();
    if (R_FAILED(rc))
        return;

    FillAbstractedState(GetController()->GetNormalizedButtonData());
    rc = UpdateAbstractedState();
    if (R_FAILED(rc))
        return;
}

void SwitchAbstractedPadHandler::UpdateOutput()
{
    svcSleepThread(1e+7L);
}