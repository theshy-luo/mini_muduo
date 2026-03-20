#pragma once

#include "mini_muduo/Callbacks.h"
#include "mini_muduo/EventLoop.h"
#include "mini_muduo/TcpConnection.h"
#include "mini_muduo/base/noncopyable.h"

#include <cstddef>
#include <unordered_set>
#include <memory>
#include <vector>

namespace mini_muduo 
{
    class TimingWheel : public Noncopyable
    {
        public:
            TimingWheel(EventLoop* loop, int timeout_seconds = 10);
            ~TimingWheel();

            // 添加 or 刷新连接
            void Feed(const TcpConnectionPtr& conn);

        protected:
            static constexpr double kTimerInterval = 1.0;

        private:
            struct Entry
            {
                explicit Entry(std::weak_ptr<TcpConnection> conn) : 
                    conn_(std::move(conn)) {}
                ~Entry();

                std::weak_ptr<TcpConnection> conn_;
            };

            using EntryPtr = std::shared_ptr<Entry>;
            using Bucket = std::unordered_set<EntryPtr>;

            void OnTimer();

            EventLoop* loop_;
            int timeout_;
            std::vector<Bucket> wheel_;
            size_t current_bucket_;

            TimerId timer_id_;
    };
} // namespace mini_muduo