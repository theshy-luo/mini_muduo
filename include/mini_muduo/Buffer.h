#pragma once

#include <cassert>
#include <cstddef>
#include <vector>
#include <string>
#include <algorithm>

namespace mini_muduo 
{
    class Buffer 
    {
        public:
            static const size_t kInitialSize = 1024;
            explicit Buffer(size_t initial_size = kInitialSize):
                buffer_(initial_size),
                read_index_(0),
                write_index_(0)
            {
                assert(ReadableBytes() == 0);
                assert(WriteableBytes() == initial_size);
                // assert(PrependableBytes() == 0);
            } 

            // 还有多少可读字节？
            size_t ReadableBytes() const
            {
                return write_index_ - read_index_;
            }

            // 还有多少可写空间？
            size_t WriteableBytes() const
            {
                return buffer_.size() - write_index_;
            }

            size_t FreeSize()
            {
                return buffer_.size() - write_index_ + read_index_;
            }

            // 找回 readerIndex 指针
            const char* Peek() const
            {
                return begin() + read_index_;
            }

            // 将 buf 转成 string 提取出来
            std::string RetrieveAllAsString()
            {
                return RetrieveAsString(ReadableBytes());
            }

            std::string RetrieveAsString(size_t len)
            {
                assert(len <= ReadableBytes());
                std::string result(Peek(), len);
                retrieve(len);
                return result;
            }

            void retrieve(size_t len)
            {
                assert(len <= ReadableBytes());
                if (len < ReadableBytes()) 
                {
                    read_index_ += len;
                }
                else 
                {
                    read_index_ = 0;
                    write_index_ = read_index_;
                }
            }
            
            // 牺牲一定的空间 换取高效的性能
            void MakeSpace(size_t len)
            {
                if (len > FreeSize()) 
                {
                    buffer_.resize(write_index_ + len);
                }
                else 
                {
                    std::copy(begin() + read_index_, 
                        begin() + write_index_, begin());
                    write_index_ = write_index_ - read_index_;
                    read_index_ = 0;
                }
            }

            // 核心方法：向Buffer中追加数据
            void Append(const char* data, size_t len);

            // 核心方法：从 fd 中直接读取数据到 Buffer 中（这需要用到 readv 系统调用，是个进阶点！）
            ssize_t ReadFd(int fd, int* saveErrno);

        private:
            char* begin() { return buffer_.data(); }
            const char* begin() const { return buffer_.data(); }

            std::vector<char> buffer_;
            size_t read_index_;
            size_t write_index_;
    };
}