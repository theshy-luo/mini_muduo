#pragma once

#include "mini_muduo/base/noncopyable.h"
#include "mini_muduo/Callbacks.h"

#include <string>
#include <vector>

namespace mini_muduo 
{
    class EventLoop;
    class EventLoopThread;
    class EventLoopThreadPool : public Noncopyable
    {
        public:
            // name的长度限制在16个字节
            EventLoopThreadPool(EventLoop* base_loop, int num_threads, 
                const std::string& base_name = std::string());
            ~EventLoopThreadPool();

            void Start(ThreadInitCallback cb);

            void SetThreadNum(int num_threads)
            {
                assert(!started_);
                num_threads_ = num_threads;
            }

            EventLoop* GetNextLoop();

            bool started() const
            { return started_; }

            const std::string& name() const
            { return base_name_; }

        private:
            EventLoop* base_loop_;
            int num_threads_;
            bool started_;
            std::string base_name_;
            std::vector<std::unique_ptr<EventLoopThread>> threads_;
            std::vector<EventLoop*> loops_;
            int next_;
    };
} // namespace mini_muduo