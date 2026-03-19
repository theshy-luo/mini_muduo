#pragma once

#include "mini_muduo/base/noncopyable.h"

#include <functional>
#include <sys/epoll.h>
#include <memory>

namespace mini_muduo 
{
    class EventLoop;
    class Timestamp;
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    class Channel : public Noncopyable 
    {
        public:
            Channel(EventLoop *loop, int fd);
            ~Channel() = default;

            // 开启保活机制
            void Tie(const std::shared_ptr<void>& obj)
            {
                tie_ = obj;
                tied_ = true;
            }

            // epoll 被唤醒后，Poller 会调用此方法，根据 revents_ 内部决定触发哪个回调
            void HandleEvent(Timestamp receive_time);

            void SetReadCallback(ReadEventCallback cb) { read_callback_ = std::move(cb); }
            void SetWriteCallback(EventCallback cb) { write_callback_ = std::move(cb); }
            void SetErrorCallback(EventCallback cb) { error_callback_ = std::move(cb); }
            void SetCloseCallback(EventCallback cb) { close_callback_ = std::move(cb); }

            // getter
            int fd() const { return fd_; }
            int events() const { return events_; }
            int revents() const { return revents_; }

            // 给 Poller 用，告诉它 epoll 实际返回了啥
            void set_revents(int revents) { revents_ = revents; }

            // channel 状态：标识它是否已经被放进 epoll 或者被移出
            int index() const { return index_; }
            void set_index(int index) { index_ = index; }

            // 供 Poller 调用的判断接口
            bool is_none_event() const { return events_ == kNoneEvent; }
            bool is_writing() const { return events_ & kWriteEvent; }
            bool is_reading() const { return events_ & kReadEvent; }

            // channel结束对写事件的监控
            void DisableWriting() 
            {
                events_ &= ~kWriteEvent;
                Update();
            }
            // channel结束对所有事件的监控
            void DisableAll() 
            {
                events_ = kNoneEvent;
                Update();
            }

            // 设置对什么事件感兴趣
            void EnableReading();
            // void disableReading();
            void EnableWriting();

            // 从 epoll 移除
            void Remove();

        private:
            static const int kNoneEvent;
            static const int kReadEvent;
            static const int kWriteEvent;

            void HandleEventWithGuard(Timestamp receive_time);

            // 弱引用，用于保活
            std::weak_ptr<void> tie_;
            // 标识是否已经开启了保活机制
            bool tied_;

            EventLoop *loop_;
            int fd_;
            // 期望监听的事件
            int events_;
            // 实际发生的事件
            int revents_;
            // 在 Poller 中的索引
            int index_;
            // 读回调
            ReadEventCallback read_callback_;
            // 写回调
            EventCallback write_callback_;
            // 错误回调
            EventCallback error_callback_;
            // 析构回调
            EventCallback close_callback_;

            void Update();
        };

} // namespace mini_muduo
