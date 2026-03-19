#include "mini_muduo/Buffer.h"

#include <cstddef>
#include <errno.h>
#include <iostream>
#include <sys/uio.h>

namespace mini_muduo 
{
    void Buffer::Append(const char* data, size_t len)
    {
        if (WriteableBytes() < len) 
        {
            MakeSpace(len);
        }
        std::copy(data, data + len, begin() + write_index_);
        write_index_ += len;
    }

    ssize_t Buffer::ReadFd(int fd, int* saveErrno)
    {
        char extra_buf[64*1024];

        struct iovec vec[2];
        const size_t writable = WriteableBytes();
        vec[0].iov_base = begin() + write_index_;
        vec[0].iov_len = writable;
        vec[1].iov_base = extra_buf;
        vec[1].iov_len = sizeof(extra_buf);

        ssize_t n = readv(fd, vec, 2);
        if (static_cast<size_t>(n) <= writable) 
        {
            write_index_ += n;
        }
        else 
        {
            if (n < 0) 
            {
                std::cerr << "Buffer::ReadFd error" << "\n";
                *saveErrno = errno;
            }
            else 
            {
                Append(extra_buf, n - writable);
            }
        }
        return n;
    }
}