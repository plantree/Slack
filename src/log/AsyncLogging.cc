/*
 * @Author: py.wang 
 * @Date: 2019-05-16 09:04:56 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-05-16 10:13:40
 */

#include "src/log/AsyncLogging.h"
#include "src/log/LogFile.h"
#include "src/base/Timestamp.h"

#include <stdio.h>

using namespace slack;

AsyncLogging::AsyncLogging(const string &basename,
                        off_t rollSize,
                        int flushInterval)
    : flushInterval_(flushInterval),
    running_(false),
    basename_(basename),
    rollSize_(rollSize),
    thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
    latch_(1),
    mutex_(),
    cond_(mutex_),
    currentBuffer_(new Buffer),
    nextBuffer_(new Buffer),
    buffers_()
{
    // initialize
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}

// 前端写入缓冲区
void AsyncLogging::append(const char *logline, int len)
{
    slack::MutexLockGuard lock(mutex_);
    // 当前缓冲区有剩余
    if (currentBuffer_->avail() > len)
    {
        currentBuffer_->append(logline, len);
    }
    else 
    {
        // 放入后端缓冲区数组，移动语义
        buffers_.push_back(std::move(currentBuffer_));

        // 双缓冲区转移, 后备缓冲区换上
        if (nextBuffer_)
        {
            currentBuffer_ = std::move(nextBuffer_);
        }
        else 
        {
            // 重置
            currentBuffer_.reset(new Buffer);   // rarely happens
        }
        currentBuffer_->append(logline, len);
        // 通知后端可写
        cond_.notify();
    }
}

// 后端异步写
void AsyncLogging::threadFunc()
{
    assert(running_ == true);
    latch_.countDown();
    // 非线程安全, 因为只有一个线程写
    LogFile output(basename_, rollSize_, false);
    // 准备两个缓冲区，与前端对应
    BufferPtr newBuffer1(new Buffer);
    BufferPtr newBuffer2(new Buffer);
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;
    buffersToWrite.reserve(16);

    while (running_)
    {
        assert(newBuffer1 && newBuffer2->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());
        // 前后端缓冲区交换
        {
            slack::MutexLockGuard lock(mutex_);
            // 没有要写的，等候flushInterval_
            if (buffers_.empty())   // rarely happens
            {
                cond_.waitForSeconds(flushInterval_);
            }
            // 强制缓冲区移入
            buffers_.push_back(std::move(currentBuffer_));
            // 置为空
            currentBuffer_ = std::move(newBuffer1);
            // 交换缓冲区
            buffersToWrite.swap(buffers_);
            if (!nextBuffer_)
            {
                nextBuffer_ = std::move(newBuffer2);
            }
        }

        assert(!buffersToWrite.empty());

        // 太多直接丢弃
        if (buffersToWrite.size() > 25)
        {
            char buf[256];
            snprintf(buf, sizeof buf, "Dropped log message at %s, %zd larger buffers\n",
                Timestamp::now().toFormattedString().c_str(),
                buffersToWrite.size()-2);
            fputs(buf, stderr);
            // 写入信息
            output.append(buf, static_cast<int>(strlen(buf)));
            // 保留前两个日志
            buffersToWrite.erase(buffersToWrite.begin()+2, buffersToWrite.end());
        }

        // 写入文件
        for (const auto &buffer : buffersToWrite)
        {
            output.append(buffer->data(), buffer->length());
        }

        // 保留两个
        if (buffersToWrite.size() > 2)
        {
            buffersToWrite.resize(2);
        }

        // 被换出到currentBuffer_
        if (!newBuffer1)
        {
            assert(!buffersToWrite.empty());
            newBuffer1 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }

        // 被换出到nextBuffer_
        if (!newBuffer2)
        {
            assert(!buffersToWrite.empty());
            newBuffer2 = std::move(buffersToWrite.back());
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }

        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}