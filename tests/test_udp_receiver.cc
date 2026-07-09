#include "mini_muduo/EventLoop.h"
#include "mini_muduo/InetAddress.h"
#include "mini_muduo/UdpDatagram.h"
#include "mini_muduo/UdpReceiver.h"
#include <cstddef>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace  
{
    int SendUdpTo(const mini_muduo::InetAddress& address,
        const void* data,
        std::size_t size)
    {
        int fd = ::socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP);
        assert(fd >= 0);

        ssize_t n = ::sendto(
            fd,
            data,
            size,
            0,
            reinterpret_cast<const sockaddr*>(address.GetSockAddr()),
            sizeof(sockaddr_in));

        ::close(fd);
        return static_cast<int>(n);
    }

    void TestUdpReceiverNormal()
    {
        mini_muduo::EventLoop loop;
        mini_muduo::InetAddress listen_address(25000);
        mini_muduo::UdpReceiver receiver(&loop, listen_address);

        std::vector<mini_muduo::UdpDatagram> received;

        receiver.SetMessageCallback([&](mini_muduo::UdpDatagram datagram)
        {
            std::cout << "udp data: "
                << std::string(
                    reinterpret_cast<const char*>(datagram.data()),
                    datagram.size()) << std::endl;
            received.push_back(std::move(datagram));
            loop.Quit();
        });

        receiver.Start();

        const char msg[] = "hello";
        assert(SendUdpTo(listen_address, msg, 5) == 5);

        loop.RunAfter(2.0, [&](){
            loop.Quit();
        });

        loop.Loop();

        assert(received.size() == 1);

        std::string payload(
        reinterpret_cast<const char*>(received[0].data()),
        received[0].size());

        assert(payload == "hello");
        assert(!received[0].truncated());
        assert(received[0].original_size() == 5);
        assert(received[0].size() == 5);
    }

    void TestUdpReceiverTruncated()
    {
        mini_muduo::EventLoop loop;
        mini_muduo::InetAddress listen_address(25000);
        mini_muduo::UdpReceiver receiver(&loop, listen_address, 4);

        std::vector<mini_muduo::UdpDatagram> received;

        receiver.SetMessageCallback([&](mini_muduo::UdpDatagram datagram)
        {
            std::cout << "udp data: "
                << std::string(
                    reinterpret_cast<const char*>(datagram.data()),
                    datagram.size()) << std::endl;
            received.push_back(std::move(datagram));
            loop.Quit();
        });

        receiver.Start();

        const char msg[] = "hello";
        assert(SendUdpTo(listen_address, msg, 5) == 5);

        loop.RunAfter(2.0, [&](){
            loop.Quit();
        });

        loop.Loop();

        assert(received.size() == 1);

        std::string payload(
        reinterpret_cast<const char*>(received[0].data()),
        received[0].size());

        assert(payload == "hell");
        assert(received[0].truncated());
        assert(received[0].original_size() == 5);
        assert(received[0].size() == 4);
    }

    void TestUdpReceiverZeroUdpDatagram()
    {
        mini_muduo::EventLoop loop;
        mini_muduo::InetAddress listen_address(25000);
        mini_muduo::UdpReceiver receiver(&loop, listen_address);

        std::vector<mini_muduo::UdpDatagram> received;

        receiver.SetMessageCallback([&](mini_muduo::UdpDatagram datagram)
        {
            std::cout << "udp data: "
                << std::string(
                    reinterpret_cast<const char*>(datagram.data()),
                    datagram.size()) << std::endl;
            received.push_back(std::move(datagram));
            loop.Quit();
        });

        receiver.Start();
        assert(SendUdpTo(listen_address, "", 0) == 0);

        loop.RunAfter(2.0, [&](){
            loop.Quit();
        });

        loop.Loop();

        assert(received.size() == 1);

        std::string payload(
        reinterpret_cast<const char*>(received[0].data()),
        received[0].size());

        assert(payload == "");
        assert(!received[0].truncated());
        assert(received[0].original_size() == 0);
        assert(received[0].size() == 0);
    }

    void TestUdpReceiverMultipleDatagrams()
    {
        mini_muduo::EventLoop loop;
        mini_muduo::InetAddress listen_address(25000);
        mini_muduo::UdpReceiver receiver(&loop, listen_address);

        std::vector<mini_muduo::UdpDatagram> received;

        receiver.SetMessageCallback([&](mini_muduo::UdpDatagram datagram)
        {
            std::cout << "udp data: "
                << std::string(
                    reinterpret_cast<const char*>(datagram.data()),
                    datagram.size()) << std::endl;
            received.push_back(std::move(datagram));
            if (received.size() == 3) 
            {
                loop.Quit();
            }
        });

        receiver.Start();
        std::string send_data[3] = {"one", "two", "three"};
        assert(SendUdpTo(listen_address, send_data[0].data(), 
            send_data[0].size()) == static_cast<int>(send_data[0].size()));
        assert(SendUdpTo(listen_address, send_data[1].data(), 
            send_data[1].size()) == static_cast<int>(send_data[1].size()));
        assert(SendUdpTo(listen_address, send_data[2].data(), 
            send_data[2].size()) == static_cast<int>(send_data[2].size()));

        loop.RunAfter(2.0, [&](){
            loop.Quit();
        });

        loop.Loop();

        assert(received.size() == 3);

        for (std::size_t i = 0; i < received.size(); ++i) 
        {
            std::string payload(
            reinterpret_cast<const char*>(received[i].data()),
            received[i].size());

            assert(payload == send_data[i]);
            assert(!received[i].truncated());
            assert(received[i].original_size() == send_data[i].size());
            assert(received[i].size() == send_data[i].size());
        }
    }
}

int main()
{
    TestUdpReceiverNormal();
    TestUdpReceiverTruncated();
    TestUdpReceiverZeroUdpDatagram();
    TestUdpReceiverMultipleDatagrams();

    return 0;
}