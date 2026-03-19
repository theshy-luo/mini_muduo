#include "mini_muduo/EventLoopThreadPool.h"
#include "mini_muduo/EventLoopThread.h"
#include <cassert>

namespace mini_muduo 
{
    EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop, int num_threads, 
        const std::string& base_name) :
        base_loop_(base_loop),
        num_threads_(num_threads),
        started_(false),
        base_name_(base_name),
        next_(0)
    {}

    EventLoopThreadPool::~EventLoopThreadPool()
    {}

    void EventLoopThreadPool::Start(ThreadInitCallback cb)
    {
        assert(!started_);
        for (int i = 0; i < num_threads_; i++) 
        {
            std::string thread_name = base_name_.empty() ? 
                std::string() : base_name_ + std::to_string(i);
            std::unique_ptr<EventLoopThread> thread(new EventLoopThread(cb, thread_name));
            loops_.push_back(thread->StartLoop());
            threads_.push_back(std::move(thread));
        }
        started_ = true;

        if (num_threads_ == 0 && cb) 
        {
            cb(base_loop_);
        }
    }
    
    EventLoop* EventLoopThreadPool::GetNextLoop()
    {
        assert(started_);
        EventLoop* loop = base_loop_;
        if (!loops_.empty()) 
        {
            loop = loops_[next_++];
            if (static_cast<size_t>(next_) >= loops_.size()) 
            {
                next_ = 0;
            }
        }
        return loop;
    }
} // namespace mini_muduo