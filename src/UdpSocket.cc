#include "mini_muduo/UdpSocket.h"
#include <cerrno>
#include <sys/socket.h>

namespace mini_muduo 
{
    UdpSocket::UdpSocket()
    : fd_(::socket(AF_INET,
        SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
        IPPROTO_UDP))
    {
        if (!fd_.valid()) 
        {
            // 记录 errno
        }
    }

    UdpSocket::UdpSocket(sa_family_t family)
    : fd_(::socket(family,
        SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
        IPPROTO_UDP))
    {
        if (!fd_.valid()) 
        {
            // 记录 errno
        }
    }

    bool UdpSocket::BindAddress(const InetAddress& address)
    {
        const sockaddr_in* ipv4_address = address.GetSockAddr();

        return ::bind(
            fd_.get(),
            reinterpret_cast<const sockaddr*>(ipv4_address),
            sizeof(*ipv4_address)) == 0;
    }

    bool UdpSocket::SetReuseAddr(bool enable)
    {
        int value = enable ? 1 : 0;

        return setsockopt(fd_.get(), SOL_SOCKET, 
            SO_REUSEADDR, &value, sizeof(value)) == 0;
    }

    bool UdpSocket::SetReceiveBufferSize(int bytes)
    {
        if (bytes <= 0) 
        {
            errno = EINVAL;
            return false;
        }

        if (!fd_.valid()) 
        {
            errno = EBADF;
            return false;
        }

        return ::setsockopt(fd_.get(), SOL_SOCKET, 
        SO_RCVBUF, &bytes, 
        sizeof(bytes)) == 0;
    }
}