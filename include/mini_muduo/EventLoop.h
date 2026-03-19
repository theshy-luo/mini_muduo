#pragma once

#include "mini_muduo/Channel.h"
#include "mini_muduo/TimerId.h"
#include "mini_muduo/TimerQueue.h"
#include "mini_muduo/base/CurrentThread.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <sys/types.h>
#include <vector>

namespace mini_muduo 
{
  class Poller;
  class TimerQueue;
  class EventLoop : public Noncopyable 
  {
    public:
      using TaskFunctor = std::function<void()>;

      EventLoop();
      ~EventLoop();

      // 定时接口
      TimerId RunAt(Timestamp time, TimerCallback cb);
      TimerId RunAfter(double delay, TimerCallback cb);
      TimerId RunEvery(double interval, TimerCallback cb);
      void CancelTimer(TimerId timer_id);

      void Loop();
      void Quit();
      void UpdateChannel(Channel *channel);
      void RemoveChannel(Channel *channel);

      bool IsInLoopThread() const { return thread_id_ == CurrentThread::tid(); }

      void AssertInLoopThread();

      /// 在循环线程中立即运行回调函数。
      /// 它会唤醒循环，并运行回调函数。
      /// 如果在同一个循环线程中，回调函数会在函数内部运行。
      /// 可以安全地从其他线程调用。
      void RunInLoop(TaskFunctor task);

      /// 将回调函数放入循环线程的队列中。
      /// 在线程池结束后运行。
      /// 可以从其他线程安全地调用。
      void QueueInLoop(TaskFunctor task);

    private:
      void WakeUp();

      void WakeUpHandleRead();

      void DoPendingFunctors();

      std::mutex mutex_;

      // 标志是否正在循环中
      bool looping_;
      // 标志是否要退出循环
      std::atomic_bool quit_;
      // 记录当前 EventLoop 所属的线程 ID。
      const pid_t thread_id_;
      // 标记queue是否已经在执行
      bool calling_pending_functors_;
      // 唤醒文件描述符
      int wakeup_fd_;
      // 唤醒通道
      std::unique_ptr<Channel> wakeup_channel_;
      // 得力助手，负责底层的 epoll
      std::unique_ptr<Poller> poller_;
      // 定时器
      std::unique_ptr<TimerQueue> timer_queue_;
      // epoll 返回的活跃通道列表
      std::vector<Channel *> activeChannels_;
      // 待执行的任务队列
      std::vector<TaskFunctor> pending_functors_;
  };

} // namespace mini_muduo