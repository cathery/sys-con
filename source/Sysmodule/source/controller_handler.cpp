#include "switch.h"
#include "controller_handler.h"
#include "SwitchHDLHandler.h"
#include "SwitchAbstractedPadHandler.h"
#include <algorithm>
#include <functional>
#include "SwitchUtils.h"

#include "log.h"

namespace syscon::controllers
{
    namespace
    {
        constexpr size_t MaxControllerHandlersSize = 10;
        std::vector<std::unique_ptr<SwitchVirtualGamepadHandler>> controllerHandlers;
        bool UseAbstractedPad;
        Mutex controllerMutex = 0;
    } // namespace

    bool IsAtControllerLimit()
    {
        return controllerHandlers.size() >= MaxControllerHandlersSize;
    }

    Result Insert(std::unique_ptr<IController>&& controllerPtr)
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
            SwitchUtils::ScopedLock scoped_lock(controllerMutex);
            controllerHandlers.push_back(std::move(switchHandler));
        }

        return rc;
    }

    std::vector<std::unique_ptr<SwitchVirtualGamepadHandler>>& Get()
    {
        return controllerHandlers;
    }

    Mutex& GetScopedLock()
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
        controllerHandlers.reserve(MaxControllerHandlersSize);
    }

    void Reset()
    {
        SwitchUtils::ScopedLock scoped_lock(controllerMutex);
        controllerHandlers.clear();
    }

    void Exit()
    {
        Reset();
    }
} // namespace syscon::controllers