#include "switch.h"
#include "log.h"
#include "configFile.h"
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static Mutex g_PrintMutex = 0;

void WriteToLog(const char *fmt, ...)
{
    mutexLock(&g_PrintMutex);

#ifdef __APPLET__
    va_list va;
    va_start(va, fmt);
    vprintf(fmt, va);
    printf("\n");
    va_end(va);

#else

    time_t unixTime = time(NULL);
    struct tm tStruct;
    localtime_r(&unixTime, &tStruct);

    FILE *fp = fopen(CONFIG_PATH "log.txt", "a");

    //Print time
    fprintf(fp, "%04i-%02i-%02i %02i:%02i:%02i: ", (tStruct.tm_year + 1900), tStruct.tm_mon, tStruct.tm_mday, tStruct.tm_hour, tStruct.tm_min, tStruct.tm_sec);

    //Print the actual text
    va_list va;
    va_start(va, fmt);
    vfprintf(fp, fmt, va);
    va_end(va);

    fprintf(fp, "\n");
    fclose(fp);
#endif

    mutexUnlock(&g_PrintMutex);
}

void LockedUpdateConsole()
{
    mutexLock(&g_PrintMutex);
    consoleUpdate(NULL);
    mutexUnlock(&g_PrintMutex);
}