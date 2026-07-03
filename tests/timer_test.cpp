#include "mini_muduo/EventLoop.h"
#include "mini_muduo/base/Logger.h"
#include "mini_muduo/base/Timestamp.h"

int main()
{
    mini_muduo::EventLoop loop;

    // 1秒后执行一次
    LOG_DEBUG << "100ms后执行一次 now:" << mini_muduo::Timestamp::Now().ToString()
        << "\n";
    loop.RunAfter(0.1, [] {
        LOG_DEBUG << "100ms到了！"
                << "\n";
    });

    // 每隔0.5秒重复执行
    auto id = loop.RunEvery(0.5, [] {
        LOG_DEBUG << "心跳 tick..."
                << "\n";
    });

    // 3 秒后自动取消心跳
    loop.RunAfter(3.0, [&id, &loop] {
        LOG_DEBUG << "取消心跳！"
                << "\n";
        loop.CancelTimer(id);
    });

    loop.RunAfter(4.0, [&loop] {
        LOG_DEBUG << "4秒到了，退出！"
                << "\n";
        loop.Quit();
    });

    loop.Loop();

    return 0;
}