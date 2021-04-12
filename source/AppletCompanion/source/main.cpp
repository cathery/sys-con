#include "switch.h"
#include <stdio.h>

int main()
{
    consoleInit(NULL);

    padConfigureInput(8, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeAny(&pad);
    hidSetNpadHandheldActivationMode(HidNpadHandheldActivationMode_Single);

    printf("Hello\n");

    while (appletMainLoop())
    {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus || kDown & HidNpadButton_B)
            break;
        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}