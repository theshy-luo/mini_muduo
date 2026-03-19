#include "mini_muduo/EpollPoller.h"
#include "mini_muduo/Channel.h"
#include "mini_muduo/Poller.h"

#include <cstddef>
#include <cstdio>
#include <iostream>
#include <strings.h>
#include <sys/epoll.h>
#include <unistd.h>

namespace mini_muduo {
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)), // 创建内核中的 epoll 句柄
      events_(kInitEventListSize)               // 给装事件的篮子预留16个格子
{
  // 如果 epollfd_ < 0，说明内核建立失败，这是致命错误
  if (epollfd_ < 0) 
  {
    std::cerr << "epoll_create1 error!";
    abort();
  }
}

EpollPoller::~EpollPoller() { ::close(epollfd_); }

int EpollPoller::Poll(int timeoutMs, ChannelList *activeChannels) 
{
  // 1. 去内核那“等”（阻塞在这儿）。
  // 把 events_.data() (也就是底层数组首地址) 传给内核去装填活跃事件
  int numEvents = ::epoll_wait(epollfd_, events_.data(),
                               (static_cast<int>(events_.size())), timeoutMs);

  // 2. 如果拿到了事件
  if (numEvents > 0) 
  {
    // 拿到了多少个，就把前多少个从 event_.data() 翻译回 Channel* 塞给外面
    FillActiveChannels(numEvents, activeChannels);

    // 如果篮子装满了（说明这一波很猛），咱们下次把篮子扩容一倍

    if (static_cast<size_t>(numEvents) == events_.capacity()) 
    {
      events_.resize(events_.capacity() * 2);
    } 
    else if (static_cast<size_t>(numEvents) <= kInitEventListSize) 
    {
      events_.resize(kInitEventListSize);
    }
  } 
  else if (numEvents < 0) 
  {
    std::cerr << "epoll_wait error!";
    abort();
  }

  return numEvents;
}

void EpollPoller::FillActiveChannels(int numEvents,
                                     ChannelList *activeChannels) const 
{
  for (int i = 0; i < numEvents; ++i) 
  {
    // 强转回 Channel 指针！
    Channel *channel = static_cast<Channel *>(events_[i].data.ptr);

    // 原封不动地把内核告诉我们的实际事件 revents 塞给 Channel
    channel->set_revents(events_[i].events);

    // 推送到输出的名单里
    activeChannels->push_back(channel);
  }
}

void EpollPoller::UpdateChannel(Channel *channel) 
{
  Poller::AssertInLoopThread();
  const int index = channel->index();

  // 如果是新来的，或者曾经被删除了但是现在又要加回来
  if (index == kNew || index == kDeleted) 
  {
    int fd = channel->fd();
    if (index == kNew) 
    {
      channels_[fd] = channel;
    }

    // 把状态改成“已添加
    channel->set_index(kAdded);

    // 真正去调内核：EPOLL_CTL_ADD
    Update(EPOLL_CTL_ADD, channel);
  } 
  else // 已经在小黑板上了 (index == kAdded)
  {
    // 如果这个 Channel 已经没有任何关心的事件了 (events() == 0)
    if (channel->is_none_event()) 
    {
      // 从内核里删掉它，并把状态改成 kDeleted
      Update(EPOLL_CTL_DEL, channel);

      channel->set_index(kDeleted);
    } 
    else 
    {
      // 它还有关心的事件（可能原来只关心读，现在要求加个关心写）
      // 真正去调内核：EPOLL_CTL_MOD
      Update(EPOLL_CTL_MOD, channel);
    }
  }
}

void EpollPoller::RemoveChannel(Channel *channel) 
{
  Poller::AssertInLoopThread();
  const int index = channel->index();
  if (index == kAdded) 
  {
    Update(EPOLL_CTL_DEL, channel);
    channel->set_index(kDeleted);
  }
}

void EpollPoller::Update(int operation, Channel *channel) 
{
  struct epoll_event event;
  // 这点极其关键：把各种杂七杂八的内存清理干净，防止有脏数据
  bzero(&event, sizeof(event));

  // 把 Channel 自己关心的事件集合给 event
  event.events = channel->events();

  // 【核心心机】：把这个 Channel 的指针，悄悄塞进 epoll_event 的 data.ptr 里！
  // 这样 epoll_wait 触发时，我们就知道是哪个 Channel 来事了！
  event.data.ptr = channel;

  int fd = channel->fd();

  // 调用真正的内核 API！
  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) 
  {
    if (operation == EPOLL_CTL_DEL) 
    {
      std::cerr << "epoll_ctl op=" << operation << " fd=" << fd << " error!\n";
    } 
    else 
    {
      std::cerr << "Fatal! epoll_ctl op=" << operation << " fd=" << fd
                << " error!\n";
      abort();
    }
  }
}

} // namespace mini_muduo