/*
 * @Author: py.wang 
 * @Date: 2019-05-15 09:26:17 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-16 07:57:02
 */

#ifndef LOG_LOGFILE_H
#define LOG_LOGFILE_H

#include "src/base/Mutex.h"
#include "src/base/Types.h"
#include "src/base/noncopyable.h"

#include <memory>

namespace slack
{

class LogFile : noncopyable
{
public:
    LogFile(const string &basename, 
            off_t rollsize,
            bool threadSafe = true,
            int flushInterval = 3,
            int checkEveryN = 1024);
    ~LogFile();

    void append(const char *logline, int len);
    void flush();
    bool rollFile();

private:
    void append_unlocked(const char *logline, int len);

    static string getLogFileName(const string &basename, time_t *now);

    const string basename_;     // 日志文件basename
    const size_t rollSize_;     // 日志文件rollSize_
    const int flushInterval_;   // 写入间隔时间
    const int checkEveryN_;

    int count_;

    std::unique_ptr<MutexLock> mutex_;  // 智能指针管理mutex_
    time_t startOfPeriod_;              // 开始记录日志时间
    time_t lastRoll_;                   // 上一次滚动时间
    time_t lastFlush_;                  // 最后一次刷新缓冲区
    class File;
    std::unique_ptr<File> file_;

    const static int kRollPerSeconds_ = 60 * 60 * 24;
};

}   // namespace slack

#endif  // LOG_LOGFILE_H