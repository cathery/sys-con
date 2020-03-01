#include "switch.h"
#include "log.h"
#include <stratosphere.hpp>

#include "usb_module.h"
#include "controller_handler.h"
#include "config_handler.h"

#define APP_VERSION "0.6.0"

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
    namespace result { bool CallFatalOnResultAssertion = false; }
}


extern "C" void __appInit(void)
{
    ams::hos::SetVersionForLibnx();
    ams::sm::DoWithSession([] 
    {
        R_ASSERT(timeInitialize());
        R_ASSERT(hiddbgInitialize());
        if (ams::hos::GetVersion() >= ams::hos::Version_700)
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
    timeExit();
}

using namespace syscon;

int main(int argc, char *argv[])
{
    WriteToLog("\n\nNew sysmodule session started on version " APP_VERSION);
    config::Initialize();
    handler::Initialize();
    usb::Initialize();

    while (true)
    {
        svcSleepThread(1e+8L);
    }

    usb::Exit();
    handler::Exit();
    config::Exit();
}
