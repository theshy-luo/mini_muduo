#include "mini_muduo/base/CurrentThread.h"

#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <cxxabi.h>
#ifdef __GLIBC__
#include <execinfo.h>
#endif
#include <stdlib.h>

namespace mini_muduo 
{
    namespace CurrentThread 
    {
        __thread pid_t t_cachedTid = 0;
        __thread char t_tidString[32];
        __thread int t_tidStringLength = 6;
        __thread const char* t_threadName = "unknown";
        
        // 核心获取操作系统线程真实 ID 的方法
        // pthread_self() 拿到的是 pthread 库层面的线程 ID，
        // 而 syscall(SYS_gettid) 拿到的是 Linux 内核调度的真实线程 ID (LWP - 轻量级进程)
        pid_t gettid() 
        {
            return static_cast<pid_t>(::syscall(SYS_gettid));
        }

        // 把取到的真实 tid 缓存在当前线程私有变量里，并且顺手把它格式化拼凑成给日志用的字符串
        void cacheTid()
        {
            if (t_cachedTid == 0)
            {
                t_cachedTid = gettid();
                t_tidStringLength = snprintf(t_tidString, sizeof(t_tidString), "%5d ", t_cachedTid);
            }
        }

        std::string stackTrace(bool demangle)
        {
            std::string stack;
#ifdef __GLIBC__
            const int max_frames = 200;
            void* frame[max_frames];
            int nptrs = ::backtrace(frame, max_frames);
            char** strings = ::backtrace_symbols(frame, nptrs);
            if (strings)
            {
                size_t len = 256;
                char* demangled = demangle ? static_cast<char*>(::malloc(len)) : nullptr;
                for (int i = 1; i < nptrs; ++i)  // skipping the 0-th, which is this function
                {
                    if (demangle)
                    {
                        // https://panthema.net/2008/0901-stacktrace-demangled/
                        // bin/exception_test(_ZN3Bar4testEv+0x79) [0x401909]
                        char* left_par = nullptr;
                        char* plus = nullptr;
                        for (char* p = strings[i]; *p; ++p)
                        {
                            if (*p == '(')
                                left_par = p;
                            else if (*p == '+')
                                plus = p;
                        }

                        if (left_par && plus)
                        {
                            *plus = '\0';
                            int status = 0;
                            char* ret = abi::__cxa_demangle(left_par+1, demangled, &len, &status);
                            *plus = '+';
                            if (status == 0)
                            {
                                demangled = ret;  // ret could be realloc()
                                stack.append(strings[i], left_par+1);
                                stack.append(demangled);
                                stack.append(plus);
                                stack.push_back('\n');
                                continue;
                            }
                        }
                    }
                    // Fallback to mangled names
                    stack.append(strings[i]);
                    stack.push_back('\n');
                }
                free(demangled);
                free(strings);
            }
#else
            (void)demangle;
            // 非 glibc 环境（如 musl libc / OpenWrt / Alpine 等），不支持 backtrace
            stack = "stackTrace not supported on this platform.\n";
#endif
            return stack;
        }
    }
}