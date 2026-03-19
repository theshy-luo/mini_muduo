#pragma once

#include <cstdint>
#include <string>
#include <sys/time.h>

namespace mini_muduo 
{
    class Timestamp
    {
        public:
            Timestamp():micro_seconds_since_epoch_(-1)
            {};

            explicit Timestamp(int64_t microSecondsSinceEpoch)
                : micro_seconds_since_epoch_(microSecondsSinceEpoch) {}
            int64_t micro_seconds_since_epoch() const { return micro_seconds_since_epoch_; }
            int64_t milli_seconds_since_epoch() const { return micro_seconds_since_epoch_ / 1000; }

            static Timestamp Invalid() { return Timestamp(); }

            static Timestamp Now()
            {
                struct timeval tv;
    
                // 第二个参数通常传 nullptr，代表不获取时区信息
                gettimeofday(&tv, nullptr); 
                
                // 组合成一个完整的 64 位微秒级时间戳
                return Timestamp(static_cast<int64_t>(tv.tv_sec) * 1000000 + tv.tv_usec);
            }

            std::string ToString() const;

            static const int kMicroSecondsPerSecond = 1000 * 1000;
            
        private:
            int64_t micro_seconds_since_epoch_;
    };

    inline bool operator<(Timestamp lhs, Timestamp rhs)
    {
        return lhs.micro_seconds_since_epoch() < rhs.micro_seconds_since_epoch();
    }

    inline bool operator==(Timestamp lhs, Timestamp rhs)
    {
        return lhs.micro_seconds_since_epoch() == rhs.micro_seconds_since_epoch();
    }

    inline Timestamp AddTime(Timestamp timestamp, double seconds)
    {
        return Timestamp(timestamp.micro_seconds_since_epoch() + static_cast<int64_t>(seconds * 1000000));
    }
}