#include "mini_muduo/PacketBufferPool.h"
#include "mini_muduo/PacketBuffer.h"
#include <memory>
#include <mutex>
#include <utility>

namespace mini_muduo 
{
    std::shared_ptr<PacketBuffer> PacketBufferPool::Acquire()
    {
        std::unique_ptr<PacketBuffer> buffer;

        {
            std::lock_guard<std::mutex> lock(state_->mutex);

            if (!state_->free_buffers.empty()) 
            {
                buffer = std::move(state_->free_buffers.back());
                state_->free_buffers.pop_back();
            }
        }

        if (!buffer) 
        {
            buffer.reset(new PacketBuffer(state_->buffer_size));
        }

        buffer->reset();

        PacketBuffer* raw = buffer.release();
        std::shared_ptr<State> state = state_;

        return std::shared_ptr<PacketBuffer>(raw, [state](PacketBuffer* buffer)
        {
            buffer->reset();

            std::lock_guard<std::mutex> lock(state->mutex);
            state->free_buffers.emplace_back(buffer);
        });
    }
}