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
        const uint16_t dependencies[] = {PscPmModuleId_Usb};

        void PscThreadFunc(void *arg);

        ams::os::StaticThread<0x1'000> g_psc_thread(&PscThreadFunc, nullptr, 0x2C);

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
        R_TRY(pscmGetPmModule(&pscModule, PscPmModuleId(126), dependencies, sizeof(dependencies) / sizeof(uint16_t), true));
        pscModuleWaiter = waiterForEvent(&pscModule.event);
        is_psc_thread_running = true;
        return g_psc_thread.Start().GetValue();
    }

    void Exit()
    {
        is_psc_thread_running = false;

        pscPmModuleFinalize(&pscModule);
        pscPmModuleClose(&pscModule);
        eventClose(&pscModule.event);

        g_psc_thread.CancelSynchronization();
        g_psc_thread.Join();
    }
}; // namespace syscon::psc