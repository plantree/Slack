/*
 * @Author: py.wang 
 * @Date: 2019-05-15 08:22:42 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-15 09:07:29
 */

#ifndef BASE_PROCESSINFO_H
#define BASE_PROCESSINFO_H

#include "src/base/Types.h"
#include "src/base/Timestamp.h"
#include "src/base/StringPiece.h"

#include <vector>

#include <pthread.h>

namespace slack
{

namespace ProcessInfo
{
    pid_t pid();
    string pidString();
    uid_t uid();
    string userName();
    uid_t euid();
    Timestamp startTime();
    int clockTicksPerSecond();
    int pageSize();
    bool isDebugBuild();    // constexpr
    
    string hostName();
    string procName();
    StringPiece procName(const string &stat);

    // read /proc/self/status
    string procStatus();

    // read /proc/self/stat
    string procStat();

    // read /proc/self/task/tid/stat
    string threadStat();

    // readlink /proc/self/exe
    string exePath();

    int openedFiles();
    int maxOpenFiles();

    struct CpuTime
    {
        double userSeconds;
        double systemSeconds;

        CpuTime() : userSeconds(0.0), systemSeconds(0.0) { }

        double total() const 
        {
            return userSeconds + systemSeconds;
        }
    };

    CpuTime cpuTime();

    int numThreads();
    std::vector<pid_t> threads();
    
}   // namespace ProcessInfo

}   // namespace slack

#endif // BASE_PROCESSINFO_H