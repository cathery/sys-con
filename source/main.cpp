#include "switch.h"
#include "log.h"
#include "mainLoop.h"

//CHANGELOG
// Fixed an issue where unplugging one controller would unplug others

//ISSUES:
// Kosmos Toolbox doesn't free up the memory associated with the sysmodule due to hiddbgAttachHdlsWorkBuffer() memory not being freed up
// After plugging and unplugging a controller for a while, sysmodule stops working
// DS3 controller seems to send random inputs

//TODO:
// Shrink unneessary heap memory/stack size used for the sysmodule
// Make a config application companion to test controller input and edit various preferences (buttons, deadzones)

extern "C"
{
// Adjust size as needed.
#define INNER_HEAP_SIZE 0x40'000
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

    void userAppInit(void)
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

        if (hosversionAtLeast(7, 0, 0))
        {
            rc = hiddbgAttachHdlsWorkBuffer();
            if (R_FAILED(rc))
                fatalThrow(rc);
        }

        rc = usbHsInitialize();
        if (R_FAILED(rc))
            fatalThrow(rc);

        rc = pscmInitialize();
        if (R_FAILED(rc))
            fatalThrow(rc);

#ifndef __APPLET__
        rc = hidInitialize();
        if (R_FAILED(rc))
            fatalThrow(rc);
#endif
    }

    void userAppExit(void)
    {
#ifndef __APPLET__
        hidExit();
#endif
        pscmExit();
        usbHsExit();
        hiddbgReleaseHdlsWorkBuffer();
        hiddbgExit();
    }

    alignas(16) u8 __nx_exception_stack[0x1000];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    __attribute__((weak)) u32 __nx_exception_ignoredebug = 1;

    void __libnx_exception_handler(ThreadExceptionDump *ctx)
    {
        WriteToLog("Sysmodule crashed with error 0x%x", ctx->error_desc);
        LockedUpdateConsole();
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
