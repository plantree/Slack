/*
 * @Author: py.wang 
 * @Date: 2019-05-13 08:50:27 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-14 08:07:22
 */

#ifndef BASE_LOGGING_H
#define BASE_LOGGING_H

#include "src/log/LogStream.h"
#include "src/base/Timestamp.h"

#include <string.h>

namespace slack
{

// 日志器
class Logger
{
public:
    // 日志级别
    enum LogLevel
    {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        FATAL,
        NUM_LOG_LEVELS
    };

    // complile time calculation of basename of source file
    class SourceFile    // 从path中提取source file
    {
    public:
        // 模板成员函数(参数是数组)
        template <int N>
        inline SourceFile(const char (&arr)[N]) // 引用数组
            : data_(arr),
            size_(N-1)
        {
            const char *slash = strrchr(data_, '/');
            if (slash)
            {
                data_ = slash + 1;
                size_ -= static_cast<int>(data_ - arr);
            }
        }

        // 参数是指针
        explicit SourceFile(const char *filename)
            : data_(filename)
        {
            const char *slash = strrchr(data_, '/');
            if (slash)
            {
                data_ = slash + 1;
            }
            size_ = static_cast<int>(strlen(data_));
        }

        // member data
        const char *data_;
        int size_;
    };

    // ctor
    Logger(SourceFile file, int line);
    Logger(SourceFile file, int line, LogLevel level);
    Logger(SourceFile file, int line, LogLevel level, const char *func);
    Logger(SourceFile file, int line, bool toAbort);
    ~Logger();

    LogStream &stream()
    {
        return impl_.stream_;
    }

    static LogLevel logLevel();
    static void setLogLevel(LogLevel level);

    // 函数指针
    //typedef void (*OutputFunc)(const char *msg, int len);
    //typedef void (*FlushFunc)();
    using OutputFunc = void(*)(const char *msg, int len);
    using FlushFunc = void(*)();

    static void setOutput(OutputFunc);
    static void setFlush(FlushFunc);

private:
    // pointer to implementation 
    // removes implementation details of a class from its object
    // representation by placing them in a separate class
    class Impl
    {
    public:
        using LogLevel = Logger::LogLevel;
        Impl(LogLevel level, int old_errno, const SourceFile &file, int line);
        void formatTime();
        void finish();

        Timestamp time_;
        LogStream stream_;
        LogLevel level_;
        int line_;
        SourceFile basename_;
    };
    // 内部类
    Impl impl_;
};

extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel()
{
    return g_logLevel;
}

// 宏
#define LOG_TRACE if (slack::Logger::LogLevel() <= slack::Logger::TRACE)\
    slack::Logger(__FILE__, __LINE__, slack::Logger::TRACE, __func__).stream()
#define LOG_DEBUG if (slack::Logger::LogLevel() <= slack::Logger::DEBUG)\
    slack::Logger(__FILE__, __LINE__, slack::Logger::DEBUG, __func__).stream()
#define LOG_INFO if (slack::Logger::LogLevel() <= slack::Logger::INFO)\
    slack::Logger(__FILE__, __LINE__, slack::Logger::INFO, __func__).stream()
#define LOG_WARN slack::Logger(__FILE__, __LINE__, slack::Logger::WARN).stream()
#define LOG_ERROR slack::Logger(__FILE__, __LINE__, slack::Logger::ERROR).stream()
#define LOG_FATAL slack::Logger(__FILE__, __LINE__, slack::Logger::FATAL).stream()

#define LOG_SYSERR slack::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL slack::Logger(__FILE__, __LINE__, true).stream()

// errno字符串表示
const char *strerror_tl(int savedErrno);

// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL(val) \
  ::slack::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", (val))

template <typename T>
T *CheckNotNull(Logger::SourceFile file, int line, const char *names, T *ptr)
{
    if (!ptr)
    {
        Logger(file, line, Logger::FATAL).stream() << names;
    }
    return ptr;
}

} // namespace slack


#endif  // LOG_LOGGING_H
