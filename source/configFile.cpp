#include "configFile.h"
#include "Controllers.h"
#include "ControllerConfig.h"

#include "switch/result.h"
#include <cstring>
#include <array>
#include "ini.h"
#include "log.h"

#define GLOBALCONFIG "config_global.ini"

#define XBOX360CONFIG "config_xbox360.ini"
#define XBOXONECONFIG "config_xboxone.ini"
#define DUALSHOCK3CONFIG "config_dualshock3.ini"
#define DUALSHOCK4CONFIG "config_dualshock4.ini"

std::array<const char *, 20> keyNames{
    "FACE_UP",
    "FACE_RIGHT",
    "FACE_DOWN",
    "FACE_LEFT",
    "LSTICK",
    "RSTICK",
    "LSTICK_CLICK",
    "RSTICK_CLICK",
    "LEFT_BUMPER",
    "RIGHT_BUMPER",
    "LEFT_TRIGGER",
    "RIGHT_TRIGGER",
    "BACK",
    "START",
    "DPAD_UP",
    "DPAD_RIGHT",
    "DPAD_DOWN",
    "DPAD_LEFT",
    "SYNC",
    "GUIDE",
};

static ControllerButton _StringToKey(const char *text)
{
    for (int i = 0; i != keyNames.size(); ++i)
    {
        if (keyNames[i] == text)
        {
            return static_cast<ControllerButton>(i);
        }
    }
    return NOT_SET;
}

static ControllerConfig temp_config;

static int _ParseConfigLine(void *dummy, const char *section, const char *name, const char *value)
{
    if (strncmp(name, "key_", 4) == 0)
    {
        ControllerButton button = _StringToKey(name + 4);
        ControllerButton buttonValue = _StringToKey(value);
        temp_config.buttons[button] = buttonValue;
        return 1;
    }
    else if (strcmp(name, "left_stick_deadzone") == 0)
    {
        temp_config.leftStickDeadzone = atoi(value);
        return 1;
    }
    else if (strcmp(name, "right_stick_deadzone") == 0)
    {
        temp_config.rightStickDeadzone = atoi(value);
        return 1;
    }
    else if (strcmp(name, "left_stick_rotation") == 0)
    {
        temp_config.leftStickRotation = atoi(value);
        return 1;
    }
    else if (strcmp(name, "right_stick_rotation") == 0)
    {
        temp_config.rightStickRotation = atoi(value);
        return 1;
    }
    else if (strcmp(name, "trigger_deadzone") == 0)
    {
        temp_config.triggerDeadzone = atoi(value);
        return 1;
    }

    return 0;
}

static Result _ReadFromConfig(const char *path)
{
    temp_config = {};
    return ini_parse(path, _ParseConfigLine, NULL);
}

void LoadAllConfigs()
{
    if (R_SUCCEEDED(_ReadFromConfig(CONFIG_PATH XBOXONECONFIG)))
        XboxOneController::LoadConfig(&temp_config);
    else
        WriteToLog("Failed to read from xbox one config!");

    if (R_SUCCEEDED(_ReadFromConfig(CONFIG_PATH XBOX360CONFIG)))
        Xbox360Controller::LoadConfig(&temp_config);
    else
        WriteToLog("Failed to read from xbox 360 config!");

    if (R_SUCCEEDED(_ReadFromConfig(CONFIG_PATH DUALSHOCK3CONFIG)))
        Dualshock3Controller::LoadConfig(&temp_config);
    else
        WriteToLog("Failed to read from dualshock 3 config!");

    if (R_SUCCEEDED(_ReadFromConfig(CONFIG_PATH DUALSHOCK4CONFIG)))
        Dualshock4Controller::LoadConfig(&temp_config);
    else
        WriteToLog("Failed to read from dualshock 4 config!");
}

/*
//Config example
[config_global.ini]
left_stick_deadzone = 0
right_stick_deadzone = 0
left_stick_rotation = 0
right_stick_rotation = 0
trigger_deadzone = 0
key_FACE_UP = FACE_UP
key_FACE_RIGHT = FACE_RIGHT
key_FACE_DOWN = FACE_DOWN
key_FACE_LEFT = FACE_LEFT
key_LSTICK = LSTICK
key_RSTICK = RSTICK
key_LSTICK_CLICK = LSTICK_CLICK
key_RSTICK_CLICK = RSTICK_CLICK
key_LEFT_BUMPER = LEFT_BUMPER
key_RIGHT_BUMPER = RIGHT_BUMPER
key_LEFT_TRIGGER = LEFT_TRIGGER
key_RIGHT_TRIGGER = RIGHT_TRIGGER
key_BACK = BACK
key_START = START
key_DPAD_UP = DPAD_UP
key_DPAD_RIGHT = DPAD_RIGHT
key_DPAD_DOWN = DPAD_DOWN;
key_DPAD_LEFT = DPAD_LEFT
key_SYNC = SYNC
key_GUIDE = GUIDE

[config_xboxone.ini]
left_stick_deadzone = 2500
right_stick_deadzone = 3500
left_stick_rotation = 0
right_stick_rotation = 0
trigger_deadzone = 0
key_FACE_UP = FACE_UP
key_FACE_RIGHT = FACE_RIGHT
key_FACE_DOWN = FACE_DOWN
key_FACE_LEFT = FACE_LEFT
key_LSTICK = LSTICK
key_RSTICK = RSTICK
key_LSTICK_CLICK = LSTICK_CLICK
key_RSTICK_CLICK = RSTICK_CLICK
key_LEFT_BUMPER = LEFT_BUMPER
key_RIGHT_BUMPER = RIGHT_BUMPER
key_LEFT_TRIGGER = LEFT_TRIGGER
key_RIGHT_TRIGGER = RIGHT_TRIGGER
key_BACK = BACK
key_START = START
key_DPAD_UP = DPAD_UP
key_DPAD_RIGHT = DPAD_RIGHT
key_DPAD_DOWN = DPAD_DOWN;
key_DPAD_LEFT = DPAD_LEFT
key_SYNC = SYNC
key_GUIDE = GUIDE
*/
