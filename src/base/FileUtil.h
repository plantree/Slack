/*
 * @Author: py.wang 
 * @Date: 2019-05-15 07:47:27 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-15 07:54:13
 */

#ifndef BASE_FILEUTIL_H
#define BASE_FILEUTIL_H

#include "src/base/StringPiece.h"
#include "src/base/noncopyable.h"

#include <sys/types.h>  // for off_t

namespace slack
{

namespace FileUtil
{

// read small file < 64KB
class ReadSmallFile : noncopyable
{
public:
    ReadSmallFile(StringArg filename);
    ~ReadSmallFile();

    // return errno
    // 函数模板隐式推导
    template <typename String>
    int readToString(int maxSize,
                    String *content,
                    int64_t *fileSize,
                    int64_t *modifyTime,
                    int64_t *createTime);
    
    // read at maximum kBufferSize into buf_
    // return errno
    int readToBuffer(int *size);

    const char *buffer() const 
    {
        return buf_;
    }

    static constexpr int kBufferSize = 64 * 1024;

private:
    int fd_;                // 文件描述符
    int err_;               // 错误信息
    char buf_[kBufferSize]; // 内部缓冲区
};

// read the file content, return errno if error happens
template <typename String>
int readFile(StringArg fileName,
            int maxSize,
            String *content,
            int64_t *fileSize = nullptr,
            int64_t *modifyTime = nullptr,
            int64_t *createTime = nullptr)
{
    ReadSmallFile file(fileName);
    return file.readToString(maxSize, content, fileSize, modifyTime, createTime);
}


}   // namespace FileUtil

}   // namespace slack

#endif // BASE_LOGUTIL_H