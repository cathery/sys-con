#include "switch.h"
#include "log.h"
#include "mainLoop.h"

//ISSUES:
// when exiting the applet, only one of the controllers is reset
// Rumble is currently missing on all controllers
// Kosmos Toolbox doesn't allow this sysmodule to be turned on after turning it off, probably due to heap memory not being freed up

//TODO:
// Shrink unneessary heap memory/stack size used for the sysmodule
// Allow to connect controllers paired through a bluetooth adapter
// Allow to connect controllers through usbDs (directly to switch)
// Make a config application companion to test controller input and edit various preferences (buttons, deadzones)

extern "C"
{
// Adjust size as needed.
#define INNER_HEAP_SIZE 0x40000
#ifndef __APPLET__

    u32 __nx_applet_type = AppletType_None;
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void)
    {
        void *addr = nx_inner_heap;
        size_t size = nx_inner_heap_size;

        // Newlib
        extern char *fake_heap_start;
        extern char *fake_heap_end;

        fake_heap_start = (char *)addr;
        fake_heap_end = (char *)addr + size;
    }

#endif

    void __attribute__((weak)) userAppInit(void)
    {
        //Seems like every thread on the switch needs to sleep for a little
        // or it will block the entire console
        //Specifically in Kosmos Toolbox's case, you need to wait about 0.2 sec
        // or it won't let you turn it on/off the sysmodule after a few tries
        svcSleepThread(2e+8L);
        Result rc = 0;
        rc = hiddbgInitialize();
        if (R_FAILED(rc))
            fatalThrow(rc);

        rc = hiddbgAttachHdlsWorkBuffer();
        if (R_FAILED(rc))
            fatalThrow(rc);

        rc = usbHsInitialize();
        if (R_FAILED(rc))
            fatalThrow(rc);

#ifndef __APPLET__
        rc = hidInitialize();
        if (R_FAILED(rc))
            fatalThrow(rc);
#endif
    }

    void __attribute__((weak)) userAppExit(void)
    {
#ifndef __APPLET__
        hidExit();
#endif
        usbHsExit();
        hiddbgReleaseHdlsWorkBuffer();
        hiddbgExit();
    }
}

int main(int argc, char *argv[])
{
    Result rc;

#ifdef __APPLET__
    appletLockExit();
    consoleInit(nullptr);
#endif

    rc = mainLoop();

#ifdef __APPLET__
    consoleExit(nullptr);
    userAppExit();
    appletUnlockExit();
#endif

    return rc;
}
