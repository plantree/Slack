# Slack: A C++ Network Library basing on Reactor 

[![Build Status](https://travis-ci.org/plantree/Slack.svg?branch=master)](https://travis-ci.org/plantree/Slack/) [![license](https://img.shields.io/github/license/mashape/apistatus.svg)](https://opensource.org/licenses/MIT)

### Introduction

陈硕老师的`muduo`代码整体过了一遍，收获很多，尽管整个过程并不轻松，头一次看这么复杂的源码。代码和书都是第一遍看，很多细节没有深究，打算第二遍把整个逻辑重新梳理一下，顺道记在博客里，整个学习过程，遇到的问题，解决的方案，以及为什么的`trade off`

### Goals

- 理解`Reactor`设计的总体思路，以及要解决的问题。和陈硕老师推荐的Linux系统下C++多线程服务端编程模式`one loop per thread + thread pool`这样设计的原因
- 使用[Catch2](https://github.com/catchorg/Catch2)做单元测试，使用C++11语法对源代码部分重写，去除`Boost`依赖，完全使用标准库基础设施
- 学习使用`Git`做版本控制，使用`Travis`做持续集成，构建工具采用`CMake`

### Environment

- OS: Ubuntu 18.04 (64 bit)
- Compiler: GCC 7.3.0 (Ubuntu 7.3.0-27ubuntu1~18.04) 
- Build: CMake 3.10.2

### Design

- 按照`base`，`log`，`net`和`http`的顺序依次实现四个独立的库（`muduo`中只是按照`base`和`net`，示例放到`example`中）
- 单元测试用例放在各自的库所在文件夹

### Features

- 底层使用Epoll水平触发实现IO多路复用，文件描述符均为非阻塞
- 原子类依赖C++11中的`atomic`，借助智能指针`shared_ptr`和`unique_ptr`管理资源，去除`Boost`依赖
- RAII编程技巧，借助对象的生存期管理资源的申请和释放
- 定时器事件借助`timerfd`，线程唤醒事件通知借助`eventfd`，以及监听和连接套接字，统一纳入到IO多路复用体系中
- 基于红黑二叉搜索树的定时器队列
- 基于双缓冲的异步日志
- Reactor设计模式，主Reactor通过accept连接，按照Round Robin的方式依次派发给线程池中的次Reactor

### Statistics

1. 代码统计

![](https://raw.githubusercontent.com/plantree/PictureBed/master/images/20190723192245.png)

2. [ab](https://www.google.com.hk/url?sa=t&rct=j&q=&esrc=s&source=web&cd=2&ved=2ahUKEwj4zKiS_srjAhWEoJ4KHZU5Ah4QFjABegQIABAB&url=https%3A%2F%2Fhttpd.apache.org%2Fdocs%2F2.4%2Fprograms%2Fab.html&usg=AOvVaw1E9XkdDRN5RkpsgZUEUrZ4)测试（ab是Apache基金会下的一款HTTP服务器压测工具）

   ```
   ab -c 100 -n 100 -k http://localhost:8000/
   ```

   并发数100，测试100次，Keep-alive（长连接）

   作为对比，使用了Nodejs的Express框架，访问的都是[https://github.com/plantree/Slack](https://github.com/plantree/Slack)的html内容

   Nodejs：

   ![](https://raw.githubusercontent.com/plantree/PictureBed/master/images/20190723200155.png)

   Slack（线程100）：

   ![](https://raw.githubusercontent.com/plantree/PictureBed/master/images/20190723201213.png)

   简单测试，看起来效果还不错！

### Plan

- 继续研究muduo源码，熟悉各种网络编程的例子
- 丰富HTTP服务器，实现为REST风格接口

### Reference

- [https://github.com/chenshuo/muduo](https://github.com/chenshuo/muduo)
- 《Linux多线程服务端编程》
- 《Unix网络编程》
- 《Unix环境高级编程》
- 《Linux系统编程手册》