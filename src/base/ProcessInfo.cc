/*
 * @Author: py.wang 
 * @Date: 2019-05-15 08:26:03 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-15 09:23:21
 */

#include "src/base/ProcessInfo.h"
#include "src/base/FileUtil.h"
#include "src/base/CurrentThread.h"

#include <algorithm>

#include <assert.h>
#include <dirent.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>
#include <sys/times.h>

namespace slack
{

namespace detail
{

// 线程局部数据
__thread int t_numOpenedFiles = 0;
int fdDirFilter(const struct dirent *d)
{
    // filename
    if (::isdigit(d->d_name[0]))
    {
        ++t_numOpenedFiles;
    }
    return 0;
}

__thread std::vector<pid_t> *t_pids = nullptr;
int taskDirFilter(const struct dirent *d)
{
    if (::isdigit(d->d_name[0]))
    {
        t_pids->push_back(atoi(d->d_name));
    }
    return 0;
}

int scanDir(const char *dirpath, int (*filter)(const struct dirent *))
{
    struct dirent **namelist = nullptr;
    // 扫描文件夹，过滤、排序
    int result = ::scandir(dirpath, &namelist, filter, alphasort);
    assert(namelist == nullptr);
    return result;
}

Timestamp g_startTime = Timestamp::now();
// assume those won't change during the life time of a process
int g_clockTicks = static_cast<int>(::sysconf(_SC_CLK_TCK));
int g_pageSize = static_cast<int>(::sysconf(_SC_PAGE_SIZE));
}   // namespace detail

}   // namespace slack

using namespace slack;
using namespace slack::detail;

pid_t ProcessInfo::pid()
{
    return ::getpid();
}

slack::string ProcessInfo::pidString()
{
    char buf[32];
    snprintf(buf, sizeof buf, "%d", pid());
    return buf;
}

uid_t ProcessInfo::uid()
{
    return ::getuid();
}

slack::string ProcessInfo::userName()
{
    struct passwd pwd;
    struct passwd *result = nullptr;
    char buf[8192];
    const char *name = "unknownuser";

    getpwuid_r(uid(), &pwd, buf, sizeof buf, &result);
    if (result)
    {
        name = pwd.pw_name;
    }
    return name;
}

// 有效用户
uid_t ProcessInfo::euid()
{
    return ::geteuid();
}

Timestamp ProcessInfo::startTime()
{
    return g_startTime;
}

int ProcessInfo::clockTicksPerSecond()
{
    return g_clockTicks;
}

int ProcessInfo::pageSize()
{
    return g_pageSize;
}

bool ProcessInfo::isDebugBuild()
{
#ifndef NDEBUG
    return false;
#else 
    return true;
#endif
}

slack::string ProcessInfo::hostName()
{
    char buf[256];
    if (::gethostname(buf, sizeof buf) == 0)
    {
        buf[sizeof(buf)-1] = '\0';
        return buf;
    }
    else 
    {
        return "unknownhost";
    }
}

slack::string ProcessInfo::procName()
{
    return procName(procStat()).as_string();
}

StringPiece ProcessInfo::procName(const string &stat)
{
    StringPiece name;
    size_t lp = stat.find('(');
    size_t rp = stat.rfind(')');
    if (lp != string::npos && rp != string::npos && lp < rp)
    {
        name.set(stat.data()+lp+1, static_cast<int>(rp-lp-1));
    }
    return name;
}

slack::string ProcessInfo::procStatus()
{
    string result;
    FileUtil::readFile("/proc/self/status", 65536, &result);
    return result;
}

slack::string ProcessInfo::procStat()
{
    string result;
    FileUtil::readFile("/proc/self/stat", 65536, &result);
    return result;
}

slack::string ProcessInfo::threadStat()
{
    char buf[64];
    snprintf(buf, sizeof buf, "/proc/self/task/%d/stat", CurrentThread::tid());
    string result;
    FileUtil::readFile(buf, 65536, &result);
    return result;
}

slack::string ProcessInfo::exePath()
{
    string result;
    char buf[1024];
    ssize_t n = ::readlink("/proc/self/exe", buf, sizeof buf);
    if (n > 0)
    {
        result.assign(buf, n);
    }
    return result;
}

int ProcessInfo::openedFiles()
{
    t_numOpenedFiles = 0;
    scanDir("/proc/self/fd", fdDirFilter);
    return t_numOpenedFiles;
}

int ProcessInfo::maxOpenFiles()
{
    struct rlimit rl;
    if (::getrlimit(RLIMIT_NOFILE, &rl))
    {
        return openedFiles();
    }
    else 
    {
        // success
        return static_cast<int>(rl.rlim_cur);   // soft limit
    }
}

ProcessInfo::CpuTime ProcessInfo::cpuTime()
{
    ProcessInfo::CpuTime t;
    struct tms tms;
    if (::times(&tms) >= 0)
    {
        const double hz = static_cast<double>(clockTicksPerSecond());
        t.userSeconds = static_cast<double>(tms.tms_utime) / hz;
        t.systemSeconds = static_cast<double>(tms.tms_stime) / hz;
    }
    return t;
}

int ProcessInfo::numThreads()
{
    int result = 0;
    string status = procStatus();
    size_t pos = status.find("Threads:");
    if (pos != string::npos)
    {
        result = ::atoi(status.c_str()+pos+8);  // len("Threads:")==8
    }
    return result;
}

std::vector<pid_t> ProcessInfo::threads()
{
    std::vector<pid_t> result;
    t_pids = &result;
    scanDir("/proc/self/task", taskDirFilter);
    t_pids = nullptr;
    std::sort(result.begin(), result.end());
    return result;
}





