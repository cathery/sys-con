#include "led.h"
#include <string.h>
#include <switch.h>

#include "util.h"

void flash_led_connect()
{
    HidsysNotificationLedPattern pattern;
    memset(&pattern, 0, sizeof(pattern));

    // Setup Breathing effect pattern data.
    pattern.baseMiniCycleDuration = 0x8; // 100ms.
    pattern.totalMiniCycles = 0x2;       // 3 mini cycles. Last one 12.5ms.
    pattern.totalFullCycles = 0x0;       // Repeat forever.
    pattern.startIntensity = 0x2;        // 13%.

    pattern.miniCycles[0].ledIntensity = 0xF;      // 100%.
    pattern.miniCycles[0].transitionSteps = 0xF;   // 15 steps. Transition time 1.5s.
    pattern.miniCycles[0].finalStepDuration = 0x0; // Forced 12.5ms.
    pattern.miniCycles[1].ledIntensity = 0x2;      // 13%.
    pattern.miniCycles[1].transitionSteps = 0xF;   // 15 steps. Transition time 1.5s.
    pattern.miniCycles[1].finalStepDuration = 0x0; // Forced 12.5ms.

    u64 uniquePadIds[5] = {0};

    s32 total_entries = 0;

    Result rc = hidsysGetUniquePadIds(uniquePadIds, 5, &total_entries);
    if (R_FAILED(rc) && rc != MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer))
        fatalThrow(rc);

    for (int i = 0; i < total_entries; i++)
    {
        hidsysSetNotificationLedPattern(&pattern, uniquePadIds[i]);
        hidsysSetNotificationLedPatternWithTimeout(&pattern, uniquePadIds[i], LED_TIMEOUT);
    }
}

void flash_led_disconnect()
{
    HidsysNotificationLedPattern pattern;
    memset(&pattern, 0, sizeof(pattern));

    u64 uniquePadIds[2];
    memset(uniquePadIds, 0, sizeof(uniquePadIds));

    s32 total_entries = 0;

    Result rc = hidsysGetUniquePadsFromNpad(hidGetHandheldMode() ? CONTROLLER_HANDHELD : CONTROLLER_PLAYER_1, uniquePadIds, 2, &total_entries);
    if (R_FAILED(rc) && rc != MAKERESULT(Module_Libnx, LibnxError_IncompatSysVer))
        fatalThrow(rc);

    for (int i = 0; i < total_entries; i++)
        hidsysSetNotificationLedPattern(&pattern, uniquePadIds[i]);
}
