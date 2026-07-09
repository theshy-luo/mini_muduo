#pragma once

#include "mini_muduo/InetAddress.h"
#include "mini_muduo/base/UniqueFd.h"

namespace mini_muduo 
{
    class UdpSocket : public Noncopyable
    {
        public:
            UdpSocket();
            explicit UdpSocket(sa_family_t family);
            ~UdpSocket() = default;

        int fd() const noexcept { return fd_.get(); }
        bool valid() const noexcept { return fd_.valid(); }

        bool BindAddress(const InetAddress& address);
        bool SetReuseAddr(bool enable);
        bool SetReceiveBufferSize(int bytes);

        private:
            UniqueFd fd_;
    };
}