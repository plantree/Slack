/*
 * @Author: py.wang 
 * @Date: 2019-05-13 09:13:32 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-14 09:02:59
 */

#include "src/log/Logging.h"

#include "src/base/CurrentThread.h"
#include "src/base/StringPiece.h"
#include "src/base/Timestamp.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sstream>

namespace slack
{

//线程局部数据
__thread char t_errnobuf[512];
__thread char t_time[32];
__thread time_t t_lastSecond;

// 增加一个中间层，将errno保存在内部
const char *strerror_tl(int savedErrno)
{
    return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

// 初始化Log等级
Logger::LogLevel initLogLevel()
{
    return Logger::TRACE;
}

Logger::LogLevel g_logLevel = initLogLevel();

const char *LogLevelName[Logger::NUM_LOG_LEVELS] = 
{
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

// helper calss for known string length at compile time
class T
{
public:
    // 构造函数中检查
    T(const char *str, unsigned len)
        : str_(str),
        len_(len)
    {
        //printf("%s\n", str);
        assert(strlen(str) == len_);
    }

    const char *str_;
    const unsigned len_;
};

// operator<< 普通函数重载
inline LogStream &operator<<(LogStream &s, T v)
{
    s.append(v.str_, v.len_);
    return s;
}

inline LogStream &operator<<(LogStream &s, const Logger::SourceFile &v)
{
    s.append(v.data_, v.size_);
    return s;
}

// stdout
void defaultOutput(const char *msg, int len)
{
    size_t n = fwrite(msg, 1, len, stdout);
    (void)n;
}

void defalutFlush()
{
    fflush(stdout);
}

Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defalutFlush;

}   // namespace slack

using namespace slack;

// Impl ctor
Logger::Impl::Impl(LogLevel level, int saveErrno, const SourceFile &file, int line)
    : time_(Timestamp::now()),
    stream_(),
    level_(level),
    line_(line),
    basename_(file)
{
    formatTime();
    // 缓存tid
    CurrentThread::tid();
    stream_ << T(CurrentThread::tidString(), 6);
    stream_ << T(LogLevelName[level], strlen(LogLevelName[level])) << " ";
    if (saveErrno != 0)
    {
        stream_ << strerror_tl(saveErrno) << " (errno=" << saveErrno << ") ";
    }
}

// 格式化时间并输出到流
void Logger::Impl::formatTime()
{
    int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
    time_t seconds = static_cast<time_t>(microSecondsSinceEpoch / time_.kMicroSecondsPerSecond);
    int microSeconds = static_cast<int>(microSecondsSinceEpoch % time_.kMicroSecondsPerSecond);
    // 秒数变化, t_lastSecond做缓存
    if (seconds != t_lastSecond)
    {
        t_lastSecond = seconds;
        struct tm tm_time;
        // get formatted time
        ::localtime_r(&seconds, &tm_time);

        int len = snprintf(t_time, sizeof t_time, "%4d%02d%02d %02d:%02d:%02d",
            tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
            tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
        assert(len == 17); (void)len;
    }
    // 左对齐
    Fmt us(".%06dZ ", microSeconds);
    assert(us.length() == 9);
    // output
    stream_ << T(t_time, 17) << T(us.data(), 9);
}

void Logger::Impl::finish()
{
    stream_ << " - " << basename_ << ":" << line_ << "\n";
}

// ctor
Logger::Logger(SourceFile file, int line)
    : impl_(INFO, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, LogLevel level, const char *func)
    : impl_(level, 0, file, line)
{
    impl_.stream_ << func << " ";
}

Logger::Logger(SourceFile file, int line, LogLevel level)
    : impl_(level, 0, file, line)
{
}

Logger::Logger(SourceFile file, int line, bool toAbort)
    : impl_(toAbort ? FATAL : ERROR, errno, file, line)
{
}

// dtor
Logger::~Logger()
{
    impl_.finish();
    // 获取内部Buffer
    const LogStream::Buffer &buf(stream().buffer());
    g_output(buf.data(), buf.length());
    if (impl_.level_ == FATAL)
    {
        g_flush();
        abort();
    }
}

void Logger::setLogLevel(Logger::LogLevel level)
{
    g_logLevel = level;
}

void Logger::setOutput(OutputFunc out)
{
    g_output = out;
}

void Logger::setFlush(FlushFunc flush)
{
    g_flush = flush;
}


