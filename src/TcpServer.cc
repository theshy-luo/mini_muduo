#include "mini_muduo/TcpServer.h"
#include "mini_muduo/Callbacks.h"
#include "mini_muduo/EventLoop.h"
#include "mini_muduo/TcpConnection.h"

#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <string>

namespace mini_muduo
{
    TcpServer::TcpServer(EventLoop* loop, const InetAddress& listen_addr, 
        const std::string& name)
        : loop_(loop),
        name_(name),
        acceptor_(new Acceptor(loop, listen_addr, true)),
        started_(false),
        next_conn_id_(0),
        thread_pool_(new EventLoopThreadPool(loop, 0, name))
    {
        acceptor_->SetNewConnectionCallback(std::bind(&TcpServer::NewConnectionCallback, this, 
            std::placeholders::_1, std::placeholders::_2));
    }

    TcpServer::~TcpServer()
    {
        std::cout << "TcpServer::~TcpServer [" << name_ << "] destructing\n";

        for (auto& pair : connections_)
        {
            TcpConnectionPtr conn(pair.second);
            pair.second.reset();
            conn->ConnectDestroyed();
        }
    }

    void TcpServer::NewConnectionCallback(int sockfd, const InetAddress& peer_addr)
    {
        InetAddress localaddr;
        socklen_t addrlen = static_cast<socklen_t>(sizeof(*localaddr.GetSockAddr()));
        if (::getsockname(sockfd, (struct sockaddr *)(localaddr.GetSockAddr()), &addrlen) < 0)
        {
            std::cerr << "sockets::getLocalAddr" << "\n";
        }

        // 从线程池中选择一个IO线程
        EventLoop* io_loop = thread_pool_->GetNextLoop();

        std::string conn_name = name_ + "#" + std::to_string(next_conn_id_++);
        connections_.emplace(conn_name, 
            TcpConnectionPtr(new TcpConnection(io_loop, conn_name, sockfd, localaddr, peer_addr)));

        TcpConnectionPtr conn = connections_[conn_name];
        conn->SetConnectionCallback(connection_callback_);
        conn->SetMessageCallback(message_callback_);
        conn->SetCloseCallback(std::bind(&TcpServer::RemoveConnection, 
            this, std::placeholders::_1));
        io_loop->QueueInLoop(std::bind(&TcpConnection::ConnectEstablished, conn));
    }

    void TcpServer::RemoveConnection(const TcpConnectionPtr& conn)
    {
        std::cout << "TcpServer::RemoveConnection [" << name_ << "] - connection " << conn->name() << "\n";

        loop_->QueueInLoop(std::bind(&TcpServer::RemoveConnectionInLoop, this, conn));
    }

    void TcpServer::RemoveConnectionInLoop(const TcpConnectionPtr& conn)
    {
        loop_->AssertInLoopThread();
        std::cout << "TcpServer::removeConnectionInLoop [" << name_
           << "] - connection " << conn->name() << " tid:" << CurrentThread::tid() << std::endl;

        size_t n = connections_.erase(conn->name());
        assert(n == 1);
        (void)n;
        EventLoop* io_loop = conn->GetLoop();
        io_loop->RunInLoop(std::bind(&TcpConnection::ConnectDestroyed, conn));
    }
    
    void TcpServer::Start()
    {
        if (!started_) 
        {
            // 开启线程池
            thread_pool_->Start(thread_init_callback_); 
            // 然后再去让 acceptor 监听
            started_ = true;
            acceptor_->listen();
        }
    }
}