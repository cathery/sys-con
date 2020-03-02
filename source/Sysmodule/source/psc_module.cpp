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

        ams::os::StaticThread<0x2'000> g_psc_thread(&PscThreadFunc, nullptr, 0x2C);

        bool is_psc_thread_running = false;

        void PscThreadFunc(void *arg)
        {
            WriteToLog("Starting PSC thread!");
            do {
                if (R_SUCCEEDED(waitSingle(pscModuleWaiter, U64_MAX)))
                {
                    PscPmState pscState;
                    u32 out_flags;
                    if (R_SUCCEEDED(pscPmModuleGetRequest(&pscModule, &pscState, &out_flags)))
                    {
                        switch (pscState)
                        {
                            case PscPmState_ReadyAwaken:
                                WriteToLog("Switch is awake! Enabling usb events...");
                                usb::CreateUsbEvents();
                                break;
                            case PscPmState_ReadySleep:
                            case PscPmState_ReadyShutdown:
                                WriteToLog("Ready to sleep! Disabling usb events...");
                                usb::DestroyUsbEvents();
                                break;
                            default:
                                break;
                        }
                        pscPmModuleAcknowledge(&pscModule, pscState);
                    }
                }
            } while (is_psc_thread_running);
        }
    }
    Result Initialize()
    {
        R_TRY(pscmGetPmModule(&pscModule, static_cast<PscPmModuleId>(126), dependencies, sizeof(dependencies) / sizeof(uint16_t), true));
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
};