#include "switch.h"
#include "log.h"
#include "configFile.h"
#include <stratosphere.hpp>

static Mutex g_PrintMutex = 0;

void WriteToLog(const char *fmt, ...)
{
    mutexLock(&g_PrintMutex);

    u64 ts;
    TimeCalendarTime caltime;
    timeGetCurrentTime(TimeType_LocalSystemClock, &ts);
    timeToCalendarTimeWithMyRule(ts, &caltime, nullptr);

    FILE *fp = fopen(CONFIG_PATH "log.txt", "a");

    //Print time
    fprintf(fp, "%04i-%02i-%02i %02i:%02i:%02i: ", caltime.year, caltime.month, caltime.day, caltime.hour, caltime.minute, caltime.second);

    //Print the actual text
    va_list va;
    va_start(va, fmt);
    vfprintf(fp, fmt, va);
    va_end(va);

    fprintf(fp, "\n");
    fclose(fp);

    mutexUnlock(&g_PrintMutex);
}

void LockedUpdateConsole()
{
    mutexLock(&g_PrintMutex);
    consoleUpdate(NULL);
    mutexUnlock(&g_PrintMutex);
}