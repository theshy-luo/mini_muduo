#include "mini_muduo/Timer.h"
#include "mini_muduo/base/Timestamp.h"

namespace mini_muduo 
{
    std::atomic<int64_t> Timer::g_num_created_(0);

    Timer::Timer(Timestamp expiration, TimerCallback cb, double interval) 
        : expiration_(expiration)
        , callback_(cb)
        , interval_(interval)
        , repeat_(interval > 0)
        , sequence_(g_num_created_++)  // 静态成员原子递增
        {}

    Timer::~Timer() {}

    void Timer::Reset(Timestamp now)
    {
        if (repeat_) 
        {
            expiration_ = AddTime(now, interval_);
        }
        else 
        {
            expiration_ = Timestamp::Invalid();
        }
    }

} // namespace mini_muduo