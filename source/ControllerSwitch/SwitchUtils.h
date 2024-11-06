#pragma once
#include "switch.h"

namespace SwitchUtils
{
    constexpr size_t ThreadStackAlignment = 0x1000;
    constexpr size_t MemoryPageSize = 0x1000;

    class ScopedLock
    {
    public:
        [[nodiscard]]
        explicit ScopedLock(Mutex& In) : mutex(In)
        {
            mutexLock(&mutex);
        }

        ~ScopedLock()
        {
            mutexUnlock(&mutex);
        }

        ScopedLock(const ScopedLock&) = delete;
        ScopedLock& operator=(const ScopedLock&) = delete;

    private:
        Mutex& mutex;
    };

#ifndef R_ABORT_UNLESS
    #define R_ABORT_UNLESS(rc)             \
        {                                  \
            if (R_FAILED(rc)) [[unlikely]] \
            {                              \
                diagAbortWithResult(rc);   \
            }                              \
        }
#endif

#ifndef R_TRY
    #define R_TRY(rc)         \
        {                     \
            if (R_FAILED(rc)) \
            {                 \
                return rc;    \
            }                 \
        }
#endif

} // namespace SwitchUtils
