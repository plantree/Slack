/*
 * @Author: py.wang 
 * @Date: 2019-05-15 08:22:42 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-15 09:07:29
 */

#ifndef MUDUO_BASE_PROCESSINFO_H
#define MUDUO_BASE_PROCESSINFO_H

#include <muduo/base/Types.h>
#include <muduo/base/Timestamp.h>
#include <vector>
#include <pthread.h>

namespace muduo
{

namespace ProcessInfo
{
    pid_t pid();
    string pidString();
    uid_t uid();
    string username();
    uid_t euid();
    Timestamp startTime();
    
    string hostname();

    // read /proc/self/status
    string procStatus();

    int openedFiles();
    int maxOpenFiles();

    int numThreads();
    std::vector<pid_t> threads();
    
}   // namespace ProcessInfo

}   // namespace muduo

#endif // MUDUO_BASE_PROCESSINFO_H