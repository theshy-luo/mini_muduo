#include "mini_muduo/Acceptor.h"
#include "mini_muduo/EventLoop.h"
#include "mini_muduo/InetAddress.h"
#include "mini_muduo/Socket.h"
#include "mini_muduo/base/Timestamp.h"

#include <cassert>
#include <functional>
#include <iostream>
#include <unistd.h>

namespace mini_muduo {
Acceptor::Acceptor(EventLoop *loop, const InetAddress &listenAddr,
                   bool reuseport)
    : loop_(loop),
      acceptSocket_(::socket(listenAddr.family(),
                             SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)),
      listening_(false), accept_channel_(loop, acceptSocket_.get_fd()) {
  acceptSocket_.SetReuseAddr(reuseport);
  acceptSocket_.BindAddress(listenAddr);
  accept_channel_.SetReadCallback([this](Timestamp) { HandleRead(); });
}

Acceptor::~Acceptor() 
{ 
    accept_channel_.DisableAll();
    accept_channel_.Remove(); 
}

void Acceptor::listen() {
  loop_->AssertInLoopThread();
  listening_ = true;
  acceptSocket_.Listen();
  accept_channel_.EnableReading();
}

void Acceptor::HandleRead() {
  loop_->AssertInLoopThread();
  InetAddress peeraddr;
  int conn_fd = acceptSocket_.Accept(&peeraddr);
  if (conn_fd >= 0) {
    if (newConnectionCallback_) {
      newConnectionCallback_(conn_fd, peeraddr);
    } else {
      std::cerr << "Acceptor::HandleRead no newConnectionCallback_" << "\n";
      ::close(conn_fd);
    }
  } else {
    std::cerr << "Acceptor::HandleRead accept error" << "\n";
  }
}

} // namespace mini_muduo