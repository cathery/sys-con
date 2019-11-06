#pragma once
#include <iomanip>
#include <fstream>
#include <iostream>
#include "configFile.h"

template <typename... T>
void WriteToLog(T &&... text)
{
#ifdef __APPLET__

    std::stringstream ss;
    ((ss << text), ...);
    printf(ss.str().c_str());
    printf("\n");

#else

    using namespace std;

    time_t unixTime = time(NULL);
    struct tm *time = localtime((const time_t *)&unixTime);

    fstream fs;
    fs.open(CONFIG_PATH "log.txt", fstream::app);

    //Print time
    fs << setfill('0');
    fs << setw(4) << (time->tm_year + 1900)
       << "-" << setw(2) << time->tm_mon
       << "-" << setw(2) << time->tm_mday
       << " " << setw(2) << time->tm_hour
       << ":" << setw(2) << time->tm_min
       << ":" << setw(2) << time->tm_sec << ": ";
    //Print the actual text
    ((fs << text), ...);
    fs << "\n";
    fs.close();

#endif
}