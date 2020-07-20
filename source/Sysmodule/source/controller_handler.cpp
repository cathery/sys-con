#include "switch.h"
#include "controller_handler.h"
#include "SwitchHDLHandler.h"
#include "SwitchAbstractedPadHandler.h"
#include <algorithm>
#include <functional>
#include "log.h"
#include "static_vector.hpp"

namespace syscon::controllers
{
    namespace
    {
        ControllersVector controllerHandlers;
        bool UseAbstractedPad;
        ScopedMutex controllerMutex;
    } // namespace

    bool IsAtControllerLimit()
    {
        return controllerHandlers.size() >= controllerHandlers.max_size();
    }

    Result Insert(std::unique_ptr<IController> &&controllerPtr)
    {
        std::unique_ptr<SwitchVirtualGamepadHandler> switchHandler;
        if (UseAbstractedPad)
        {
            switchHandler = std::make_unique<SwitchAbstractedPadHandler>(std::move(controllerPtr));
            WriteToLog("Inserting controller as abstracted pad");
        }
        else
        {
            switchHandler = std::make_unique<SwitchHDLHandler>(std::move(controllerPtr));
            WriteToLog("Inserting controller as HDLs");
        }

        Result rc = switchHandler->Initialize();
        if (R_SUCCEEDED(rc))
        {
            SCOPED_LOCK(controllerMutex);
            controllerHandlers.push_back(std::move(switchHandler));
        }

        return rc;
    }

    ControllersVector &Get()
    {
        return controllerHandlers;
    }

    ScopedMutex &GetScopedLock()
    {
        return controllerMutex;
    }
    /*
    void Remove(std::function func)
    {
        std::remove_if(controllerHandlers.begin(), controllerHandlers.end(), func);
    }
    */

    void Initialize()
    {
        UseAbstractedPad = hosversionBetween(5, 7);
    }

    void Reset()
    {
        SCOPED_LOCK(controllerMutex);
        controllerHandlers.clear();
    }

    void Exit()
    {
        Reset();
    }
} // namespace syscon::controllers