#include "mini_muduo/EventLoopThread.h" 
#include "mini_muduo/EventLoop.h"

namespace mini_muduo 
{
    EventLoopThread::EventLoopThread(ThreadInitCallback cb, const std::string& base_name):
        loop_(nullptr),
        exiting_(false),
        init_callback_(std::move(cb)),
        thread_(std::bind(&EventLoopThread::ThreadFunc, this), base_name)
    {}
    EventLoopThread::~EventLoopThread()
    {
        exiting_ = true;
        EventLoop* loop = nullptr;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            loop = loop_;
        }

        if (loop != nullptr)
        {
            loop->Quit();
        }

        if (thread_.started()) 
        {
            thread_.Join();
        }
    }    

    EventLoop* EventLoopThread::StartLoop()
    {
        thread_.Start();
        EventLoop* loop = nullptr;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [this](){ return loop_ != nullptr;});
            loop = loop_;
        }
        return loop;
    }

    void EventLoopThread::ThreadFunc()
    {
        EventLoop loop;
        if (init_callback_)
        {
            init_callback_(&loop);
        }

        { 
            // 保护 loop_ 的修改
            std::unique_lock<std::mutex> lock(mutex_);
            loop_ = &loop;
            cond_.notify_one(); // 唤醒 StartLoop
        }

        loop.Loop();

        // 退出循环后，清理 loop_ 指针
        { 
            std::unique_lock<std::mutex> lock(mutex_);
            loop_ = nullptr;
        }
    }
} // namespace mini_muduo