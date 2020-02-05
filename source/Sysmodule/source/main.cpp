#include "switch.h"
#include "log.h"
#include "mainLoop.h"
#include <stratosphere.hpp>

// libnx fake heap initialization
extern "C"
{
    u32 __nx_applet_type = AppletType_None;

    #define INNER_HEAP_SIZE 0x40 * ams::os::MemoryPageSize
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
        R_ASSERT(fsInitialize());
        R_ASSERT(fsdevMountSdmc());
        R_ASSERT(hiddbgInitialize());
        if (hosversionAtLeast(7, 0, 0))
            R_ASSERT(hiddbgAttachHdlsWorkBuffer());
        R_ASSERT(usbHsInitialize());
        R_ASSERT(pscmInitialize());
    });
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

int main(int argc, char *argv[])
{
    return mainLoop();
}
