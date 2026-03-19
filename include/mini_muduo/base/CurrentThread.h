#pragma once

#include <sys/types.h>
#include <string>

namespace mini_muduo 
{
    namespace CurrentThread 
    {
        // __thread 修饰的变量是线程局部存储 (Thread Local Storage, TLS) 的
        // 意味着每个线程都会有一份这些变量的独立实体，互相不干扰。
        // 这比每次都调用系统调用获取 tid 性能高得多。
        extern __thread pid_t t_cachedTid;
        extern __thread char t_tidString[32];
        extern __thread int t_tidStringLength;
        extern __thread const char* t_threadName;

        // 如果还没有缓存过当前线程的 ID，则调用底层的系统调用获取并缓存起来
        void cacheTid();

        // 供外部获取当前线程的真实 ID
        inline int tid()
        {
            // __builtin_expect 是 GCC 内置宏，用于分支预测优化。
            // t_cachedTid == 0 也就是“没缓存过”这种事情对于一个线程来说只发生一次，
            // 绝大多数情况都是假 (0)，所以告诉 CPU 这个分支大概率不执行，能提升性能。
            if (__builtin_expect(t_cachedTid == 0, 0))
            {
                cacheTid();
            }
            return t_cachedTid;
        }

        // 获取字符串格式的 tid，专门为了以后写日志追求极致性能保留的
        inline const char* tidString() // for logging
        {
            return t_tidString;
        }

        inline int tidStringLength() // for logging
        {
            return t_tidStringLength;
        }

        inline const char* name()
        {
            return t_threadName;
        }

        std::string stackTrace(bool demangle);
    }
}