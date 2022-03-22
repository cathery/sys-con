#pragma once

#define CONFIG_PATH "/config/sys-con/"
#define LOG_PATH CONFIG_PATH "log.txt"

#ifdef __cplusplus
extern "C"
{
#endif
    void DiscardOldLogs();

    void WriteToLog(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

    void LockedUpdateConsole();

#ifdef __cplusplus
}
#endif
