#include "switch.h"
#include "log.h"
#include "config_handler.h"
#include <stratosphere.hpp>

static ams::os::Mutex printMutex;

void DiscardOldLogs()
{
    std::scoped_lock printLock(printMutex);

    FsFileSystem *fs = fsdevGetDeviceFileSystem("sdmc");
    FsFile file;
    s64 fileSize;

    Result rc = fsFsOpenFile(fs, LOG_PATH, FsOpenMode_Read, &file);
    if (R_FAILED(rc))
        return;

    rc = fsFileGetSize(&file, &fileSize);
    fsFileClose(&file);
    if (R_FAILED(rc))
        return;

    if (fileSize >= 0x20'000)
    {
        fsFsDeleteFile(fs, LOG_PATH);
        WriteToLog("Deleted previous log file");
    }
}

void WriteToLog(const char *fmt, ...)
{
    std::scoped_lock printLock(printMutex);

    u64 ts;
    TimeCalendarTime caltime;
    timeGetCurrentTime(TimeType_LocalSystemClock, &ts);
    timeToCalendarTimeWithMyRule(ts, &caltime, nullptr);

    FILE *fp = fopen(LOG_PATH, "a");

    //Print time
    fprintf(fp, "%04i-%02i-%02i %02i:%02i:%02i: ", caltime.year, caltime.month, caltime.day, caltime.hour, caltime.minute, caltime.second);

    //Print the actual text
    va_list va;
    va_start(va, fmt);
    vfprintf(fp, fmt, va);
    va_end(va);

    fprintf(fp, "\n");
    fclose(fp);
}

void LockedUpdateConsole()
{
    std::scoped_lock printLock(printMutex);
    consoleUpdate(NULL);
}