#pragma once

#include "mini_muduo/base/noncopyable.h"
#include "mini_muduo/base/Timestamp.h"
#include "mini_muduo/Callbacks.h"

#include <atomic>
#include <cstdint>

namespace mini_muduo 
{
    class Timer : public Noncopyable 
    {
        public:
            Timer(Timestamp expiration, TimerCallback cb, double interval = 0.0);
            ~Timer();

            Timestamp expiration() const { return expiration_; }
            bool repeat() const { return repeat_; }
            int64_t sequence() const { return sequence_; }

            void Run() const { callback_(); }
            
            void Reset(Timestamp now);

        private:
            Timestamp expiration_;
            TimerCallback callback_;
            double interval_;
            bool repeat_;
            int64_t sequence_;

            static std::atomic<int64_t> g_num_created_;
    };

} // namespace mini_muduo