#include "mini_muduo/base/Timestamp.h"

#include <inttypes.h>

namespace mini_muduo 
{
    std::string Timestamp::ToString() const
    {
        char buf[32] = {0};
        int64_t seconds = micro_seconds_since_epoch_ / kMicroSecondsPerSecond;
        int64_t microseconds = micro_seconds_since_epoch_ % kMicroSecondsPerSecond;
        snprintf(buf, sizeof(buf), "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
        return buf;
    }
}