# Slack: A C++ Network Library basing on Reactor 

[![Build Status](https://travis-ci.org/plantree/Slack.svg?branch=master)](https://travis-ci.org/plantree/Slack/) [![license](https://img.shields.io/github/license/mashape/apistatus.svg)](https://opensource.org/licenses/MIT)

### Introduction

陈硕老师的`Muduo`代码整体过了一遍，收获很多，尽管整个过程并不轻松，头一次看这么复杂的源码。代码和书都是第一遍看，很多细节没有深究，打算第二遍把整个逻辑重新梳理一下，顺道记在博客里，整个学习过程，遇到的问题，解决的方案，以及为什么的`trade off`。

### Goals

- 理解`Reactor`设计的总体思路，以及要解决的问题。和陈硕老师推荐的Linux系统下C++多线程服务端编程模式`one(event) loop per thread + thread pool`这样设计的原因
- 使用[Catch2](https://github.com/catchorg/Catch2)做单元测试，使用C++11语法对源代码部分重写，减少`Boost`依赖，完全使用标准库基础设施
- 学习使用`Git`做版本控制，使用`Travis`做持续集成，构建工具采用`CMake`

### Environment

- OS: Ubuntu 18.04 (64 bit)
- Compiler: gcc 7.3.0 (Ubuntu 7.3.0-27ubuntu1~18.04) 
- Build: CMake 3.10.2

### Design

- 按照`base`，`log`，`net`和`http`的顺序依次实现四个独立的库（`muduo`中只是按照`base`和`net`，示例放到`example`中）
- 单元测试用例放在各自的库所在文件夹

### Todo

- [x] Timestamp
- [x] Atomic
- [x] Exception/CurrentThread
- [x] Mutex/Condition/CountDownLatch
- [ ] Thread
- [ ] 

### Blogs

1. 



### Reference

- [https://github.com/chenshuo/muduo](https://github.com/chenshuo/muduo)
- 