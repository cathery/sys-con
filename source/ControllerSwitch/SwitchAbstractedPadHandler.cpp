#include "SwitchAbstractedPadHandler.h"
#include "ControllerHelpers.h"
#include <cmath>
#include <array>

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
    R_TRY(m_controller->Initialize());

    if (DoesControllerSupport(GetController()->GetType(), SUPPORTS_NOTHING))
        return 0;

    R_TRY(InitAbstractedPadState());

    if (DoesControllerSupport(m_controller->GetType(), SUPPORTS_PAIRING))
    {
        R_TRY(InitOutputThread());
    }

    R_TRY(InitInputThread());

    return 0;
}

void SwitchAbstractedPadHandler::Exit()
{
    if (DoesControllerSupport(GetController()->GetType(), SUPPORTS_NOTHING))
    {
        m_controller->Exit();
        return;
    }

    ExitInputThread();
    ExitOutputThread();
    m_controller->Exit();
    ExitAbstractedPadState();
}

//Used to give out unique ids to abstracted pads
static std::array<bool, 8> uniqueIDs{};

static s8 getUniqueId()
{
    for (s8 i = 0; i != 8; ++i)
    {
        if (uniqueIDs[i] == false)
        {
            uniqueIDs[i] = true;
            return i;
        }
    }
    return 0;
}

static void freeUniqueId(s8 id)
{
    uniqueIDs[id] = false;
}

Result SwitchAbstractedPadHandler::InitAbstractedPadState()
{
    Result rc;
    m_state = {0};
    m_abstractedPadID = getUniqueId();
    m_state.type = BIT(0);
    m_state.npadInterfaceType = HidNpadInterfaceType_USB;
    m_state.flags = 0xff;
    m_state.state.battery_level = 4;
    ControllerConfig *config = GetController()->GetConfig();
    m_state.singleColorBody = config->bodyColor.rgbaValue;
    m_state.singleColorButtons = config->buttonsColor.rgbaValue;

    rc = hiddbgSetAutoPilotVirtualPadState(m_abstractedPadID, &m_state);
    if (R_FAILED(rc))
        return rc;

    return rc;
}

Result SwitchAbstractedPadHandler::ExitAbstractedPadState()
{
    freeUniqueId(m_abstractedPadID);
    return hiddbgUnsetAutoPilotVirtualPadState(m_abstractedPadID);
}

void SwitchAbstractedPadHandler::FillAbstractedState(const NormalizedButtonData &data)
{
    m_state.state.buttons = 0;
    m_state.state.buttons |= (data.buttons[0] ? HidNpadButton_X : 0);
    m_state.state.buttons |= (data.buttons[1] ? HidNpadButton_A : 0);
    m_state.state.buttons |= (data.buttons[2] ? HidNpadButton_B : 0);
    m_state.state.buttons |= (data.buttons[3] ? HidNpadButton_Y : 0);

    m_state.state.buttons |= (data.buttons[4] ? HidNpadButton_StickL : 0);
    m_state.state.buttons |= (data.buttons[5] ? HidNpadButton_StickR : 0);

    m_state.state.buttons |= (data.buttons[6] ? HidNpadButton_L : 0);
    m_state.state.buttons |= (data.buttons[7] ? HidNpadButton_R : 0);

    m_state.state.buttons |= (data.buttons[8] ? HidNpadButton_ZL : 0);
    m_state.state.buttons |= (data.buttons[9] ? HidNpadButton_ZR : 0);

    m_state.state.buttons |= (data.buttons[10] ? HidNpadButton_Minus : 0);
    m_state.state.buttons |= (data.buttons[11] ? HidNpadButton_Plus : 0);

    ControllerConfig *config = GetController()->GetConfig();

    if (config && config->swapDPADandLSTICK)
    {
        m_state.state.buttons |= ((data.sticks[0].axis_y > 0.5f) ? HidNpadButton_Up : 0);
        m_state.state.buttons |= ((data.sticks[0].axis_x > 0.5f) ? HidNpadButton_Right : 0);
        m_state.state.buttons |= ((data.sticks[0].axis_y < -0.5f) ? HidNpadButton_Down : 0);
        m_state.state.buttons |= ((data.sticks[0].axis_x < -0.5f) ? HidNpadButton_Left : 0);

        float daxis_x{}, daxis_y{};

        daxis_y += data.buttons[12] ? 1.0f : 0.0f;  //DUP
        daxis_x += data.buttons[13] ? 1.0f : 0.0f;  //DRIGHT
        daxis_y += data.buttons[14] ? -1.0f : 0.0f; //DDOWN
        daxis_x += data.buttons[15] ? -1.0f : 0.0f; //DLEFT

        ConvertAxisToSwitchAxis(daxis_x, daxis_y, 0, &m_state.state.analog_stick_l.x, &m_state.state.analog_stick_l.y);
    }
    else
    {
        m_state.state.buttons |= (data.buttons[12] ? HidNpadButton_Up : 0);
        m_state.state.buttons |= (data.buttons[13] ? HidNpadButton_Right : 0);
        m_state.state.buttons |= (data.buttons[14] ? HidNpadButton_Down : 0);
        m_state.state.buttons |= (data.buttons[15] ? HidNpadButton_Left : 0);

        ConvertAxisToSwitchAxis(data.sticks[0].axis_x, data.sticks[0].axis_y, 0, &m_state.state.analog_stick_l.x, &m_state.state.analog_stick_l.y);
    }

    ConvertAxisToSwitchAxis(data.sticks[1].axis_x, data.sticks[1].axis_y, 0, &m_state.state.analog_stick_r.x, &m_state.state.analog_stick_r.y);

    m_state.state.buttons |= (data.buttons[16] ? HiddbgNpadButton_Capture : 0);
    m_state.state.buttons |= (data.buttons[17] ? HiddbgNpadButton_Home : 0);
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
    if (R_SUCCEEDED(m_controller->OutputBuffer()))
        return;

    if (DoesControllerSupport(m_controller->GetType(), SUPPORTS_RUMBLE))
    {
        Result rc;
        HidVibrationValue value;
        rc = hidGetActualVibrationValue(m_vibrationDeviceHandle, &value);
        if (R_SUCCEEDED(rc))
            GetController()->SetRumble(static_cast<uint8_t>(value.amp_high * 255.0f), static_cast<uint8_t>(value.amp_low * 255.0f));
    }

    svcSleepThread(1e+7L);
}