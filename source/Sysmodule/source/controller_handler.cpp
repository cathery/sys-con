#include "switch.h"
#include "controller_handler.h"
#include "SwitchHDLHandler.h"
#include "SwitchAbstractedPadHandler.h"
#include <algorithm>
#include <functional>

#include "log.h"

namespace syscon::controllers
{
    namespace 
    {
        constexpr size_t MaxControllerHandlersSize = 10;
        std::vector<std::unique_ptr<SwitchVirtualGamepadHandler>> controllerHandlers;
        bool UseAbstractedPad;
    }

    bool IsAtControllerLimit()
    {
        return controllerHandlers.size() >= MaxControllerHandlersSize;
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
            controllerHandlers.push_back(std::move(switchHandler));

        return rc;
    }

    std::vector<std::unique_ptr<SwitchVirtualGamepadHandler>>& Get()
    {
        return controllerHandlers;
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
        controllerHandlers.clear();
    }

    void Exit()
    {
        Reset();
    }
}