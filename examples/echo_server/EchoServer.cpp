#include "mini_muduo/Callbacks.h"
#include "mini_muduo/EventLoop.h"
#include "mini_muduo/InetAddress.h"
#include "mini_muduo/TcpServer.h"

#include <iostream>
#include <string>

using namespace mini_muduo;

static void OnConnectionCallback(const mini_muduo::TcpConnectionPtr& conn)
{
    if (conn->Connected()) 
    {
        std::cout << "Connected: " << conn->name() << "\n";
    }
    else 
    {
        std::cout << "Disconnected: " << conn->name() << "\n";
    }
}

static void OnMessage(const mini_muduo::TcpConnectionPtr& conn, 
    mini_muduo::Buffer* buf, mini_muduo::Timestamp time)
{
    std::cout << "OnMessage Echo time:" << time.milli_seconds_since_epoch() << " tid:" 
        << CurrentThread::tid() << std::endl;
    std::string msg = buf->RetrieveAllAsString();
    conn->send(msg); // 核心：Echo 回去
}

int main()
{
    mini_muduo::EventLoop loop;
    mini_muduo::InetAddress addr(20000);
    mini_muduo::TcpServer Server(&loop, addr, "lqh EchoServer");
    
    Server.SetConnectionCallback(OnConnectionCallback);
    Server.SetMessageCallback(OnMessage);

    Server.SetThreadNum(4);

    Server.Start();
    loop.Loop();

    return 0;
}