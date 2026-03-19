#include "mini_muduo/Channel.h"
#include "mini_muduo/EventLoop.h"
#include "mini_muduo/base/Timestamp.h"
#include <cassert>

namespace mini_muduo {
    // 定义三个常量，代表 Channel 感兴趣的具体事件
    // EPOLLIN: 表示对应的 fd 有数据可读
    // EPOLLPRI: 表示对应的 fd 有紧急数据可读（通常连同读一块儿注册）
    const int Channel::kNoneEvent = 0;
    const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
    const int Channel::kWriteEvent = EPOLLOUT;

    Channel::Channel(EventLoop *loop, int fd)
        : tied_(false), loop_(loop), fd_(fd), 
        events_(kNoneEvent), // 刚生下来，什么事件也不关心
        revents_(kNoneEvent), // 也什么事件都没发生过
         // -1 表示还没被加到 Poller（也就是内核）的事件表中
        index_(-1)
    {}

    // 开启读事件监听（让这根管道开始关心读事件）
    void Channel::EnableReading() 
    {
        // 把 kReadEvent 的二进制位通过 |= 加到现在的 events_ 里面
        events_ |= kReadEvent;

        // 通知EventLoop
        Update();
    }

    void Channel::EnableWriting() 
    {
        events_ |= kWriteEvent;

        // 通知EventLoop
        Update();
    }

    void Channel::Remove()
    {
        assert(is_none_event());
        loop_->RemoveChannel(this);
    }

    void Channel::Update() { loop_->UpdateChannel(this); }

    void Channel::HandleEvent(Timestamp receive_time) 
    {
        if (tied_) 
        {
            // 关键：尝试把弱引用提升为强引用（保命符）
            std::shared_ptr<void> guard = tie_.lock();
            if (guard)
            {
                // 只要 guard 还在栈上，引用计数就至少为 1
                // 哪怕回调里执行了 erase，对象也只是在等 guard 销毁时才真正解体
                HandleEventWithGuard(receive_time);
            }
        }
        else
        {
            HandleEventWithGuard(receive_time);
        }
    }

    void Channel::HandleEventWithGuard(Timestamp receive_time) 
    {
        // 如果系统报告该 socket 发生了挂起（被对方关了，且没数据可读）
        // 并且没有触发读事件，说明连接真断了，呼叫 close 回调！
        if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) 
        {
            if (close_callback_) 
            {
                close_callback_();
            }
        }

        // 如果系统报告出错了！
        if (revents_ & EPOLLERR) 
        {
            if (error_callback_) 
            {
                error_callback_();
            }
        }

        // ⭐ 如果可写了（内核发送缓冲区有空位了）！
        if (revents_ & EPOLLOUT) 
        {
            if (write_callback_) 
            {
                write_callback_();
            }
        }

        // ⭐
        // 最常见：如果有数据可读，或者有紧急数据，甚至对方半关闭了（EPOLLRDHUP），都调读回调！
        if (revents_ & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) 
        {
            if (read_callback_) 
            {
                read_callback_(receive_time);
            }
        }
    }

} // namespace mini_muduo