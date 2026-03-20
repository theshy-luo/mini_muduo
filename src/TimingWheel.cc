#include "mini_muduo/TimingWheel.h"

#include <functional>
#include <memory>
#include <utility>

namespace mini_muduo
{
    TimingWheel::TimingWheel(EventLoop* loop, int timeout_seconds)
        : loop_(loop)
        , timeout_(timeout_seconds)
        , wheel_(static_cast<size_t>(timeout_ / kTimerInterval))
        , current_bucket_(0)
    {
        // 打开定时器进行周期检查
        timer_id_ = loop_->RunEvery(kTimerInterval, 
            std::bind(&TimingWheel::OnTimer, this));
    }

    TimingWheel::~TimingWheel()
    {
        loop_->CancelTimer(timer_id_);
    }

    TimingWheel::Entry::~Entry()
    {
        if (conn_.expired()) 
        {
            return;
        }
        auto conn = conn_.lock();
        conn->SetContext(nullptr);
        conn->ShutDown();
    }

    void TimingWheel::Feed(const TcpConnectionPtr& conn)
    {
        loop_->AssertInLoopThread();

        EntryPtr entry = nullptr;
        if (conn->GetContext()) 
        {
            auto wp = std::static_pointer_cast<std::weak_ptr<Entry>>(conn->GetContext());
            entry = wp->lock();
        }
        
        // 如果是第一次进入，或者旧的 Entry 已经因为超时正在析构中
        if (!entry) 
        {
            entry = std::make_shared<Entry>(conn);
            conn->SetContext(std::make_shared<std::weak_ptr<Entry>>(entry));
        }

            wheel_[current_bucket_].insert(entry);
        }

    void TimingWheel::OnTimer()
    {
        current_bucket_ = (current_bucket_ + 1) % wheel_.size();

        if (!wheel_[current_bucket_].empty()) 
        {
            wheel_[current_bucket_].clear();
        }
    }
        
} // namespace mini_muduo