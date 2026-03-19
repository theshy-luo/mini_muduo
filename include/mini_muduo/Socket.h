#pragma once

#include "mini_muduo/base/noncopyable.h"

#include <netinet/in.h>

namespace mini_muduo {
    class InetAddress;

    class Socket : public Noncopyable {
        public:
            // 构造函数只负责接管这个 fd
            explicit Socket(int sockfd) : sockfd_(sockfd) {};

            // 析构的时候负责 ::close(sockfd_)
            ~Socket();

            int get_fd() const { return sockfd_; }

            // 绑定地址
            void BindAddress(const InetAddress &localaddr);

            // 开始监听
            void Listen();

            /// 成功时，返回一个非负整数，该整数是
            /// 已接受套接字的描述符，该套接字已被
            /// 设置为非阻塞和执行时关闭。*peeraddr 被赋值。
            /// 出错时，返回 -1，*peeraddr 保持不变。
            int Accept(InetAddress *peeraddr);

            // 设置端口复用 (SO_REUSEADDR, SO_REUSEPORT 等)
            void SetReuseAddr(bool on);
            // void setReusePort(bool on);
            // void setKeepAlive(bool on);

            void ShutdownWrite();

        private:
            int sockfd_;
    };
} // namespace mini_muduo