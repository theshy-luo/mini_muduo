#include "mini_muduo/base/Thread.h"
#include "mini_muduo/base/CurrentThread.h"
#include "mini_muduo/base/Exception.h"
#include <cassert>
#include <semaphore.h>
#include <sys/prctl.h>

namespace mini_muduo 
{
namespace detail 
{
    struct ThreadData
    {
        TaskCallback task_;
        std::string name_;
        pid_t* t_id_;
        const sem_t* sem_;

        ThreadData(TaskCallback t, std::string n, pid_t* id, const sem_t* sem)
            : task_(std::move(t)), name_(std::move(n)), t_id_(id), sem_(sem)
        {
        }

        ~ThreadData()
        {
        }
        
        void RunInThread()
        {
            // 拿到子线程真实 tid，写入回父线程传来的 t_id 指针里
            *t_id_ = CurrentThread::tid();
            //设置线程名字
            CurrentThread::t_threadName = name_.empty() ? "minimuduo_Thread" : name_.c_str();
            ::prctl(PR_SET_NAME, CurrentThread::t_threadName);
            // 通知主线程："我已经拿到 tid 了，你可以继续了"
            sem_post(const_cast<sem_t*>(sem_));
            // 执行真正的任务
            try
            {
                task_();
            }
            catch(const Exception& ex)
            {
                fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                fprintf(stderr, "reason: %s\n", ex.what());
                fprintf(stderr, "stack trace: %s\n", ex.stack());
                abort();
            }
            catch (const std::exception& e)
            {
                fprintf(stderr, "exception caught in Thread %s\n", name_.c_str());
                fprintf(stderr, "reason: %s\n", e.what());
                abort();
            }
            catch (...)
            {
                fprintf(stderr, "unknown exception caught in Thread %s\n", name_.c_str());
                abort();
            }
        }
    };

    void* StartThread(void* obj)
    {
        ThreadData* data = static_cast<ThreadData*>(obj);
        data->RunInThread();
        delete data;
        return nullptr;
    }
}
    std::atomic<int> Thread::thread_create_num_(0);
    
    Thread::Thread(TaskCallback task, const std::string& name)
        : started_(false),
        joined_(false),
        pthread_(0),
        t_id_(0),
        name_(name),
        task_(std::move(task)),
        task_num_(0)
    {
        sem_init(&sem_, 0, 0);
        SetDefaultName();
    }

    Thread::~Thread()
    {
        sem_destroy(&sem_);
        if (started_ && !joined_)
        {
            pthread_detach(pthread_);
        }
    }

    void Thread::Start()
    {
        assert(!started_);
        started_ = true;
        detail::ThreadData* data = new detail::ThreadData(task_, name_, &t_id_, &sem_);
        if (pthread_create(&pthread_, nullptr, &detail::StartThread, data) != 0)
        {
            started_ = false;
            delete data;
            abort();
        }
        // 等待子线程拿到 tid 后才返回
        sem_wait(&sem_);
    }

    int Thread::Join()
    {
        assert(started_);
        assert(!joined_);
        joined_ = true;
        return pthread_join(pthread_, nullptr);
    }

    void Thread::SetDefaultName()
    {
        int num = thread_create_num_++;
        if (name_.empty()) 
        {
            name_ = "Thread-" + std::to_string(num);
        }
    }
}