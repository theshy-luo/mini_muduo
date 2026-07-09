#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace mini_muduo 
{
    class PacketBuffer
    {
        public:
            static const std::size_t kDefaultCapacity  = 1500; 
            explicit PacketBuffer(std::size_t capacity = kDefaultCapacity)
            : capacity_(capacity)
            {
                data_.reset(new std::uint8_t[capacity_]);
            }
            ~PacketBuffer() = default;

            std::uint8_t* writable_data() noexcept
            {
                return data_.get();
            }
            const std::uint8_t* data() const noexcept 
            { 
                return data_.get();
            }

            std::size_t size() const noexcept
            {
                return size_;
            }
            std::size_t capacity() const noexcept
            {
                return capacity_;
            }
            
            void set_size(std::size_t size) noexcept
            {
                assert(size <= capacity_);
                size_ = size;
            }
            void reset() noexcept
            {
                size_ = 0;
            }            

        private:
            std::unique_ptr<std::uint8_t[]> data_;
            std::size_t capacity_;
            std::size_t size_{0};
    };
}