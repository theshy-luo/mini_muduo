#include "mini_muduo/Poller.h"
#include "mini_muduo/Channel.h"
#include "mini_muduo/EventLoop.h"

namespace mini_muduo
{
    Poller::Poller(EventLoop* loop)
        : ownerLoop_(loop)
    {
    }

    bool Poller::hasChannel(Channel* channel) const
    {
        auto it = channels_.find(channel->fd());
        return it != channels_.end() && it->second == channel;
    }

    void Poller::AssertInLoopThread() const
    {
        ownerLoop_->AssertInLoopThread();
    }
}
