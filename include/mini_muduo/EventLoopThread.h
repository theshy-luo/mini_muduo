#pragma once

#include "mini_muduo/base/Thread.h"
#include "mini_muduo/Callbacks.h"
#include <mutex>
#include <condition_variable>

namespace mini_muduo 
{
    class EventLoop;
    class EventLoopThread : public Noncopyable
    {
        public:
            EventLoopThread(ThreadInitCallback cb, const std::string& base_name = std::string());
            ~EventLoopThread();

            EventLoop* StartLoop();

        private:
            void ThreadFunc();

            EventLoop* loop_; // 由子线程创建并回写，受 mutex_ 保护
            std::mutex mutex_; 
            bool exiting_;
            std::condition_variable cond_;
            ThreadInitCallback init_callback_;

            Thread thread_; // 建议：放在最后，保证其他成员先初始化
    };
} // namespace mini_muduo