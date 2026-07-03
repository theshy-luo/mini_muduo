#include "mini_muduo/base/Logger.h"
#include <unistd.h>
#include <limits>
#include <string>

int main()
{
    // 测试不同的日志级别（g_logLevel 默认是 DEBUG，所以你看不到 TRACE）
    LOG_TRACE << "This is a TRACE log. Should not be seen by default.";
    LOG_DEBUG << "This is a DEBUG log.";
    LOG_INFO << "This is an INFO log with an int: " << 12345;
    LOG_WARN << "This is a WARN log with a string: " << std::string("Hello Logger");
    LOG_ERROR << "This is an ERROR log.";
    
    // 测试不同的数据类型转换是否正确
    LOG_INFO << "Integer max: " << std::numeric_limits<int>::max();
    LOG_INFO << "Long long min: " << std::numeric_limits<long long>::min();
    LOG_INFO << "Double value: " << 3.14159265;
    LOG_INFO << "Pointer address: " << (void*)&main;

    // 测试结束
    LOG_INFO << "Hello mini_muduo Logger! Everything works perfectly!";

    // LOG_FATAL 抛出异常，由调用方决定如何处理。
    try
    {
        LOG_FATAL << "This is a fatal error!";
    }
    catch (const mini_muduo::Exception& ex)
    {
        LOG_INFO << "Caught expected fatal exception: " << ex.what();
    }

    return 0;
}
