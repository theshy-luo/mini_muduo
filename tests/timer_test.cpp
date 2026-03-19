#include "mini_muduo/EventLoop.h"
#include "mini_muduo/base/Timestamp.h"

#include <iostream>

int main() 
{
    mini_muduo::EventLoop loop;
    
    // 1秒后执行一次
    std::cout << "100ms后执行一次 now:" << mini_muduo::Timestamp::Now().ToString() << "\n";
    loop.RunAfter(0.1, []{
        std::cout << "100ms到了！" << "\n";
    });
    
    // 每隔0.5秒重复执行
    auto id = loop.RunEvery(0.5, []{
        std::cout << "心跳 tick..." << "\n";
    });

    // 3 秒后自动取消心跳
    loop.RunAfter(3.0, [&id, &loop]{
        std::cout << "取消心跳！" << "\n";
        loop.CancelTimer(id);
    });

    loop.RunAfter(4.0, [&loop]{
        std::cout << "4秒到了，退出！" << "\n";
        loop.Quit();
    });
    
    loop.Loop();

    return 0;
}