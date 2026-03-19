#include "mini_muduo/Socket.h"
#include "mini_muduo/InetAddress.h"

#include <arpa/inet.h>
#include <iostream>
#include <netinet/in.h>
#include <strings.h>
#include <unistd.h>

namespace mini_muduo {
Socket::~Socket() { ::close(sockfd_); }

void Socket::BindAddress(const InetAddress &localaddr) {
  int res = ::bind(sockfd_, (struct sockaddr *)localaddr.GetSockAddr(),
                   sizeof(struct sockaddr_in));
  if (res == -1) {
    std::cerr << "socket bind fail errno is " << errno << "\n";
    return;
  }
}

void Socket::Listen() {
  int res = ::listen(sockfd_, SOMAXCONN);
  if (res == -1) {
    std::cerr << "socket listen fail errno is " << errno << "\n";
    return;
  }
}

int Socket::Accept(InetAddress *peeraddr) {
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  socklen_t len = sizeof(addr);
  int conn_fd = ::accept4(sockfd_, (struct sockaddr *)&addr, &len,
                          SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (conn_fd >= 0) {
    peeraddr->SetSockAddr(addr);
  }
  return conn_fd;
}

    void Socket::SetReuseAddr(bool on) 
    {
        int optval = on ? 1 : 0;
        int res = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval,
                                static_cast<socklen_t>(sizeof optval));
        if (res < 0)   
        {
            std::cerr << "socket setsockopt (SOL_SOCKET, SO_REUSEADDR) fail errno is "
                    << errno << "\n";
            return;
        }
    }

    void Socket::ShutdownWrite()
    {
        int res = ::shutdown(sockfd_, SHUT_WR);
        if (res < 0) 
        {
            std::cerr << "socket shutdownWrite fail errno is " << errno << "\n";
        }
    }
} // namespace mini_muduo