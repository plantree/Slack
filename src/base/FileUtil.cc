/*
 * @Author: py.wang 
 * @Date: 2019-05-15 07:54:46 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-15 08:48:08
 */

#include "src/base/FileUtil.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string>

using namespace slack;

// 只读模式打开，exec的时候自动关闭文件描述符，多线程必要，避免数据竞争
// fcntl也可以设置，但是不是原子操作
FileUtil::ReadSmallFile::ReadSmallFile(StringArg filename)
    : fd_(::open(filename.c_str(), O_RDONLY | O_CLOEXEC)),
    err_(0)
{
    buf_[0] = '\0';
    if (fd_ < 0)
    {
        err_ = errno;
    }
}

FileUtil::ReadSmallFile::~ReadSmallFile()
{
    if (fd_ >= 0)
    {
        ::close(fd_);   // FIXME: check EINTR
    }
}

// return errno
template <typename String>
int FileUtil::ReadSmallFile::readToString(int maxSize,
                                    String *content,
                                    int64_t *fileSize,
                                    int64_t *modifyTime,
                                    int64_t *createTime)
{
    static_assert(sizeof(off_t) == 8, "_FILE_OFFSET_BITS = 64");
    assert(content != nullptr);
    // save original error
    int err = err_;

    if (fd_ >= 0)
    {
        // 清空
        content->clear();

        if (fileSize)
        {
            struct stat statbuf;
            if (::fstat(fd_, &statbuf) == 0)
            {
                // regular file
                if (S_ISREG(statbuf.st_mode))
                {
                    *fileSize = statbuf.st_size;
                    content->reserve(static_cast<int>(std::min(implicit_cast<int64_t>(maxSize), *fileSize)));
                }
                else if (S_ISDIR(statbuf.st_mode))
                {
                    err = EISDIR;
                }
                if (modifyTime)
                {
                    *modifyTime = statbuf.st_mtime;
                }
                if (createTime)
                {
                    *createTime = statbuf.st_ctime;
                }
            }
            else 
            {
                err = errno;
            }
        }

        // 开始读取
        while (content->size() < implicit_cast<size_t>(maxSize))
        {
            size_t toRead = std::min(implicit_cast<size_t>(maxSize) - content->size(), sizeof(buf_));
            ssize_t n = ::read(fd_, buf_, toRead);
            if (n > 0)
            {
                content->append(buf_, n);
            }
            else 
            {
                // error
                if (n < 0)
                {
                    err = errno;
                }
                // EOF
                break;
            }
        }
    }
    return err;
}

int FileUtil::ReadSmallFile::readToBuffer(int *size)
{
    int err = err_;
    if (fd_ >= 0)
    {
        // 不改变lseek
        ssize_t n = ::pread(fd_, buf_, sizeof(buf_)-1, 0);
        if (n >= 0)
        {
            if (size)
            {
                *size = static_cast<int>(n);
            }
            buf_[n] = '\0';
        }
        else 
        {
            err = errno;
        }
    }
    return err;
}

// 实例化
template int FileUtil::readFile(StringArg filename,     
                                int maxSize,
                                string *content,
                                int64_t *, int64_t *, int64_t *);

template int FileUtil::ReadSmallFile::readToString(int maxSize,
                                            string *content,
                                            int64_t *, int64_t *, int64_t *);



