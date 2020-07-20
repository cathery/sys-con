#pragma once

#include "ControllerHelpers.h"
#include "SwitchVirtualGamepadHandler.h"
#include "scoped_mutex.hpp"
#include "static_vector.hpp"

namespace syscon::controllers
{
    constexpr size_t MaxControllerHandlerSize = 10;
    typedef StaticVector<std::unique_ptr<SwitchVirtualGamepadHandler>, MaxControllerHandlerSize> ControllersVector;

    bool IsAtControllerLimit();
    Result Insert(std::unique_ptr<IController> &&controllerPtr);
    ControllersVector &Get();
    ScopedMutex &GetScopedLock();

    //void Remove(void Remove(bool (*func)(std::unique_ptr<SwitchVirtualGamepadHandler> a)));;

    void Initialize();
    void Reset();
    void Exit();
} // namespace syscon::controllers