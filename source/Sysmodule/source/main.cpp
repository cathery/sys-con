#include "switch.h"
#include "log.h"
#include <stratosphere.hpp>

#include "usb_module.h"
#include "controller_handler.h"
#include "config_handler.h"
#include "psc_module.h"
#include "SwitchHDLHandler.h"

extern "C" {
    #include "network.h"
}

#include <sys/stat.h>

#define APP_VERSION "0.6.4"

// libnx fake heap initialization
extern "C"
{
    // We aren't an applet, so disable applet functionality.
    u32 __nx_applet_type = AppletType_None;
    // We are a sysmodule, so don't use more FS sessions than needed.
    u32 __nx_fs_num_sessions = 1;
    // We don't need to reserve memory for fsdev, so don't use it.
    u32 __nx_fsdev_direntry_cache_size = 1;

#define INNER_HEAP_SIZE 0xE7'000
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
    static const SocketInitConfig socketInitConfig = {
        .bsdsockets_version = 1,

        .tcp_tx_buf_size = 0x800,
        .tcp_rx_buf_size = 0x800,
        .tcp_tx_buf_max_size = 0x25000,
        .tcp_rx_buf_max_size = 0x25000,

        //We don't use UDP, set all UDP buffers to 0
        .udp_tx_buf_size = 0,
        .udp_rx_buf_size = 0,

        .sb_efficiency = 1,
    };
    R_ABORT_UNLESS(smInitialize());
    // ams::sm::DoWithSession([]
    {
        //Initialize system firmware version
        R_ABORT_UNLESS(setsysInitialize());
        SetSysFirmwareVersion fw;
        R_ABORT_UNLESS(setsysGetFirmwareVersion(&fw));
        hosversionSet(MAKEHOSVERSION(fw.major, fw.minor, fw.micro));
        setsysExit();


        R_ABORT_UNLESS(hiddbgInitialize());

        R_ABORT_UNLESS(hidInitialize());
        R_ABORT_UNLESS(hidsysInitialize());

        if (hosversionAtLeast(7, 0, 0))
            R_ABORT_UNLESS(hiddbgAttachHdlsWorkBuffer(&SwitchHDLHandler::GetHdlsSessionId()));
        R_ABORT_UNLESS(usbHsInitialize());
        R_ABORT_UNLESS(pscmInitialize());
        R_ABORT_UNLESS(fsInitialize());
        R_ABORT_UNLESS(socketInitialize(&socketInitConfig));
    }
    // );
    smExit();

    R_ABORT_UNLESS(fsdevMountSdmc());
}

extern "C" void __appExit(void)
{
    socketExit();
    hidsysExit();
    hidExit();

    pscmExit();
    usbHsExit();
    hiddbgReleaseHdlsWorkBuffer(SwitchHDLHandler::GetHdlsSessionId());
    hiddbgExit();
    fsdevUnmountAll();
    fsExit();
}

using namespace syscon;

static loop_status_t loop(loop_status_t (*callback)(void))
{
    loop_status_t status = LOOP_CONTINUE;

    while (true)
    {
        svcSleepThread(1e+7);
        status = callback();
        if (status != LOOP_CONTINUE)
            return status;
    }
    return LOOP_EXIT;
}

int main(int argc, char *argv[])
{
    WriteToLog("\n\nNew sysmodule session started on version " APP_VERSION);
    config::Initialize();
    controllers::Initialize();
    usb::Initialize();
    psc::Initialize();

    loop_status_t status = LOOP_RESTART;

    WriteToLog("Going to pre_init");

    network_pre_init();

    WriteToLog("pre_init completed");

    while (status == LOOP_RESTART)
    {
        /* initialize ftp subsystem */
        if (network_init() == 0)
        {
            /* ftp loop */
            status = loop(network_loop);

            /* done with ftp */
            network_exit();
        }
        else
            status = LOOP_EXIT;
    }
    network_post_exit();

    psc::Exit();
    usb::Exit();
    controllers::Exit();
    config::Exit();
}
