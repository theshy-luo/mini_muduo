#pragma once

#include "mini_muduo/base/noncopyable.h"
#include <unordered_map>
#include <vector>

namespace mini_muduo {
class Channel;
class EventLoop;

// 多路复用接口类 (抽象基类)
// muduo 使用 IO多路复用 的分发器
class Poller : public Noncopyable {
public:
  using ChannelList = std::vector<Channel *>;

  Poller(EventLoop *loop);
  virtual ~Poller() = default;

  // 给所有活跃的 Channel 填到 activeChannels 里去
  // 必须由派生类实现 (比如 EpollPoller)
  virtual int Poll(int timeoutMs, ChannelList *activeChannels) = 0;

  // 更新 Channel 在内核中的监听状态 (EPOLL_CTL_ADD / MOD)
  virtual void UpdateChannel(Channel *channel) = 0;

  // 从内核移除 Channel (EPOLL_CTL_DEL)
  virtual void RemoveChannel(Channel *channel) = 0;

  // 判断一个 Channel 是不是还在当前 Poller 中
  bool hasChannel(Channel *channel) const;

  void AssertInLoopThread() const;

protected:
  // 保存 sockfd 及其对应的 Channel 的映射
  using ChannelMap = std::unordered_map<int, Channel *>;
  ChannelMap channels_;

private:
  EventLoop *ownerLoop_;
};
} // namespace mini_muduo
