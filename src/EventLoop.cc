#include "mini_muduo/EventLoop.h"
#include "mini_muduo/Channel.h"
#include "mini_muduo/EpollPoller.h"
#include "mini_muduo/Poller.h"
#include "mini_muduo/TimerId.h"
#include "mini_muduo/base/Timestamp.h"
#include "mini_muduo/TimingWheel.h"

#include <iostream> // 目前没有日志库，遇到严重错误先报错打印一下
#include <mutex>
#include <sys/eventfd.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace  
{
  int CreateEventfd()
    {
      int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
      if (evtfd < 0)
      {
        std::cerr << "Failed in eventfd";
        abort();
      }
      return evtfd;
    }
}

namespace mini_muduo 
{
  // 这是一个线程局部变量。
  // 作用：限制一个线程只能有一个 EventLoop 对象（也就是 One Loop Per Thread
  // 核心思想）
  __thread EventLoop *t_loopInThisThread = nullptr;

  EventLoop::EventLoop()
      : looping_(false)
      , quit_(false)
      , thread_id_(CurrentThread::tid())
      , calling_pending_functors_(false)
      , wakeup_fd_(CreateEventfd())
      , wakeup_channel_(new Channel(this, wakeup_fd_))
      , poller_(new EpollPoller(this)) 
      , timer_queue_(new TimerQueue(this))
      , timing_wheel_(nullptr)
  {
    // 如果发现这个线程里已经有人创建过 EventLoop 了，这是致命的，立刻报错
    if (t_loopInThisThread) 
    {
      std::cerr << "Fatal Error: Another EventLoop " << t_loopInThisThread
                << " exists in this thread " << thread_id_ << "\n";
    } 
    else 
    {
      t_loopInThisThread = this;
    }

    wakeup_channel_->SetReadCallback(std::bind(&EventLoop::WakeUpHandleRead, this));
    wakeup_channel_->EnableReading();

  }

  EventLoop::~EventLoop() { t_loopInThisThread = nullptr; }

  TimerId EventLoop::RunAt(Timestamp time, TimerCallback cb)
  {
    return timer_queue_->AddTimer(time, std::move(cb));
  }

  TimerId EventLoop::RunAfter(double delay, TimerCallback cb)
  {
    Timestamp time = AddTime(Timestamp::Now(), delay);
    return RunAt(time, std::move(cb));
  }

  TimerId EventLoop::RunEvery(double interval, TimerCallback cb)
  {
    Timestamp time = AddTime(Timestamp::Now(), interval);
    return timer_queue_->AddTimer(time, std::move(cb), interval);
  }

  void EventLoop::CancelTimer(TimerId timer_id)
  {
    timer_queue_->CancelTimer(timer_id);
  }

  void EventLoop::EnableIdleTimeout(int seconds)
  {
    timing_wheel_.reset(new TimingWheel(this, seconds));
  }

  void EventLoop::Feed(const TcpConnectionPtr& conn)
  {
    if (timing_wheel_) 
    {
      timing_wheel_->Feed(conn);
    }
  }

  void EventLoop::Loop() 
  {
    looping_ = true;
    quit_ = false;

    // 不断地循环，直到被人调用 Quit() 设为 true
    while (!quit_) 
    {
      activeChannels_.clear();

      // 【核心大招】：调用底层的多路复用器，让它去内核帮我问一下，哪些 Socket
      // 来事了！
      poller_->Poll(10000 /*超时时间*/, &activeChannels_);

      // 拿到一堆有事件发生的 Channel 之后，开始唤醒它们
      for (Channel *channel : activeChannels_) 
      {
        // 对于每一个活跃事件，调用 channel 的处理回调。
        // 也就是咱们之前辛辛苦苦在 Channel 里写的那个根据 revents_ 瞎忙活的函数！
        channel->HandleEvent(Timestamp::Now());
      }

      DoPendingFunctors();
    }

    looping_ = false;
  }

  void EventLoop::Quit() 
  {
    // 将开关拨到退出状态，下一次 while 循环判断时就会退出 loop
    quit_ = true;
    if (!IsInLoopThread()) 
    {
      WakeUp();
    }
  }

  void EventLoop::UpdateChannel(Channel *channel) 
  {
    if (!IsInLoopThread()) 
    {
      std::cerr << "Fatal: UpdateChannel must be called in loop thread!\n";
      return;
    }
    poller_->UpdateChannel(channel);
  }

  void EventLoop::RemoveChannel(Channel *channel) 
  {
    poller_->RemoveChannel(channel);
  }

  void EventLoop::AssertInLoopThread()
  {
    if (!IsInLoopThread()) 
    {
      std::cerr << "EventLoop::abortNotInLoopThread - EventLoop " << this
        << " was created in threadId_ = " << thread_id_
        << ", current thread id = " <<  CurrentThread::tid();
      abort();
    }
  }

  void EventLoop::RunInLoop(TaskFunctor task)
  {
    if (IsInLoopThread()) 
    {
      task();
    }
    else 
    {
      QueueInLoop(std::move(task));
    }
  }

  void EventLoop::QueueInLoop(TaskFunctor task)
  {
    { 
      std::unique_lock<std::mutex> lock(mutex_);
      pending_functors_.push_back(std::move(task));
    }

    if (!IsInLoopThread() || calling_pending_functors_) 
    {
      WakeUp();
    }
  }

  void EventLoop::WakeUp()
  {
    uint64_t one = 1;
    ssize_t n = ::write(wakeup_fd_, &one, sizeof(one));
    if (n < 0) 
    {
      std::cerr << "EventLoop::WakeUp() writes " << n << " bytes instead of 8";
    }
  }

  void EventLoop::WakeUpHandleRead()
  {
    uint64_t one = 1;
    ssize_t n =  ::read(wakeup_fd_, &one, sizeof(one));
    if (n < 0) 
    {
      std::cerr << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
  }

  void EventLoop::DoPendingFunctors()
  {
    std::vector<TaskFunctor> funtors;
    calling_pending_functors_ = true;

    {
      std::unique_lock<std::mutex> lock(mutex_);
      funtors.swap(pending_functors_);
    }

    for (auto function : funtors) 
    {
      function();
    }

    calling_pending_functors_ = false;
  }

} // namespace mini_muduo