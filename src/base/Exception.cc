/*
 * @Author: py.wang 
 * @Date: 2019-05-04 09:08:42 
 * @Last Modified by: py.wang
 * @Last Modified time: 2019-06-06 18:26:02
 */

#include "src/base/Exception.h"
#include "src/base/CurrentThread.h"

#include <iostream>
#include <cxxabi.h>
#include <execinfo.h>
#include <stdlib.h>
#include <stdio.h>

namespace slack
{

Exception::Exception(string msg)
    : message_(std::move(msg)),
    stack_(CurrentThread::stackTrace(false))
{
}

}   // namespace slack
