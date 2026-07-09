#pragma once

#include "mini_muduo/base/noncopyable.h"

#include <unistd.h>

namespace mini_muduo 
{
    class UniqueFd : public Noncopyable
    {
        public:
            UniqueFd() noexcept = default;
            explicit UniqueFd(int fd) noexcept
            : fd_(fd >= 0 ? fd : -1)
            {}
            ~UniqueFd() noexcept
            {
                reset();
            }

            UniqueFd(UniqueFd&& other) noexcept
                : fd_(other.release())
            {}

            UniqueFd& operator=(UniqueFd&& other) noexcept
            {
                if (this != &other) 
                {
                    reset(other.release());
                }

                return *this;
            }

            int get() const noexcept
            {
                return fd_;
            }

            bool valid() const noexcept
            {
                return fd_ >= 0;  
            }

            int release() noexcept
            {
                int old_fd  = fd_;
                fd_ = -1;
                return old_fd ;
            }

            void reset(int new_fd = -1) noexcept
            {
                if (fd_ == new_fd) 
                {
                    return;
                }
                if (valid()) 
                {
                    ::close(fd_);
                }
                fd_ = new_fd >= 0 ? new_fd : -1;
            }

        private:
            int fd_{-1};
    };

}