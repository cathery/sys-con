#include "switch.h"
#include "log.h"
#include "config_handler.h"
#include <stratosphere.hpp>

static ams::os::Mutex printMutex(false);

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
        LOG("Deleted previous log file");
    }
}

void WriteToLog(const char *fmt, ...)
{
    std::scoped_lock printLock(printMutex);

    ams::TimeSpan ts = ams::os::ConvertToTimeSpan(ams::os::GetSystemTick());

    FILE *fp = fopen(LOG_PATH, "a");

    //Print time
    fprintf(fp, "%02lid %02li:%02li:%02li: ", ts.GetDays(), ts.GetHours() % 24, ts.GetMinutes() % 60, ts.GetSeconds() % 60);

    //Print the actual text
    va_list va;
    va_start(va, fmt);
    vfprintf(fp, fmt, va);
    va_end(va);

    fclose(fp);
}

void LockedUpdateConsole()
{
    std::scoped_lock printLock(printMutex);
    consoleUpdate(NULL);
}