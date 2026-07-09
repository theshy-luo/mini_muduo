#pragma once

#include "mini_muduo/Channel.h"
#include "mini_muduo/EventLoop.h"
#include "mini_muduo/InetAddress.h"
#include "mini_muduo/UdpDatagram.h"
#include "mini_muduo/UdpSocket.h"
#include "mini_muduo/base/Timestamp.h"
#include "mini_muduo/base/noncopyable.h"
#include "mini_muduo/PacketBufferPool.h"

#include <cstddef>
#include <memory>
#include <utility>

namespace mini_muduo 
{
    class UdpReceiver : public Noncopyable
    {
        public:
            static const std::size_t kDefaultMaxPacketPerRead = 64;
            UdpReceiver(        
                EventLoop* loop,
                const InetAddress& listen_address,
                std::size_t max_datagram_size = PacketBuffer::kDefaultCapacity,
                std::size_t initial_buffer_count = PacketBufferPool::kDefaultInitialBufferCount,
                std::size_t max_packets_per_read = kDefaultMaxPacketPerRead);

            ~UdpReceiver();

            void SetMessageCallback(UdpMessageCallback callback)
            {
                message_callback_ = std::move(callback);
            }

            void Start();
            void Stop();

            bool started() const noexcept { return started_; }
            int fd() const noexcept { return socket_.fd(); }

        private:
            void HandleRead(Timestamp receive_time);

            EventLoop* loop_;
            UdpSocket socket_;

            // 创建 socket、设置选项并成功 bind 后才能创建 Channel。
            std::unique_ptr<Channel> channel_;

            // 一次最多从udp socket读出来的包数
            std::size_t max_packets_per_read_;

            PacketBufferPool buffer_pool_;

            UdpMessageCallback message_callback_;
            bool started_;
    };
} // namespace mini_muduo