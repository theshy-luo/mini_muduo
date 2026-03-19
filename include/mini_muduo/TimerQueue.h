#pragma once

#include "mini_muduo/Channel.h"
#include "mini_muduo/Timer.h"
#include "mini_muduo/base/Timestamp.h"
#include "mini_muduo/base/noncopyable.h"
#include "mini_muduo/TimerId.h"

#include <cstdint>
#include <set>
#include <utility>
#include <vector>

namespace mini_muduo 
{
    class EventLoop;

    class TimerQueue : public Noncopyable
    {
        public:
            TimerQueue(EventLoop *loop);
            ~TimerQueue();

            // 核心接口
            TimerId AddTimer(Timestamp when, TimerCallback cb, double interval = 0.0);

            void CancelTimer(TimerId timer_id);

        private:
            using Entry = std::pair<Timestamp, Timer*>;
            using TimerList = std::set<Entry>;
            using ActiveTimer = std::pair<Timer*, int64_t>;
            using ActiveTimerSet = std::set<ActiveTimer>;

            void HandleRead();

            // 取出所有到期的定时器
            std::vector<Entry> GetExpired(Timestamp now);

            // 重置/删除到期定时器
            void Reset(const std::vector<Entry>& expired, Timestamp now);

            void AddTimeInLoop(Timestamp when, Timer* timer);

            void CancelTimerInLoop(TimerId timer_id);

            bool insert(Timer* timer);

            EventLoop *loop_;
            const int timer_fd_;
            Channel timer_fd_channel_;


            // 有序存储
            TimerList timers_;
            ActiveTimerSet active_timers_; // 与 timers_ 镜像，同步增删
            
            // 取消缓冲区：存放"在 callback 执行期间"被 cancel 的定时器
            bool calling_expired_timers_; // 当前是否正在执行到期回调
            ActiveTimerSet cancelling_timers_; // 在执行期间被取消的定时器
    };
}
