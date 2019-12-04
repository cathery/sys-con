#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    void WriteToLog(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

    void LockedUpdateConsole();

#ifdef __cplusplus
}
#endif
