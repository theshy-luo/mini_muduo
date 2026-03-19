#include "mini_muduo/TimerQueue.h"
#include "mini_muduo/Timer.h"
#include "mini_muduo/TimerId.h"
#include "mini_muduo/base/Timestamp.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <ostream>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

namespace mini_muduo 
{
namespace detail 
{
    int CreateTimerFd()
    {
        int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
        if (timerfd < 0)
        {
            std::cerr << "Failed to create timerfd" << std::endl;
        }
        return timerfd;
    }

    struct timespec HowMuchTimeFromNow(Timestamp when)
    {
        int64_t microseconds = when.micro_seconds_since_epoch() - 
            Timestamp::Now().micro_seconds_since_epoch();
        if (microseconds < 100) 
        {
            microseconds = 100;
        }
        struct timespec ts;
        ts.tv_sec = static_cast<time_t>(microseconds / 
            Timestamp::kMicroSecondsPerSecond);
        ts.tv_nsec = static_cast<long>((microseconds % 
            Timestamp::kMicroSecondsPerSecond) * 1000);
        
        return ts;
    }

    void StopTimerFd(int timer_fd)
    {
        struct itimerspec zero;
        ::memset(&zero, 0, sizeof(zero));
        ::timerfd_settime(timer_fd, 0, &zero, nullptr);
    }

    void ResetTimerFd(int timer_fd, Timestamp expiration)
    {
        struct itimerspec newValue;
        struct itimerspec oldValue;
        ::memset(&newValue, 0, sizeof(newValue));
        newValue.it_value = HowMuchTimeFromNow(expiration);
        ::timerfd_settime(timer_fd, 0, &newValue, &oldValue);
    }

    void ReadTimerFd(int timer_fd, Timestamp now)
    {
        uint64_t how_many;
        ssize_t n = ::read(timer_fd, &how_many, sizeof(how_many));

        std::cout << "TimerQueue::handleRead() " << how_many << " at " << now.ToString() << std::endl;
        if (n != sizeof(how_many))
        {
            std::cerr << "TimerQueue::handleRead() reads " << n << " bytes instead of 8" << std::endl;
        }
    }
}
    TimerQueue::TimerQueue(EventLoop *loop):
        loop_(loop),
        timer_fd_(detail::CreateTimerFd()),
        timer_fd_channel_(loop, timer_fd_),
        calling_expired_timers_(false)
    {
        timer_fd_channel_.SetReadCallback([this](Timestamp) { HandleRead(); });
        timer_fd_channel_.EnableReading();
    }

    TimerQueue::~TimerQueue()
    {
        timer_fd_channel_.DisableAll();
        timer_fd_channel_.Remove();
        ::close(timer_fd_);

        for (auto& timer : timers_)
        {
            delete timer.second;
        }
    }

    TimerId TimerQueue::AddTimer(Timestamp when, TimerCallback cb, double interval)
    {
        Timer* timer = new Timer(when, cb, interval);
        loop_->RunInLoop(std::bind(&TimerQueue::AddTimeInLoop, 
            this, when, timer));

        return TimerId(timer, timer->sequence());
    }

    void TimerQueue::CancelTimer(TimerId timer_id)
    {
        loop_->QueueInLoop(
            std::bind(&TimerQueue::CancelTimerInLoop, this, timer_id));
    }

    void TimerQueue::HandleRead()
    {
        loop_->AssertInLoopThread();
        
        Timestamp now = Timestamp::Now();
        // 读走 timerfd 的数据，防止它一直触发
        detail::ReadTimerFd(timer_fd_, now);

        std::vector<TimerQueue::Entry> expired = GetExpired(now);

        calling_expired_timers_ = true;
        for (auto& entry : expired)
        {
            entry.second->Run();
        }

        Reset(expired, now);
        calling_expired_timers_ = false;
    }

    std::vector<TimerQueue::Entry> TimerQueue::GetExpired(Timestamp now)
    {
        std::vector<Entry> expired;
        TimerList::iterator end = timers_.lower_bound(
            Entry(now, reinterpret_cast<Timer*>(UINTPTR_MAX)));
        for (auto it = timers_.begin(); it != end; ++it)
        {
            expired.push_back(*it);
            active_timers_.erase(ActiveTimer(it->second, it->second->sequence()));
        }
        timers_.erase(timers_.begin(), end);

        return expired;
    }

    void TimerQueue::Reset(const std::vector<Entry>& expired, Timestamp now)
    {
        for (auto& entry : expired)
        {
            ActiveTimer timer(entry.second, entry.second->sequence());
            if (entry.second->repeat() && 
                cancelling_timers_.find(timer) == cancelling_timers_.end()) 
            {
                entry.second->Reset(now);
                insert(entry.second);
            }
            else 
            {
                delete entry.second;
            }
        }
        cancelling_timers_.clear();

        if (!timers_.empty()) 
        {
            detail::ResetTimerFd(timer_fd_, timers_.begin()->first);
        }
    }

    void TimerQueue::AddTimeInLoop(Timestamp when, Timer* timer)
    {
        loop_->AssertInLoopThread();

        if(insert(timer))
        {
            detail::ResetTimerFd(timer_fd_, when);
        }
    }

    void TimerQueue::CancelTimerInLoop(TimerId timer_id)
    {
        loop_->AssertInLoopThread();
        ActiveTimer timer(timer_id.timer_, timer_id.sequence_);
        auto it = active_timers_.find(timer);
        if (it != active_timers_.end()) 
        {
            bool was_earliest = (!timers_.empty() && 
                (timers_.begin()->second == it->first));
            timers_.erase(Entry(it->first->expiration(), it->first));
            delete it->first;
            active_timers_.erase(it);

            if (was_earliest) 
            {
                if (!timers_.empty()) 
                {
                    detail::ResetTimerFd(timer_fd_, timers_.begin()->first);
                }
                else 
                {
                    detail::StopTimerFd(timer_fd_);
                }
            }
        }
        else if(calling_expired_timers_)
        {
            // 正在执行回调，加入取消缓冲区，Reset 时会跳过它
            cancelling_timers_.insert(timer);
        }
    }

    bool TimerQueue::insert(Timer* timer)
    {
        loop_->AssertInLoopThread();

        bool earliest_changed = false;
        Timestamp when = timer->expiration();
        TimerList::iterator it = timers_.begin();
        // 加入的timer是否为最早的
        if (it == timers_.end() ||when < it->first) 
        {
            earliest_changed = true;
        }

        {
            std::pair<TimerList::iterator, bool> result = 
                timers_.insert(Entry(when, timer));
            assert(result.second);
            (void)result;
        }

        {
            std::pair<ActiveTimerSet::iterator, bool> result = 
                active_timers_.insert(ActiveTimer(timer, timer->sequence()));
            assert(result.second);
            (void)result;
        }
        
        return earliest_changed;
    }
}