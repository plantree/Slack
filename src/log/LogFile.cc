/*
 * @Author: py.wang 
 * @Date: 2019-05-16 07:55:15 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-16 09:33:56
 */

#include <muduo/base/LogFile.h>
#include <muduo/base/Logging.h>
#include <muduo/base/ProcessInfo.h>
#include <muduo/base/noncopyable.h>

#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

using namespace muduo;

// not thread safe
class LogFile::File : noncopyable
{
public:
    explicit File(const string &filename)
        : fp_(::fopen(filename.data(), "ae")),   // append and O_CLOEXEC
        writtenBytes_(0)
    {
        assert(fp_);
        ::setbuffer(fp_, buffer_, sizeof buffer_);  // 设置缓冲区
    }

    ~File()
    {
        ::fclose(fp_);
    }

    void append(const char *logline, const size_t len)
    {
        size_t n = write(logline, len);
        size_t remain = len - n;
        while (remain > 0)
        {
            size_t x = write(logline + n, remain);
            // FIXME
            if (x == 0) // EOF
            {
                int err = ferror(fp_);
                if (err)
                {
                    fprintf(stderr, "LogFile::File::append() failed %s\n", strerror_tl(err));
                }
                break;
            }
            n += x;
            remain = len - n;
        }
        writtenBytes_ += len;
    }

    void flush()
    {
        ::fflush(fp_);
    }

    size_t writtenBytes() const 
    {
        return writtenBytes_;
    }

private:
    size_t write(const char *logline, size_t len)
    {
        #undef fwrite_unlocked
        // 系统的无锁写，非线程安全，不管有没有锁，都会写
        // https://linux.die.net/man/3/flockfile
        return ::fwrite_unlocked(logline, 1, len, fp_);
    }

    FILE *fp_;
    char buffer_[64 * 1024];
    size_t writtenBytes_;
};


LogFile::LogFile(const string &basename,
                size_t rollSize,
                bool threadSafe,
                int flushInterval)
    : basename_(basename),
    rollSize_(rollSize),
    flushInterval_(flushInterval),
    count_(0),
    mutex_(threadSafe ? new MutexLock : nullptr),
    startOfPeriod_(0),
    lastRoll_(0),
    lastFlush_(0)
{
    assert(basename_.find('/') == string::npos);
    rollFile();
}

LogFile::~LogFile()
{
}

void LogFile::append(const char *logline, int len)
{
    if (mutex_)
    {
        MutexLockGuard lock(*mutex_);
        append_unlocked(logline, len);
    }
    else 
    {
        append_unlocked(logline, len);
    }
}

void LogFile::flush()
{
    if (mutex_)
    {
        MutexLockGuard lock(*mutex_);
        file_->flush();
    }
    else 
    {
        file_->flush();
    }
}

void LogFile::append_unlocked(const char *logline, int len)
{
    file_->append(logline, len);    // 写入文件

    // 达到rollSize_
    if (file_->writtenBytes() > rollSize_)
    {
        rollFile();
    }
    else 
    {
        // 达到检查次数
        if (count_ > kCheckTimeRoll_)
        {
            count_ = 0;
            time_t now = ::time(nullptr);
            time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
            if (thisPeriod_ != startOfPeriod_)
            {
                rollFile();
            }
            else if (now - lastFlush_ > flushInterval_)
            {
                lastFlush_ = now;
                file_->flush();
            }
        }
        else 
        {
            ++count_;
        }
    }
}

void LogFile::rollFile()
{
    time_t now = 0;
    string filename = getLogFileName(basename_, &now);
    // 对齐至kRollPerSeconds_的整数倍，也就是时间调整到零点
    time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

    if (now > lastRoll_)
    {
        lastRoll_ = now;
        lastFlush_ = now;
        startOfPeriod_ = start;
        file_.reset(new File(filename));
    }
}

string LogFile::getLogFileName(const string &basename, time_t *now)
{
    string filename;
    filename.reserve(basename.size() + 64);
    filename = basename;

    char timebuf[32];
    char pidbuf[32];
    struct tm tm;
    *now = time(nullptr);
    //gmtime_r(now, &tm);
    localtime_r(now, &tm);
    strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
    filename += timebuf;
    filename += ProcessInfo::hostname();
    snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
    filename += pidbuf;
    filename += ".log";

    return filename;
}
