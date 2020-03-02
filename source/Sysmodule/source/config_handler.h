#pragma once
#include "ControllerTypes.h"

#define CONFIG_PATH "/config/sys-con/"

#define GLOBALCONFIG CONFIG_PATH "config_global.ini"
#define XBOXCONFIG CONFIG_PATH "config_xboxorig.ini"
#define XBOX360CONFIG CONFIG_PATH "config_xbox360.ini"
#define XBOXONECONFIG CONFIG_PATH "config_xboxone.ini"
#define DUALSHOCK3CONFIG CONFIG_PATH "config_dualshock3.ini"
#define DUALSHOCK4CONFIG CONFIG_PATH "config_dualshock4.ini"

namespace syscon::config
{
    struct GlobalConfig
    {
        uint16_t dualshock4_productID;
    };

    inline GlobalConfig globalConfig
    {
        .dualshock4_productID = PRODUCT_DUALSHOCK4_1X,
    };

    void LoadGlobalConfig(const GlobalConfig &config);
    void LoadAllConfigs();
    bool CheckForFileChanges();

    Result Initialize();
    void Exit();

    Result Enable();
    void Disable();
};