#pragma once

#include "mini_muduo/Channel.h"
#include "mini_muduo/Socket.h"
#include "mini_muduo/base/noncopyable.h"

#include <functional>

namespace mini_muduo {
class EventLoop;
class InetAddress;

class Acceptor : public Noncopyable {
public:
  using NewConnectionCallback =
      std::function<void(int sockfd, const InetAddress &)>;

  Acceptor(EventLoop *loop, const InetAddress &listenAddr, bool reuseport);
  ~Acceptor();

  // 别人设置进来的回调（当有新连接时，Acceptor 会呼叫这个回调通知 TcpServer）
  void SetNewConnectionCallback(const NewConnectionCallback &cb) {
    newConnectionCallback_ = cb;
  }

  bool listening() const { return listening_; }

  void listen(); // 调用底层 Socket::listen，并开启 accept_channel_ 的读事件

private:
  void HandleRead(); // accept_channel_ 绑定的核心回呼函数：里面去调用 accept()!

  EventLoop *loop_;
  Socket acceptSocket_;
  bool listening_;

  // 它是大管家，管理着那个监听 fd 的生死
  Channel accept_channel_;
  // 它是传声筒，负责和 EventLoop 及 epoll 打交道
  NewConnectionCallback newConnectionCallback_;
};
} // namespace mini_muduo