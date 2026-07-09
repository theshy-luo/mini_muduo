#include "mini_muduo/InetAddress.h"
#include "mini_muduo/UdpSocket.h"
#include <cassert>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace  
{
    void TestCreationAndFlags()
    {
        mini_muduo::UdpSocket socket;

        assert(socket.valid());
        assert(socket.fd() >= 0);

        int status_flags = ::fcntl(socket.fd(), F_GETFL);
        assert(status_flags >= 0);
        assert((status_flags & O_NONBLOCK) != 0);

        int descriptor_flags = ::fcntl(socket.fd(), F_GETFD);
        assert(descriptor_flags >= 0);
        assert((descriptor_flags & FD_CLOEXEC) != 0);

        int socket_type = 0;
        socklen_t length = sizeof(socket_type);

        assert(::getsockopt(socket.fd(), SOL_SOCKET, 
            SO_TYPE, &socket_type, &length) == 0);

        assert(socket_type == SOCK_DGRAM);
    }

    void TestOptionsAndBind()
    {
        mini_muduo::UdpSocket socket;

        assert(socket.SetReuseAddr(true));
        assert(socket.SetReceiveBufferSize(64 * 1024));

        mini_muduo::InetAddress local_address(0, "127.0.0.1");
        assert(socket.BindAddress(local_address));

        sockaddr_in bound_address{};
        socklen_t length = sizeof(bound_address);

        assert(::getsockname(socket.fd(), reinterpret_cast<sockaddr*>(&bound_address), 
            &length) == 0);

        // 绑定端口 0 后，内核应分配一个实际端口。
        assert(ntohs(bound_address.sin_port) != 0);

        int receive_buffer_size = 0;
        length = sizeof(receive_buffer_size);

        assert(::getsockopt(socket.fd(), SOL_SOCKET, 
            SO_RCVBUF, &receive_buffer_size, &length) == 0);

        assert(receive_buffer_size > 0);
    }

    void TestInvalidReceiveBufferSize()
    {
        mini_muduo::UdpSocket socket;

        errno = 0;
        assert(!socket.SetReceiveBufferSize(0));
        assert(errno == EINVAL);
    }

    void TestDestructorClosesFd()
    {
        int raw_fd = -1;

        {
            mini_muduo::UdpSocket socket;
            assert(socket.valid());
            raw_fd = socket.fd();
        }

        errno = 0;
        assert(::fcntl(raw_fd, F_GETFD) == -1);
        assert(errno == EBADF);
    }
}

int main()
{
    TestCreationAndFlags();
    TestOptionsAndBind();
    TestInvalidReceiveBufferSize();
    TestDestructorClosesFd();
    return 0;
}