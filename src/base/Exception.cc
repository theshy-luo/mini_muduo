#include "mini_muduo/base/Exception.h"
#include "mini_muduo/base/CurrentThread.h"

namespace mini_muduo 
{
    Exception::Exception(std::string what)
        : message_(std::move(what)),
          stack_(CurrentThread::stackTrace(true))
    {
    }
}