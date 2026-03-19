#pragma once

#include "mini_muduo/base/copyable.h"

#include <cstdint>

namespace mini_muduo 
{
    class Timer;
    class TimerId : copyable
    {
        public:
            TimerId()
                : timer_(nullptr)
                , sequence_(0)
            {}
            TimerId(Timer* timer, int64_t sequence)
                : timer_(timer)
                , sequence_(sequence)
            {}
            ~TimerId() = default;

        // 允许 TimerQueue 访问私有成员
        friend class TimerQueue;

        private:
            Timer* timer_;
            int64_t sequence_;
    };
} // namespace mini_muduo