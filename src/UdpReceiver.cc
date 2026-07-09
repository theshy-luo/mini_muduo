#include "mini_muduo/UdpReceiver.h"
#include "mini_muduo/Channel.h"
#include "mini_muduo/InetAddress.h"
#include "mini_muduo/UdpDatagram.h"
#include "mini_muduo/UdpSocket.h"
#include "mini_muduo/base/Logger.h"
#include "mini_muduo/base/Timestamp.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <system_error>
#include <utility>
#include <vector>

namespace mini_muduo 
{
    namespace  
    {
        std::size_t ValidateBufferSize(std::size_t size)
        {
            if (size == 0) 
            {
                throw std::invalid_argument(
                    "UdpReceiver max_datagram_size must be greater than zero");
            }

            return size;
        }

        void ThrowSystemError(const char* operation)
        {
            const int saved_error = errno;
            throw std::system_error(
                saved_error, std::generic_category(), operation);
        }
    }// namespace

    const std::size_t UdpReceiver::kDefaultMaxPacketPerRead;

    UdpReceiver::UdpReceiver(EventLoop* loop,
        const InetAddress& listen_address,
        std::size_t max_datagram_size,
        std::size_t initial_buffer_count,
        std::size_t max_packets_per_read)
    : loop_(loop), socket_(listen_address.family()), channel_(), 
      max_packets_per_read_(max_packets_per_read),
      buffer_pool_(ValidateBufferSize(max_datagram_size), initial_buffer_count),
      message_callback_(), started_(false)
    {
        if (loop_ == nullptr) 
        {
            throw std::invalid_argument(
                "UdpReceiver loop must not be null");
        }

        if (max_packets_per_read == 0)
        {
            throw std::invalid_argument(
                "UdpReceiver max_packets_per_read must be greater than zero");
        }

        if (!socket_.valid()) 
        {
            ThrowSystemError("socket");
        }

        if (!socket_.SetReuseAddr(true))
        {
            ThrowSystemError("setsockopt(SO_REUSEADDR)");
        }

        if (!socket_.BindAddress(listen_address)) 
        {
            ThrowSystemError("bind");
        }

        channel_.reset(new Channel(loop_, socket_.fd()));

        channel_->SetReadCallback([this](Timestamp receive_time)
        {
            HandleRead(receive_time);
        });
    }

    UdpReceiver::~UdpReceiver()
    {
        loop_->AssertInLoopThread();

        Stop();
    }

    void UdpReceiver::Start()
    {
        loop_->AssertInLoopThread();

        if (started_) 
        {
            return;
        }

        if (!message_callback_) 
        {
            throw std::logic_error("UdpReceiver callback must be set before Start");
        }

        channel_->EnableReading();
        started_ = true;
    }

    void UdpReceiver::Stop()
    {
        loop_->AssertInLoopThread();

        if (!started_) 
        {
            return;
        }

        channel_->DisableAll();
        channel_->Remove();
        started_ = false;
    }

    void UdpReceiver::HandleRead(Timestamp receive_time)
    {
        loop_->AssertInLoopThread();

        std::size_t packets_read = 0;

        while (packets_read < max_packets_per_read_) 
        {
            const std::size_t batch_size = std::min<std::size_t>(
                max_packets_per_read_ - packets_read, 
                kDefaultMaxPacketPerRead);
            
            // 预先准备好这一轮可以取出的数据报最大容量
            std::vector<UdpDatagram> datagrams;
            datagrams.reserve(batch_size);

            std::vector<sockaddr_in> peer_addresses(batch_size);
            std::vector<iovec> io_vectors(batch_size);
            std::vector<mmsghdr> messages(batch_size);

            std::memset(messages.data(), 0, sizeof(mmsghdr) * messages.size());

            for (std::size_t i = 0; i < batch_size; ++i) 
            {
                UdpDatagram datagram(buffer_pool_.Acquire());

                io_vectors[i].iov_base = datagram.writable_data();
                io_vectors[i].iov_len = datagram.capacity();

                messages[i].msg_hdr.msg_name = &peer_addresses[i];
                messages[i].msg_hdr.msg_namelen  = sizeof(peer_addresses[i]);
                messages[i].msg_hdr.msg_iov  = &io_vectors[i];
                messages[i].msg_hdr.msg_iovlen  = 1;

                datagrams.push_back(std::move(datagram));
            }

            const int received_count = ::recvmmsg(socket_.fd(), messages.data(), 
                static_cast<unsigned int>(messages.size()), 
                MSG_DONTWAIT | MSG_TRUNC, nullptr);
            
            if (received_count < 0) 
            {
                const int saved_error = errno;

                if (saved_error == EINTR) 
                {
                    continue;
                }

                if (saved_error == EAGAIN || saved_error == EWOULDBLOCK) 
                {
                    return;
                }

                LOG_DEBUG << "UdpReceiver::HandleRead recvmmsg error, errno=" << saved_error;
                return;
            }

            if (received_count == 0)
            {
                return;
            }

            packets_read += static_cast<std::size_t>(received_count);

            for (int i = 0; i < received_count; ++i) 
            {
                UdpDatagram& datagram = datagrams[static_cast<std::size_t>(i)];

                const std::size_t original_size = static_cast<std::size_t>(messages[i].msg_len);

                const std::size_t readable_size = std::min(original_size, datagram.capacity());

                const bool truncated = (messages[i].msg_hdr.msg_flags & MSG_TRUNC) != 0 ||
                    original_size > datagram.capacity();

                datagram.SetOriginalSize(original_size);
                datagram.SetSize(readable_size);
                datagram.SetPeer(InetAddress(peer_addresses[static_cast<std::size_t>(i)]));
                datagram.SetReceiveTime(receive_time);
                datagram.SetTruncated(truncated);

                if (message_callback_) 
                {
                    message_callback_(std::move(datagram));
                }

                if (!started_) 
                {   
                    return;
                }
            }

            if (static_cast<std::size_t>(received_count) < batch_size)
            {
                return;
            }
        }
    }
}
