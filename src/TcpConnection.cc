#include "mini_muduo/TcpConnection.h"
#include "mini_muduo/Callbacks.h"
#include "mini_muduo/Channel.h"
#include "mini_muduo/base/Timestamp.h"
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace mini_muduo 
{
    TcpConnection::TcpConnection(EventLoop *loop, const std::string &name, int sockfd,
                const InetAddress &localAddr, const InetAddress &peerAddr):
        loop_(loop),
        name_(name),
        state_(kConnecting),
        socket_(new Socket(sockfd)),
        channel_(new Channel(loop, sockfd)),
        local_addr_(localAddr),
        peer_addr_(peerAddr)
    {
        // 核心绑定：当 Channel 被 epoll 唤醒时，会通过这些钩子回到 TcpConnection 内部处理
        channel_->SetReadCallback(std::bind(&TcpConnection::HandleRead, this, std::placeholders::_1));
        channel_->SetWriteCallback(std::bind(&TcpConnection::HandleWrite, this));
        channel_->SetCloseCallback(std::bind(&TcpConnection::HandleClose, this));
        channel_->SetErrorCallback(std::bind(&TcpConnection::HandleError, this));

        std::cout << "TcpConnection::TcpConnection[" << name_ << "] at" << this 
            << " fd=" << sockfd << "\n";
    }

    TcpConnection::~TcpConnection()
    {
        std::cout << "TcpConnection::~TcpConnection[" << name_ 
            << "] state=" << state_ << "\n";
    }

    void TcpConnection::HandleRead(Timestamp receive_time)
    {
        int save_errno = 0;
        // 1. 利用 Buffer 自动扩容的 ReadFd 把数据从内核全吸进来
        ssize_t n = input_buffer_.ReadFd(channel_->fd(), &save_errno);

        if (n > 0) 
        {
            // 成功读到数据，通知用户处理业务逻辑
            // 注意：这里用 shared_from_this() 保证回调执行期间对象不死
            message_callback_(shared_from_this(), &input_buffer_, receive_time);
        }
        else if(n == 0) 
        {
            // 对方关闭了连接
            HandleClose();
        }
        else 
        {
            // 发生了错误
            errno = save_errno;
            HandleError();
        }
    }

    void TcpConnection::HandleWrite()
    {
        if (channel_->is_writing()) 
        {
            ssize_t n = ::write(channel_->fd(), 
                output_buffer_.Peek(), output_buffer_.ReadableBytes());
            if (n > 0) 
            {
                output_buffer_.retrieve(n);
                if (output_buffer_.ReadableBytes() == 0) 
                {
                    // 1. 发完了！关掉写监听，防止 epoll 疯狂轰炸
                    channel_->DisableWriting();
                    
                    // 2. 如果设置了“写完回调”，通知用户
                    if (write_complete_callback_) 
                    {
                        write_complete_callback_(shared_from_this());
                    }

                    // 3. 如果用户之前想关连接，现在发完了，可以送客了
                    if (state_ == kDisconnecting) 
                    {
                        socket_->ShutdownWrite();
                    }
                }
            }
            else 
            {
                std::cerr << "TcpConnection::HandleWrite error" << std::endl;
            }
        }  
    }

    void TcpConnection::HandleClose()
    {
        assert(state_ == kConnected || state_ == kDisconnecting);
        SetState(kDisconnected);
        channel_->DisableAll();// 拔掉所有监听

        TcpConnectionPtr guard_this(shared_from_this());
        if (connection_callback_) 
        {
            connection_callback_(guard_this); // 通知用户：连接已断开
        }
        if (close_callback_) 
        {
            close_callback_(guard_this); // 内部清理，通常会通知 TcpServer 把自己从 Map 里删掉
        }
    }

    void TcpConnection::HandleError()
    {
        int err = 0;
        socklen_t len = sizeof(err);
        if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &err, &len) < 0) 
        {
            err = errno;
        }
        std::cerr << "TcpConnection::HandleError [" << name_ << "] SO_ERROR=" 
            << err << std::endl;
    }

    void TcpConnection::ConnectEstablished()
    {
        SetState(kConnected);
        channel_->EnableReading(); // 核心：正式开始监听读事件
        channel_->Tie(shared_from_this());

        if (connection_callback_) 
        {
            connection_callback_(shared_from_this());
        }
    }

    void TcpConnection::ConnectDestroyed()
    {
        if (state_ == kConnected || state_ == kDisconnecting) 
        {
            SetState(kDisconnected);
            channel_->DisableAll();
            
            connection_callback_(shared_from_this());
        }

        channel_->DisableAll();
        channel_->Remove(); // 核心：从 Poller 中移除
    }

    void TcpConnection::send(const void* message, size_t len)
    {
        if (state_ == kConnected) 
        {
            ssize_t nwrote = 0;
            size_t remaining = len;
            bool fault_error = false;
            
            // 1. 如果 output_buffer_ 为空，直接尝试发送
            if (!channel_->is_writing() && 
                output_buffer_.ReadableBytes() == 0) 
            {
                nwrote = ::write(channel_->fd(), message, len);
                if (nwrote >= 0) 
                {
                    remaining = len - nwrote;

                    if (remaining == 0 && write_complete_callback_) 
                    {
                        // 发完了！可以在这里通知用户
                        write_complete_callback_(shared_from_this());
                    }
                }
                else 
                {
                    nwrote = 0;
                    if (errno != EWOULDBLOCK) 
                    {
                         std::cerr << "TcpConnection::send write error\n";
                    }

                    if (errno == EPIPE) 
                    {
                        fault_error = true;
                    }
                }
            }

            // 2. 没发完，或者刚才报错了但不是致命伤，就把剩下的存入缓冲区
            if (!fault_error && remaining > 0) 
            {
                output_buffer_.Append(static_cast<const char*>(message) + nwrote, remaining);
                if (!channel_->is_writing()) 
                {
                    channel_->EnableWriting(); // 开启写监听，由 HandleWrite 接力
                }
            }
        }
    }

    void TcpConnection::send(const std::string& buf)
    {
        send(buf.data(), buf.size());
    }

    void TcpConnection::send(Buffer* buf)
    {
        std::string data = buf->RetrieveAllAsString();
        send(data.c_str(), data.size());
    }

    void TcpConnection::shutdown()
    {
        if (state_ == kConnected) 
        {
            SetState(kDisconnecting);

            // 如果现在没在发数据，直接调底层 shutdown
            if (!channel_->is_writing()) 
            {
                socket_->ShutdownWrite();
            }
            // 如果正在发数据，不用管，HandleWrite 最后会帮我们收尾
        }
    }
    
} // namespace mini_muduo