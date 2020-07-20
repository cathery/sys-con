#include "psc_module.h"
#include "usb_module.h"
#include "config_handler.h"
#include "controller_handler.h"
#include "log.h"
#include "static_thread.hpp"

namespace syscon::psc
{
    namespace
    {
        PscPmModule pscModule;
        Waiter pscModuleWaiter;
        const uint16_t dependencies[] = {PscPmModuleId_Fs};

        //Thread to check for psc:pm state change (console waking up/going to sleep)
        void PscThreadFunc(void *arg);

        StaticThread<0x1000> g_psc_thread(&PscThreadFunc, nullptr, 0x2C);

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
            } while (g_psc_thread.IsRunning());
        }
    } // namespace
    Result Initialize()
    {
        R_TRY(pscmGetPmModule(&pscModule, PscPmModuleId(126), dependencies, sizeof(dependencies) / sizeof(uint16_t), true));
        pscModuleWaiter = waiterForEvent(&pscModule.event);
        return g_psc_thread.Start();
    }

    void Exit()
    {
        pscPmModuleFinalize(&pscModule);
        pscPmModuleClose(&pscModule);
        eventClose(&pscModule.event);

        g_psc_thread.Join();
    }
}; // namespace syscon::psc