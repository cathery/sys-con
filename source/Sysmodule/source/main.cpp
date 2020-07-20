#include "switch.h"
#include "log.h"
#include "utils.hpp"

#include "usb_module.h"
#include "controller_handler.h"
#include "config_handler.h"
#include "psc_module.h"

#define APP_VERSION "0.6.2"

// libnx fake heap initialization
extern "C"
{
    // We aren't an applet, so disable applet functionality.
    u32 __nx_applet_type = AppletType_None;
    // We are a sysmodule, so don't use more FS sessions than needed.
    u32 __nx_fs_num_sessions = 1;
    // We don't need to reserve memory for fsdev, so don't use it.
    u32 __nx_fsdev_direntry_cache_size = 1;

#define INNER_HEAP_SIZE 0x40'000
    size_t nx_inner_heap_size = INNER_HEAP_SIZE;
    char nx_inner_heap[INNER_HEAP_SIZE];

    void __libnx_initheap(void)
    {
        // Newlib
        extern char *fake_heap_start;
        extern char *fake_heap_end;

        fake_heap_start = nx_inner_heap;
        fake_heap_end = nx_inner_heap + nx_inner_heap_size;
    }
}

extern "C" void __appInit(void)
{
    util::DoWithSmSession([] {
        util::SetHOSVersion();
        R_ASSERT(hiddbgInitialize());
        if (hosversionAtLeast(7, 0, 0))
            R_ASSERT(hiddbgAttachHdlsWorkBuffer());
        R_ASSERT(usbHsInitialize());
        R_ASSERT(pscmInitialize());
        R_ASSERT(fsInitialize());
    });

    R_ASSERT(fsdevMountSdmc());
}

extern "C" void __appExit(void)
{
    pscmExit();
    usbHsExit();
    hiddbgReleaseHdlsWorkBuffer();
    hiddbgExit();
    fsdevUnmountAll();
    fsExit();
}

using namespace syscon;

int main(int argc, char *argv[])
{
    WriteToLog("\n\nNew sysmodule session started on version " APP_VERSION);
    config::Initialize();
    controllers::Initialize();
    usb::Initialize();
    psc::Initialize();

    while (true)
    {
        svcSleepThread(1e+8L);
    }

    psc::Exit();
    usb::Exit();
    controllers::Exit();
    config::Exit();
}
