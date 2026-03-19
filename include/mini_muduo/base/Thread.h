#pragma once

#include "mini_muduo/base/noncopyable.h"

#include <atomic>
#include <functional>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <string>

namespace mini_muduo 
{
    using TaskCallback = std::function<void()>;
    class Thread : public Noncopyable
    {
        public:
            // name的长度限制在16个字节
            explicit Thread(TaskCallback task, const std::string& name = std::string());
            ~Thread();

            void Start();
            int Join();

            void SetTaskNum(int task_num) { task_num_ = task_num; }

            bool started() const { return started_; }
            pid_t tid() const { return t_id_; }
            const std::string& name() const { return name_; }
            int task_num() const { return task_num_; }

        private:
            void SetDefaultName();

            bool started_;
            bool joined_;
            pthread_t pthread_;
            pid_t t_id_;
            std::string name_;
            TaskCallback task_;
            std::atomic<int> task_num_;
            sem_t sem_;

            static std::atomic<int> thread_create_num_;
    };
}