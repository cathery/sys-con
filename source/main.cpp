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
#define INNER_HEAP_SIZE 0x100000
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

        rc = usbCommsInitialize();
        if (R_FAILED(rc))
            fatalThrow(rc);
    }

    void __attribute__((weak)) userAppExit(void)
    {
        usbCommsExit();
        usbHsExit();
        hiddbgReleaseHdlsWorkBuffer();
        hiddbgExit();
    }

    alignas(16) u8 __nx_exception_stack[0x1000];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    __attribute__((weak)) u32 __nx_exception_ignoredebug = 1;

    void __libnx_exception_handler(ThreadExceptionDump *ctx)
    {
        WriteToLog("Sysmodule crashed with error ", ctx->error_desc);
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
