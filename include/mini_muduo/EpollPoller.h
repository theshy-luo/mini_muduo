#pragma once

#include "mini_muduo/Channel.h"
#include "mini_muduo/EventLoop.h"
#include "mini_muduo/Poller.h"

namespace mini_muduo {
class EpollPoller : public Poller {
public:
  EpollPoller(EventLoop *loop);
  ~EpollPoller() override;

  // 重写基类的纯虚函数（override 关键字是 C++11 好习惯）
  int Poll(int timeoutMs, ChannelList *activeChannels) override;
  void UpdateChannel(Channel *channel) override;
  void RemoveChannel(Channel *channel) override;

private:
  // 初始化 epoll_wait 放活跃事件的数组大小，通常给个不大不小的常数
  static const int kInitEventListSize = 16;

  // 这是一个内部辅助函数：真正在内核跑 epoll_ctl
  void Update(int operation, Channel *channel);

  // 如果 epoll_wait 拿到了一堆活跃事件，需要把它们填充到 activeChannels 中返回
  void FillActiveChannels(int numEvents, ChannelList *activeChannels) const;

  // epoll 实例的文件描述符
  int epollfd_;

  // 用于给 epoll_wait 传出拿到的活跃事件集合
  using EventList = std::vector<struct epoll_event>;
  EventList events_;
};
} // namespace mini_muduo