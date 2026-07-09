#include "mini_muduo/PacketBufferPool.h"

namespace  
{
    void TestPacketBufferPoolReuse()
    {
        mini_muduo::PacketBufferPool pool(1500);

        auto first = pool.Acquire();
        auto* first_raw = first.get();

        first.reset();

        auto second = pool.Acquire();
        assert(second.get() == first_raw);
    }
}

int main()
{
    TestPacketBufferPoolReuse();
    return 0;
}