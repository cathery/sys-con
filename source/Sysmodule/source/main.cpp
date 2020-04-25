#include "switch.h"
#include "log.h"
#include <stratosphere.hpp>

#include "usb_module.h"
#include "controller_handler.h"
#include "config_handler.h"
#include "psc_module.h"

#define APP_VERSION "0.6.1"

// libnx fake heap initialization
extern "C"
{
    u32 __nx_applet_type = AppletType_None;

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

    // Exception handling
    alignas(16) u8 __nx_exception_stack[ams::os::MemoryPageSize];
    u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);
    void __libnx_exception_handler(ThreadExceptionDump *ctx)
    {
        ams::CrashHandler(ctx);
    }
}

// libstratosphere variables
namespace ams
{
    ncm::ProgramId CurrentProgramId = {0x690000000000000D};
    namespace result
    {
        bool CallFatalOnResultAssertion = true;
    }
} // namespace ams

extern "C" void __appInit(void)
{
    ams::sm::DoWithSession([] {
        //Initialize system firmware version
        R_ABORT_UNLESS(setsysInitialize());
        SetSysFirmwareVersion fw;
        R_ABORT_UNLESS(setsysGetFirmwareVersion(&fw));
        hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();

        R_ABORT_UNLESS(timeInitialize());
        R_ABORT_UNLESS(hiddbgInitialize());
        if (hosversionAtLeast(7, 0, 0))
            R_ABORT_UNLESS(hiddbgAttachHdlsWorkBuffer());
        R_ABORT_UNLESS(usbHsInitialize());
        R_ABORT_UNLESS(pscmInitialize());
        R_ABORT_UNLESS(fsInitialize());
    });

    R_ABORT_UNLESS(fsdevMountSdmc());
}

extern "C" void __appExit(void)
{
    pscmExit();
    usbHsExit();
    hiddbgReleaseHdlsWorkBuffer();
    hiddbgExit();
    fsdevUnmountAll();
    fsExit();
    timeExit();
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
