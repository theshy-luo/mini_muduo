#pragma once

#include "mini_muduo/Acceptor.h"
#include "mini_muduo/EventLoopThreadPool.h"
#include "mini_muduo/InetAddress.h"

#include <atomic>
#include <map>
#include <string>

namespace mini_muduo 
{
    class EventLoop;
    
    class TcpServer : public Noncopyable
    {
        public:
            TcpServer(EventLoop* loop, const InetAddress& listen_addr, 
                const std::string& name);
            ~TcpServer();

            // 启动服务器
            void Start();

            void SetThreadNum(int num_threads)
            {
                assert(!started_);
                thread_pool_->SetThreadNum(num_threads);
            }

            // 设置用户回调
            void SetConnectionCallback(const ConnectionCallback& cb)
            {
                connection_callback_ = cb;
            }
            void SetMessageCallback(const MessageCallback& cb)
            {
                message_callback_ = cb;
            }
            void SetThreadInitCallback(const ThreadInitCallback& cb)
            {
                thread_init_callback_ = cb;
            }
            
        private:
            // 当 Acceptor 发现新连接时，会呼叫这个内部函数
            void NewConnectionCallback(int sockfd, 
                const InetAddress& peer_addr);
            // 当 TcpConnection 要断开时，会呼叫这个内部函数
            void RemoveConnection(const TcpConnectionPtr& conn);

            void RemoveConnectionInLoop(const TcpConnectionPtr& conn);

            EventLoop* loop_;
            const std::string name_;

            std::unique_ptr<Acceptor> acceptor_;
            std::atomic_bool started_; // 判断服务器是否启动
            std::atomic_int next_conn_id_; // 用来给连接生成唯一名字的计数器

            // 核心账本：保存所有活跃的连接
            std::map<std::string, TcpConnectionPtr> connections_;

            ConnectionCallback connection_callback_;
            MessageCallback message_callback_;
            ThreadInitCallback thread_init_callback_;

            std::shared_ptr<EventLoopThreadPool> thread_pool_;
    };
}