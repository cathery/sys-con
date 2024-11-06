#include "switch.h"
#include "log.h"
#include "SwitchUtils.h"

#include "usb_module.h"
#include "controller_handler.h"
#include "config_handler.h"
#include "psc_module.h"
#include "SwitchHDLHandler.h"

#define APP_VERSION "0.6.5"

// Size of the inner heap (adjust as necessary).
#define INNER_HEAP_SIZE 0x40'000

extern "C"
{
    // Sysmodules should not use applet*.
    u32 __nx_applet_type = AppletType_None;

    // Sysmodules will normally only want to use one FS session.
    u32 __nx_fs_num_sessions = 1;

    // Newlib heap configuration function (makes malloc/free work).
    void __libnx_initheap(void)
    {
        static u8 inner_heap[INNER_HEAP_SIZE];
        extern void* fake_heap_start;
        extern void* fake_heap_end;

        // Configure the newlib heap.
        fake_heap_start = inner_heap;
        fake_heap_end   = inner_heap + sizeof(inner_heap);
    }
}

void* workmem = nullptr;
size_t workmem_size = 0x1000;

extern "C" void __appInit(void)
{
    R_ABORT_UNLESS(smInitialize());
    // ams::sm::DoWithSession([]
    {
        // Initialize system firmware version
        R_ABORT_UNLESS(setsysInitialize());
        SetSysFirmwareVersion fw;
        R_ABORT_UNLESS(setsysGetFirmwareVersion(&fw));
        hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();

        R_ABORT_UNLESS(hiddbgInitialize());
        if (hosversionAtLeast(7, 0, 0))
        {
            workmem = aligned_alloc(0x1000, workmem_size);

            if (!workmem)
            {
                diagAbortWithResult(MAKERESULT(Module_Libnx, LibnxError_InitFail_HID));
            }

            R_ABORT_UNLESS(hiddbgAttachHdlsWorkBuffer(&SwitchHDLHandler::GetHdlsSessionId(), workmem, workmem_size));
        }
        R_ABORT_UNLESS(usbHsInitialize());
        R_ABORT_UNLESS(pscmInitialize());
        R_ABORT_UNLESS(fsInitialize());
    }
    // );
    smExit();

    R_ABORT_UNLESS(fsdevMountSdmc());
}

extern "C" void __appExit(void)
{
    pscmExit();
    usbHsExit();
    hiddbgReleaseHdlsWorkBuffer(SwitchHDLHandler::GetHdlsSessionId());
    hiddbgExit();
    fsdevUnmountAll();
    fsExit();
    free(workmem);
}

using namespace syscon;

int main(int argc, char* argv[])
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
