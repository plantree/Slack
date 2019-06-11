/*
 * @Author: py.wang 
 * @Date: 2019-05-15 09:26:17 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-16 07:57:02
 */

#ifndef MUDUO_BASE_LOGFILE_H
#define MUDUO_BASE_LOGFILE_H

#include <muduo/base/Mutex.h>
#include <muduo/base/Types.h>
#include <muduo/base/noncopyable.h>

#include <memory>

namespace muduo
{

class LogFile : noncopyable
{
public:
    LogFile(const string &basename, 
            size_t rollsize,
            bool threadSafe = true,
            int flushInterval = 3);
    ~LogFile();

    void append(const char *logline, int len);
    void flush();

private:
    void append_unlocked(const char *logline, int len);

    static string getLogFileName(const string &basename, time_t *now);
    void rollFile();

    const string basename_;     // 日志文件basename
    const size_t rollSize_;     // 日志文件rollSize_
    const int flushInterval_;   // 写入间隔时间

    int count_;

    std::unique_ptr<MutexLock> mutex_;  // 智能指针管理mutex_
    time_t startOfPeriod_;      // 开始记录日志时间
    time_t lastRoll_;           // 上一次滚动时间
    time_t lastFlush_;          // 最后一次刷新缓冲区
    class File;                 // 内部文件类
    std::unique_ptr<File> file_;

    const static int kCheckTimeRoll_ = 1024;
    const static int kRollPerSeconds_ = 60 * 60 * 1024;
};

}   // namespace muduo

#endif  // MUDUO_BASE_LOGFILE_H