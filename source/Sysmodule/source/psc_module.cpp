#include "psc_module.h"
#include <stratosphere.hpp>
#include "usb_module.h"
#include "config_handler.h"
#include "controller_handler.h"
#include "log.h"

namespace syscon::psc
{
    namespace
    {
        PscPmModule pscModule;
        Waiter pscModuleWaiter;
        const uint32_t dependencies[] = {PscPmModuleId_Fs};

        //Thread to check for psc:pm state change (console waking up/going to sleep)
        void PscThreadFunc(void *arg);

        alignas(ams::os::ThreadStackAlignment) u8 psc_thread_stack[0x1000];
        Thread g_psc_thread;

        bool is_psc_thread_running = false;

        void PscThreadFunc(void *arg)
        {
            do
            {
                if (R_SUCCEEDED(waitSingle(pscModuleWaiter, UINT64_MAX)))
                {
                    PscPmState pscState;
                    u32 out_flags;
                    if (R_SUCCEEDED(pscPmModuleGetRequest(&pscModule, &pscState, &out_flags)))
                    {
                        switch (pscState)
                        {
                            case PscPmState_Awake:
                            case PscPmState_ReadyAwaken:
                                //usb::CreateUsbEvents();
                                break;
                            case PscPmState_ReadySleep:
                            case PscPmState_ReadyShutdown:
                                //usb::DestroyUsbEvents();
                                controllers::Reset();
                                break;
                            default:
                                break;
                        }
                        pscPmModuleAcknowledge(&pscModule, pscState);
                    }
                }
            } while (is_psc_thread_running);
        }
    } // namespace
    Result Initialize()
    {
        R_TRY(pscmGetPmModule(&pscModule, PscPmModuleId(126), dependencies, sizeof(dependencies) / sizeof(uint32_t), true));
        pscModuleWaiter = waiterForEvent(&pscModule.event);
        is_psc_thread_running = true;
        R_ABORT_UNLESS(threadCreate(&g_psc_thread, &PscThreadFunc, nullptr, psc_thread_stack, sizeof(psc_thread_stack), 0x2C, -2));
        R_ABORT_UNLESS(threadStart(&g_psc_thread));
        return 0;
    }

    void Exit()
    {
        is_psc_thread_running = false;

        pscPmModuleFinalize(&pscModule);
        pscPmModuleClose(&pscModule);
        eventClose(&pscModule.event);

        svcCancelSynchronization(g_psc_thread.handle);
        threadWaitForExit(&g_psc_thread);
        threadClose(&g_psc_thread);
    }
}; // namespace syscon::psc