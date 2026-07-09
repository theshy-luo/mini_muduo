#pragma once

#include "mini_muduo/InetAddress.h"
#include "mini_muduo/base/Timestamp.h"
#include "mini_muduo/PacketBuffer.h"

#include <functional>

// UdpDatagram 持有共享 PacketBuffer。
// data() 返回只读 payload 指针；只要 UdpDatagram 副本仍存在，payload 就保持有效。
// 回调不要保存裸指针或引用；需要长期持有时应保存 UdpDatagram 值。

namespace mini_muduo 
{
    class UdpDatagram
    {
        public:
            const std::uint8_t* data() const noexcept
            {
                return buffer_->data();
            }
            std::size_t size() const noexcept
            {
                return buffer_->size();
            }
            std::size_t capacity() const noexcept
            {
                return buffer_->capacity();
            }

            const InetAddress& peer() const noexcept
            {
                return peer_;
            }
            Timestamp receive_time() const noexcept
            {
                return receive_time_;
            }

            std::size_t original_size() const noexcept
            {
                return original_size_;
            }
            bool truncated() const noexcept
            {
                return truncated_;
            }

        private:
            friend class UdpReceiver;

            explicit UdpDatagram(std::shared_ptr<PacketBuffer> buffer)
            : buffer_(std::move(buffer))
            {}

            void SetPeer(const InetAddress& peer) noexcept
            {
                peer_ = peer;
            }

            void SetReceiveTime(const Timestamp& receive_time) noexcept
            {
                receive_time_ = receive_time;
            }

            void SetOriginalSize(const std::size_t original_size) noexcept
            {
                original_size_ = original_size;
            }

            void SetTruncated(bool truncated) noexcept
            {
                truncated_ = truncated;
            }

            void SetSize(const std::size_t size)
            {
                buffer_->set_size(size);
            }

            std::uint8_t* writable_data() noexcept
            {
                return buffer_->writable_data();
            }

            std::shared_ptr<PacketBuffer> buffer_;
            InetAddress peer_;
            Timestamp receive_time_;
            std::size_t original_size_{0};
            bool truncated_{false};
    };

    using UdpMessageCallback = std::function<void(UdpDatagram)>;
}
