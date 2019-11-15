#pragma once
#include "switch/result.h"

struct GlobalConfig
{
    uint16_t dualshock4_productID;
};

Result mainLoop();

void LoadGlobalConfig(const GlobalConfig *config);