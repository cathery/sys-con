#include "switch.h"
#include "config_handler.h"
#include "Controllers.h"
#include "ControllerConfig.h"
#include "log.h"
#include "ini.h"
#include <cstring>
#include <stratosphere.hpp>
#include "usb_module.h"

namespace syscon::config
{
    namespace
    {
        ControllerConfig tempConfig;
        GlobalConfig tempGlobalConfig;
        RGBAColor tempColor;
        char firmwarePath[100];

        UTimer filecheckTimer;
        Waiter filecheckTimerWaiter = waiterForUTimer(&filecheckTimer);

        void ConfigChangedCheckThreadFunc(void *arg);

        ams::os::StaticThread<0x2'000> g_config_changed_check_thread(&ConfigChangedCheckThreadFunc, nullptr, 0x3E);

        bool is_config_changed_check_thread_running = false;

        constexpr std::array keyNames{
            "DEFAULT",
            "NONE",
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
            "CAPTURE",
            "HOME",
            "SYNC",
            "TOUCHPAD",
        };

        ControllerButton StringToKey(const char *text)
        {
            for (int i = 0; i != keyNames.size(); ++i)
            {
                if (strcmp(keyNames[i], text) == 0)
                {
                    return static_cast<ControllerButton>(i);
                }
            }
            return NONE;
        }

        RGBAColor DecodeColorValue(const char *value)
        {
            RGBAColor color{255};
            uint8_t counter = 0;
            int charIndex = 0;
            while (value[charIndex] != '\0')
            {
                if (charIndex == 0)
                    color.values[counter++] = atoi(value + charIndex++);
                if (value[charIndex++] == ',')
                {
                    color.values[counter++] = atoi(value + charIndex);
                    if (counter == 4)
                        break;
                }
            }
            return color;
        }

        int ParseConfigLine(void *dummy, const char *section, const char *name, const char *value)
        {
            if (strncmp(name, "KEY_", 4) == 0)
            {
                ControllerButton button = StringToKey(name + 4);
                ControllerButton buttonValue = StringToKey(value);
                if (button >= 2)
                {
                    tempConfig.buttons[button - 2] = buttonValue;
                    return 1;
                }
            }
            else if (strcmp(name, "left_stick_deadzone") == 0)
            {
                tempConfig.stickDeadzonePercent[0] = atoi(value);
                return 1;
            }
            else if (strcmp(name, "right_stick_deadzone") == 0)
            {
                tempConfig.stickDeadzonePercent[1] = atoi(value);
                return 1;
            }
            else if (strcmp(name, "left_stick_rotation") == 0)
            {
                tempConfig.stickRotationDegrees[0] = atoi(value);
                return 1;
            }
            else if (strcmp(name, "right_stick_rotation") == 0)
            {
                tempConfig.stickRotationDegrees[1] = atoi(value);
                return 1;
            }
            else if (strcmp(name, "left_trigger_deadzone") == 0)
            {
                tempConfig.triggerDeadzonePercent[0] = atoi(value);
                return 1;
            }
            else if (strcmp(name, "right_trigger_deadzone") == 0)
            {
                tempConfig.triggerDeadzonePercent[1] = atoi(value);
                return 1;
            }
            else if (strcmp(name, "swap_dpad_and_lstick") == 0)
            {
                tempConfig.swapDPADandLSTICK = (strcmp(value, "true") ? false : true);
                return 1;
            }
            else if (strcmp(name, "firmware_path") == 0)
            {
                strcpy(firmwarePath, value);
                return 1;
            }
            else if (strncmp(name, "color_", 6) == 0)
            {
                if (strcmp(name + 6, "body") == 0)
                {
                    tempConfig.bodyColor = DecodeColorValue(value);
                    return 1;
                }
                else if (strcmp(name + 6, "buttons") == 0)
                {
                    tempConfig.buttonsColor = DecodeColorValue(value);
                    return 1;
                }
                else if (strcmp(name + 6, "leftGrip") == 0)
                {
                    tempConfig.leftGripColor = DecodeColorValue(value);
                    return 1;
                }
                else if (strcmp(name + 6, "rightGrip") == 0)
                {
                    tempConfig.rightGripColor = DecodeColorValue(value);
                    return 1;
                }
                else if (strcmp(name + 6, "led") == 0)
                {
                    tempColor = DecodeColorValue(value);
                    return 1;
                }
            }

            return 0;
        }

        Result ReadFromConfig(const char *path)
        {
            tempConfig = ControllerConfig{};
            return ini_parse(path, ParseConfigLine, NULL);
        }

        void ConfigChangedCheckThreadFunc(void *arg)
        {
            WriteToLog("Starting config check thread!");
            do
            {
                if (R_SUCCEEDED(waitSingle(filecheckTimerWaiter, UINT64_MAX)))
                {
                    if (config::CheckForFileChanges())
                    {
                        WriteToLog("File check succeeded! Loading configs...");
                        config::LoadAllConfigs();
                    }
                }
            } while (is_config_changed_check_thread_running);
        }
    } // namespace

    void LoadGlobalConfig(const GlobalConfig &config)
    {
        config::globalConfig = config;
    }

    void LoadAllConfigs()
    {
        if (R_SUCCEEDED(ReadFromConfig(GLOBALCONFIG)))
        {
            LoadGlobalConfig(tempGlobalConfig);
        }
        else
            WriteToLog("Failed to read from global config!");

        if (R_SUCCEEDED(ReadFromConfig(XBOXCONFIG)))
        {
            XboxController::LoadConfig(&tempConfig);
        }
        else
            WriteToLog("Failed to read from xbox orig config!");

        if (R_SUCCEEDED(ReadFromConfig(XBOXONECONFIG)))
            XboxOneController::LoadConfig(&tempConfig);
        else
            WriteToLog("Failed to read from xbox one config!");

        if (R_SUCCEEDED(ReadFromConfig(XBOX360CONFIG)))
        {
            Xbox360Controller::LoadConfig(&tempConfig);
            Xbox360WirelessController::LoadConfig(&tempConfig);
        }
        else
            WriteToLog("Failed to read from xbox 360 config!");

        if (R_SUCCEEDED(ReadFromConfig(DUALSHOCK3CONFIG)))
            Dualshock3Controller::LoadConfig(&tempConfig);
        else
            WriteToLog("Failed to read from dualshock 3 config!");

        if (R_SUCCEEDED(ReadFromConfig(DUALSHOCK4CONFIG)))
            Dualshock4Controller::LoadConfig(&tempConfig, tempColor);
        else
            WriteToLog("Failed to read from dualshock 4 config!");
    }

    bool CheckForFileChanges()
    {
        static u64 globalConfigLastModified;
        static u64 xboxConfigLastModified;
        static u64 xbox360ConfigLastModified;
        static u64 xboxOneConfigLastModified;
        static u64 dualshock3ConfigLastModified;
        static u64 dualshock4ConfigLastModified;

        // Maybe this should be called only once when initializing?
        // I left it here in case this would cause issues when ejecting the SD card
        FsFileSystem *fs = fsdevGetDeviceFileSystem("sdmc");

        if (fs == nullptr)
            return false;

        bool filesChanged = false;
        FsTimeStampRaw timestamp;

        if (R_SUCCEEDED(fsFsGetFileTimeStampRaw(fs, GLOBALCONFIG, &timestamp)))
            if (globalConfigLastModified != timestamp.modified)
            {
                globalConfigLastModified = timestamp.modified;
                filesChanged = true;
            }

        if (R_SUCCEEDED(fsFsGetFileTimeStampRaw(fs, XBOXCONFIG, &timestamp)))
            if (xboxConfigLastModified != timestamp.modified)
            {
                xboxConfigLastModified = timestamp.modified;
                filesChanged = true;
            }

        if (R_SUCCEEDED(fsFsGetFileTimeStampRaw(fs, XBOX360CONFIG, &timestamp)))
            if (xbox360ConfigLastModified != timestamp.modified)
            {
                xbox360ConfigLastModified = timestamp.modified;
                filesChanged = true;
            }

        if (R_SUCCEEDED(fsFsGetFileTimeStampRaw(fs, XBOXONECONFIG, &timestamp)))
            if (xboxOneConfigLastModified != timestamp.modified)
            {
                xboxOneConfigLastModified = timestamp.modified;
                filesChanged = true;
            }

        if (R_SUCCEEDED(fsFsGetFileTimeStampRaw(fs, DUALSHOCK3CONFIG, &timestamp)))
            if (dualshock3ConfigLastModified != timestamp.modified)
            {
                dualshock3ConfigLastModified = timestamp.modified;
                filesChanged = true;
            }

        if (R_SUCCEEDED(fsFsGetFileTimeStampRaw(fs, DUALSHOCK4CONFIG, &timestamp)))
            if (dualshock4ConfigLastModified != timestamp.modified)
            {
                dualshock4ConfigLastModified = timestamp.modified;
                filesChanged = true;
            }
        return filesChanged;
    }

    Result Initialize()
    {
        DiscardOldLogs();
        config::LoadAllConfigs();
        config::CheckForFileChanges();
        utimerCreate(&filecheckTimer, 1e+9L, TimerType_Repeating);
        return Enable();
    }

    void Exit()
    {
        Disable();
    }

    Result Enable()
    {
        if (filecheckTimer.started)
            return 1;
        utimerStart(&filecheckTimer);
        is_config_changed_check_thread_running = true;
        return g_config_changed_check_thread.Start().GetValue();
    }

    void Disable()
    {
        is_config_changed_check_thread_running = false;
        utimerStop(&filecheckTimer);
        g_config_changed_check_thread.CancelSynchronization();
        g_config_changed_check_thread.Join();
    }
} // namespace syscon::config