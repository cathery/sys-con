#include "configFile.h"
#include "Controllers.h"
#include "ControllerConfig.h"

#include "switch/result.h"
#include <cstring>
#include <sys/stat.h>
#include <array>
#include "ini.h"
#include "log.h"

#define GLOBALCONFIG "config_global.ini"

#define XBOXCONFIG "config_xboxorig.ini"
#define XBOX360CONFIG "config_xbox360.ini"
#define XBOXONECONFIG "config_xboxone.ini"
#define DUALSHOCK3CONFIG "config_dualshock3.ini"
#define DUALSHOCK4CONFIG "config_dualshock4.ini"

std::array<const char *, NUM_CONTROLLERBUTTONS> keyNames{
    "FACE_UP",
    "FACE_RIGHT",
    "FACE_DOWN",
    "FACE_LEFT",
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
    for (int i = 0; i != NUM_CONTROLLERBUTTONS; ++i)
    {
        if (strcmp(keyNames[i], text) == 0)
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
        temp_config.buttons[buttonValue] = button;
        return 1;
    }
    else if (strcmp(name, "left_stick_deadzone") == 0)
    {
        temp_config.leftStickDeadzonePercent = atoi(value);
        return 1;
    }
    else if (strcmp(name, "right_stick_deadzone") == 0)
    {
        temp_config.rightStickDeadzonePercent = atoi(value);
        return 1;
    }
    else if (strcmp(name, "left_stick_rotation") == 0)
    {
        temp_config.leftStickRotationDegrees = atoi(value);
        return 1;
    }
    else if (strcmp(name, "right_stick_rotation") == 0)
    {
        temp_config.rightStickRotationDegrees = atoi(value);
        return 1;
    }
    else if (strcmp(name, "trigger_deadzone") == 0)
    {
        temp_config.triggerDeadzonePercent = atoi(value);
        return 1;
    }
    else if (strcmp(name, "swap_dpad_and_lstick") == 0)
    {
        temp_config.swapDPADandLSTICK = (strcmp(value, "true") ? false : true);
        return 1;
    }

    return 0;
}

static Result _ReadFromConfig(const char *path)
{
    temp_config = ControllerConfig{};
    for (int i = 0; i != NUM_CONTROLLERBUTTONS; ++i)
    {
        temp_config.buttons[i] = NOT_SET;
    }
    return ini_parse(path, _ParseConfigLine, NULL);
}

void LoadAllConfigs()
{
    if (R_SUCCEEDED(_ReadFromConfig(CONFIG_PATH XBOXCONFIG)))
    {
        XboxController::LoadConfig(&temp_config);
    }
    else
        WriteToLog("Failed to read from xbox orig config!");

    if (R_SUCCEEDED(_ReadFromConfig(CONFIG_PATH XBOXONECONFIG)))
        XboxOneController::LoadConfig(&temp_config);
    else
        WriteToLog("Failed to read from xbox one config!");

    if (R_SUCCEEDED(_ReadFromConfig(CONFIG_PATH XBOX360CONFIG)))
    {
        Xbox360Controller::LoadConfig(&temp_config);
        Xbox360WirelessController::LoadConfig(&temp_config);
    }
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

bool CheckForFileChanges()
{
    bool filesChanged = false;

    static time_t globalConfigLastModified;
    static time_t xboxConfigLastModified;
    static time_t xbox360ConfigLastModified;
    static time_t xboxOneConfigLastModified;
    static time_t dualshock3ConfigLastModified;
    static time_t dualshock4ConfigLastModified;
    struct stat result;

    if (stat(CONFIG_PATH GLOBALCONFIG, &result) == 0)
        if (globalConfigLastModified != result.st_mtime)
        {
            globalConfigLastModified = result.st_mtime;
            filesChanged = true;
        }

    if (stat(CONFIG_PATH XBOXCONFIG, &result) == 0)
        if (xboxConfigLastModified != result.st_mtime)
        {
            xboxConfigLastModified = result.st_mtime;
            filesChanged = true;
        }

    if (stat(CONFIG_PATH XBOX360CONFIG, &result) == 0)
        if (xbox360ConfigLastModified != result.st_mtime)
        {
            xbox360ConfigLastModified = result.st_mtime;
            filesChanged = true;
        }

    if (stat(CONFIG_PATH XBOXONECONFIG, &result) == 0)
        if (xboxOneConfigLastModified != result.st_mtime)
        {
            xboxOneConfigLastModified = result.st_mtime;
            filesChanged = true;
        }

    if (stat(CONFIG_PATH DUALSHOCK3CONFIG, &result) == 0)
        if (dualshock3ConfigLastModified != result.st_mtime)
        {
            dualshock3ConfigLastModified = result.st_mtime;
            filesChanged = true;
        }

    if (stat(CONFIG_PATH DUALSHOCK4CONFIG, &result) == 0)
        if (dualshock4ConfigLastModified != result.st_mtime)
        {
            dualshock4ConfigLastModified = result.st_mtime;
            filesChanged = true;
        }
    return filesChanged;
}