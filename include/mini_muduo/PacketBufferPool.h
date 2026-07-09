#pragma once

#include "mini_muduo/PacketBuffer.h"
#include <cstddef>
#include <memory>
#include <mutex>
#include <vector>

namespace mini_muduo 
{
    class PacketBufferPool
    {
        public:
            static const std::size_t kDefaultInitialBufferCount  = 1024; 
            struct State
            {
                explicit State(std::size_t buffer_size,  
                    std::size_t initial_buffer_count = kDefaultInitialBufferCount)
                : buffer_size(buffer_size)
                {
                    free_buffers.reserve(initial_buffer_count);
                    for(size_t i = 0; i < initial_buffer_count; i++)
                    {
                        free_buffers.push_back(
                            std::unique_ptr<PacketBuffer>(
                                new PacketBuffer(buffer_size)));
                    }
                }

                std::size_t buffer_size;
                std::mutex mutex;
                std::vector<std::unique_ptr<PacketBuffer>> free_buffers;
            };

            explicit PacketBufferPool(std::size_t buffer_size, 
                std::size_t initial_buffer_count = kDefaultInitialBufferCount)
            : state_(new State(buffer_size, initial_buffer_count))
            {}
            ~PacketBufferPool() = default;

            std::shared_ptr<PacketBuffer> Acquire();

        private:
            std::shared_ptr<State> state_; 
    };
}