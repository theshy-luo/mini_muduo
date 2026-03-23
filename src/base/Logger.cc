#include "mini_muduo/base/Logger.h"
#include "mini_muduo/base/CurrentThread.h"
#include "mini_muduo/base/LogStream.h"

#include <cassert>
#include <ctime>
#include <cstdio>
#include <sys/select.h>
#include <string.h>

#define CSI_START                      "\033["
#define CSI_END                        "\033[0m"
/* output log front color */
#define F_BLACK                        "30;"
#define F_RED                          "31;"
#define F_GREEN                        "32;"
#define F_YELLOW                       "33;"
#define F_BLUE                         "34;"
#define F_MAGENTA                      "35;"
#define F_CYAN                         "36;"
#define F_WHITE                        "37;"
/* output log background color */
#define B_BLACK                        "40;"
#define B_RED                          "41;"
#define B_GREEN                        "42;"
#define B_YELLOW                       "43;"
#define B_BLUE                         "44;"
#define B_MAGENTA                      "45;"
#define B_CYAN                         "46;"
#define B_WHITE                        "47;"
/* output log fonts style */
#define S_BOLD                         "1m"
#define S_UNDERLINE                    "4m"
#define S_BLINK                        "5m"
#define S_NORMAL                       "22m"

namespace mini_muduo 
{
    __thread char t_errnobuf[512];
    
    Logger::LogLevel g_log_level = Logger::NUM_LOG_LEVELS;

    static const char* log_level_name[] = 
    {
        "TRACE",
        "DEBUG",
        "INFO",
        "WARN",
        "ERROR",
        "FATAL",
    };

    static const char* log_level_font_style_start[] = 
    {
        CSI_START F_BLACK S_NORMAL,
        CSI_START F_BLUE S_NORMAL,
        CSI_START F_GREEN S_NORMAL,
        CSI_START F_YELLOW S_NORMAL,
        CSI_START F_RED S_NORMAL,
        CSI_START F_MAGENTA S_NORMAL,
    };

    Logger::Impl::Impl(LogLevel level, int save_errno, 
        const SourceFile& file, int line)
        : time(Timestamp::Now())
        , stream()
        , level(level)
        , line(line)
        , base_name(file)
    {
        stream << log_level_font_style_start[level];
        FormatFileTime();
        stream << CurrentThread::tid() << ' ';
        stream << log_level_name[level] << ' ';
        if (save_errno != 0)
        {
            stream << strerror_r(save_errno, t_errnobuf, sizeof(t_errnobuf)) 
                << " (errno=" << save_errno << ") ";
        }
    }

    void Logger::Impl::FormatFileTime()
    {
        long int seconds = static_cast<long int>(time.second_since_epoch());
        long int milli_seconds = static_cast<long int>(time.milli_seconds_since_epoch() - seconds * 1000);

        struct tm tm_time;
        ::localtime_r(&seconds, &tm_time);

        // 转换为年月日时分秒（UTC 时间）
        char buf[64];
        int len = snprintf(buf, sizeof(buf),
            "%4d-%02d-%02d %02d:%02d:%02d.%03ld ",
            tm_time.tm_year + 1900,
            tm_time.tm_mon + 1,
            tm_time.tm_mday,
            tm_time.tm_hour,
            tm_time.tm_min,
            tm_time.tm_sec,
            milli_seconds);

        assert(len > 0);
        
        stream << buf;
    }

    void Logger::Impl::Finish()
    {
        stream << " - " << base_name.data_ << ":" << line << "\n";
        stream << CSI_END;
    }   

    Logger::Logger(SourceFile file, int line)
        : impl_(g_log_level, 0, file, line)
    {
    }

    Logger::Logger(SourceFile file, int line, LogLevel level)
        : impl_(level, 0, file, line)
    {
    }

    Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
        : impl_(level, 0, file, line)
    {
        impl_.stream << func << ' ';
    }

    Logger::~Logger()
    {
        impl_.Finish();

        const LogStream::Buffer& buf(stream().buffer());
        fwrite(buf.data(), buf.length(), 1, stdout);

        if (impl_.level == FATAL) 
        {
            fflush(stdout);
            abort();
        }
    }
} // namespace mini_muduo