#pragma once

#include "mini_muduo/base/noncopyable.h"

#include <cassert>
#include <string>
#include <cstddef>
#include <cstring>

namespace mini_muduo 
{
    namespace detail 
    {
        const int kSmallBuffer = 4000;
        const int kLargeBuffer = 4000 * 1000;

        template <int SIZE>
        class FixedBuffer : private  Noncopyable
        {
            public:
                FixedBuffer() : cur_(data_)
                {};
                ~FixedBuffer() = default;

                void append(const char* buf, size_t len)
                {
                    assert(static_cast<size_t>(avail()) >= len);
                    if (static_cast<size_t>(avail()) >= len) 
                    {
                        ::memcpy(cur_, buf, len);
                        cur_ += len;
                    }
                }

                const char* data() const { return data_; }
                int length() const { return static_cast<int>(cur_ - data_);}
                char* current() { return cur_;}
                int avail() const { return static_cast<int>(end() - cur_);}
                
                void add(size_t len) { cur_ += len; }
                void reset() { cur_ = data_; }
                void bzero() { ::memset(data_, 0, sizeof(data_)); }

            private:
                const char* end() const
                {
                    return data_ + sizeof(data_);
                }

                char data_[SIZE];
                char* cur_;
        };
    } // namespace detail

    class LogStream : public Noncopyable
    {
        public:
            using Buffer = detail::FixedBuffer<detail::kSmallBuffer>;

            LogStream() = default;
            ~LogStream() = default;

            LogStream& operator<<(char c);
            LogStream& operator<<(const char *const str);

            LogStream& operator<<(short c);
            LogStream& operator<<(unsigned short c);
            LogStream& operator<<(int c);
            LogStream& operator<<(unsigned int c);
            LogStream& operator<<(long c);
            LogStream& operator<<(long long c);
            LogStream& operator<<(unsigned long c);
            LogStream& operator<<(unsigned long long c);

            LogStream& operator<<(float c);
            LogStream& operator<<(double c);
            
            LogStream& operator<<(const std::string& str);
            LogStream& operator<<(const Buffer& buf);
            LogStream& operator<<(const void*);

            void Append(const char* buf, size_t len)
            {
                buffer_.append(buf, len);
            }
            const Buffer& buffer() const { return buffer_; }
            void ResetBuffer() { buffer_.reset(); }

        private:
            template <typename T>
            void FormatInteger(T value);

            Buffer buffer_;

            static const int kMaxNumericSize = 48;
    };
} // namespace mini_muduo
