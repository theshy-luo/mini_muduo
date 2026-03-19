#include "mini_muduo/Channel.h"
#include "mini_muduo/EventLoop.h"
#include "mini_muduo/base/Timestamp.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

using namespace mini_muduo;

int main() {
  EventLoop loop;
  std::vector<Channel *> clinetChannel(10);

  // 1. 创建一个监听 socket
  int listenFd =
      ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);

  // 2. 绑定端口
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(12347);
  addr.sin_addr.s_addr = INADDR_ANY;
  int res = ::bind(listenFd, (struct sockaddr *)&addr, sizeof(addr));
  if (res == -1) {
    std::cerr << "socket bind fail errno is " << errno << "\n";
    return 0;
  }

  // 开始监听
  ::listen(listenFd, SOMAXCONN);
  std::cout << "Echo Server listening on port 12347...\n";

  // 4. 用我们的 Reactor！给监听 fd 穿上 Channel 制服
  Channel listenChannel(&loop, listenFd);

  // 5. 设置读回调：有新连接进来时触发
  listenChannel.SetReadCallback([&](Timestamp receive_time) 
  {
    // accept 新连接
    struct sockaddr_in clinetAddr;
    socklen_t len = sizeof(clinetAddr);
    int connFd = ::accept4(listenFd, (struct sockaddr *)&clinetAddr, &len,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connFd < 0) 
    {
      std::cerr << "accept4 fail \n";
      return;
    }
    std::cout << "New connection fd=" << connFd << " receive_time=" 
      << receive_time.milli_seconds_since_epoch() << "\n";

    // 给新连接也穿上 Channel 制服（注意：这里要用 new，因为 lambda
    // 结束后要存活）
    Channel *connChannel = new Channel(&loop, connFd);
    clinetChannel.push_back(connChannel);
    connChannel->SetReadCallback([connFd, connChannel, &loop](Timestamp receive_time) 
    {
      char buf[1024];
      ssize_t n = ::read(connFd, buf, sizeof(buf));
      if (n > 0) 
      {
        ::write(connFd, buf, n); // Echo！收到啥就发回啥！
      } 
      else 
      {
        std::cout << "Connection closed fd=" << connFd << " receive_time=" 
          << receive_time.milli_seconds_since_epoch() << "\n";
        loop.RemoveChannel(connChannel);
      }
    });

    connChannel->EnableReading(); // 开始监听这个连接的读事件
  });

  // 6. 让监听 channel 开始监听读事件（有新连接 = 可读事件）
  listenChannel.EnableReading();

  loop.Loop();

  for (Channel *channel : clinetChannel) {
    delete channel;
  }
  ::close(listenFd);

  return 0;
}
