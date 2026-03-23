#pragma once

#include "mini_muduo/base/LogStream.h"
#include "mini_muduo/base/Timestamp.h"

namespace mini_muduo 
{
    class Logger 
    {
        public:
            enum LogLevel 
            {
                TRACE, 
                DEBUG, 
                INFO, 
                WARN, 
                ERROR, 
                FATAL, 
                NUM_LOG_LEVELS 
            };

            // 辅助类：用来捕捉文件名
            class SourceFile 
            { 
                public:
                    template<int N>
                    SourceFile(const char (&arr)[N])
                        : data_(arr)
                        , size_(N-1)
                    {
                        const char* slash = strrchr(data_, '/');
                        if (slash)
                        {
                            data_ = slash + 1;
                            size_ -= static_cast<int>(data_ - arr);
                        }
                    }

                    SourceFile(const char* filename) 
                        : data_(filename)
                    {
                        const char* slash = strrchr(data_, '/');
                        if (slash)
                        {
                            data_ = slash + 1;
                        }
                        size_ = static_cast<int>(strlen(data_));
                    }

                    const char* data_;
                    int size_;
            };

            Logger(SourceFile file, int line);
            Logger(SourceFile file, int line, LogLevel level);
            Logger(SourceFile file, int line, LogLevel level, const char* func);

            ~Logger();// 析构时输出

            LogStream& stream() { return impl_.stream; }

        private:
            struct Impl 
            {
                Timestamp time;
                LogStream stream;
                LogLevel level;
                int line;
                SourceFile base_name;

                Impl(LogLevel level, int ole_errno, const SourceFile& file, int line);

                void FormatFileTime();
                void Finish();
            };

            Impl impl_;
    };

    extern Logger::LogLevel g_log_level;

    inline Logger::LogLevel GetLogLevel()
    {
        return g_log_level;
    }

    #define LOG_TRACE \
        if (mini_muduo::Logger::TRACE <= mini_muduo::g_log_level) \
        mini_muduo::Logger(__FILE__, __LINE__, mini_muduo::Logger::TRACE, __func__).stream()

    #define LOG_DEBUG \
        if (mini_muduo::Logger::DEBUG <= mini_muduo::g_log_level) \
        mini_muduo::Logger(__FILE__, __LINE__, mini_muduo::Logger::DEBUG, __func__).stream()

    #define LOG_INFO \
        if (mini_muduo::Logger::INFO <= mini_muduo::g_log_level) \
        mini_muduo::Logger(__FILE__, __LINE__, mini_muduo::Logger::INFO, __func__).stream()

    #define LOG_WARN \
        if (mini_muduo::Logger::WARN <= mini_muduo::g_log_level) \
        mini_muduo::Logger(__FILE__, __LINE__, mini_muduo::Logger::WARN, __func__).stream()

    #define LOG_ERROR \
        if (mini_muduo::Logger::ERROR <= mini_muduo::g_log_level) \
        mini_muduo::Logger(__FILE__, __LINE__, mini_muduo::Logger::ERROR, __func__).stream()

    #define LOG_FATAL \
        if (mini_muduo::Logger::FATAL <= mini_muduo::g_log_level) \
        mini_muduo::Logger(__FILE__, __LINE__, mini_muduo::Logger::FATAL, __func__).stream()
} // namespace mini_muduo