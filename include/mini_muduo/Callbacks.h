#pragma once

#include "mini_muduo/base/Timestamp.h"

#include <memory>
#include <functional>

namespace mini_muduo 
{
    // 提前声明两个核心类，不然这里用不了它们的指针
    class Buffer;
    class TcpConnection;
    class Timestamp;
    class EventLoop;

    using ThreadInitCallback = std::function<void(EventLoop*)>;

    using TimerCallback = std::function<void()>;

    // 利用 std::shared_ptr 管理连接的生命周期！
    using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

    using TimerCallback = std::function<void()>;
    using ConnectionCallback = std::function<void (const TcpConnectionPtr&)>;
    using CloseCallback = std::function<void (const TcpConnectionPtr&)>;
    using WriteCompleteCallback = std::function<void (const TcpConnectionPtr&)>;

    // 消息到来时的回调（参数：连接智能指针，缓冲区指针，接收时间）
    // 对于 EchoServer 来说，绝大部逻辑就在这一个回调里
    using MessageCallback = std::function<void (const TcpConnectionPtr&, Buffer *, Timestamp)>;
}

#include "mini_muduo/TcpConnection.h"
