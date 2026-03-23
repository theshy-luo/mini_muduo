#include "mini_muduo/base/LogStream.h"

#include <iostream>
#include <algorithm>
#include <cassert>
#include <cstddef>

namespace mini_muduo 
{
    namespace detail 
    {
        const char digits[] = "9876543210123456789";
        const char digitsHex[] = "0123456789ABCDEF";
        const char* zero = digits + 9;

        // 高效地将整数转为字符串
        template<typename T>
        size_t convert(char buf[], T value)
        {
            T i = value;
            char* p = buf;

            do {
                int lsd = static_cast<int>(i % 10);
                i /= 10;
                *p++ = zero[lsd];
            }while (i != 0);

            if (value < 0) 
            {
                *p++ = '-';
            }

            std::reverse(buf, p);

            return p - buf;
        }

        // 高效地将十六进制整数转为字符串
        size_t convertHex(char buf[], uintptr_t value)
        {
            uintptr_t i = value;
            char* p = buf;

            do
            {
                int lsd = static_cast<int>(i % 16);
                i /= 16;
                *p++ = digitsHex[lsd];
            } while (i != 0);

            *p = '\0';
            std::reverse(buf, p);

            return p - buf;
        }
    }

    template <typename T>
    void LogStream::FormatInteger(T value)
    {
        if (buffer_.avail() < kMaxNumericSize) 
        {
            std::cerr << "LogStream::FormatInteger() buffer is full!" << std::endl;
            assert(false);
        }
        size_t len = detail::convert(buffer_.current(), value);
        buffer_.add(len);
    }

    LogStream& LogStream::operator<<(char c)
    {
        buffer_.append(&c, 1);
        return *this;
    }

    LogStream& LogStream::operator<<(const char *const str)
    {
        if (str) 
        {
            buffer_.append(str, strlen(str));
        }
        return *this;
    }

    LogStream& LogStream::operator<<(short value)
    {
        *this << static_cast<int>(value);
        return *this;
    }

    LogStream& LogStream::operator<<(unsigned short value)
    {
        *this << static_cast<unsigned int>(value);
        return *this;
    }

    LogStream& LogStream::operator<<(int value)
    {
        FormatInteger(value);
        return *this;
    }

    LogStream& LogStream::operator<<(unsigned int value)
    {
        FormatInteger(value);
        return *this;
    }

    LogStream& LogStream::operator<<(long value)
    {
        FormatInteger(value);
        return *this;
    }

    LogStream& LogStream::operator<<(long long value)
    {
        FormatInteger(value);
        return *this;
    }

    LogStream& LogStream::operator<<(unsigned long value)
    {
        FormatInteger(value);
        return *this;
    }

    LogStream& LogStream::operator<<(unsigned long long value)
    {
        FormatInteger(value);
        return *this;
    }

    LogStream& LogStream::operator<<(float value)
    {
        *this << static_cast<double>(value);
        return *this;
    }

    LogStream& LogStream::operator<<(double value)
    {
        if (buffer_.avail() < kMaxNumericSize) 
        {
            assert(false);
        }
        int len = snprintf(buffer_.current(), kMaxNumericSize, "%.12f", value);
        assert(len >= 0);
        buffer_.add(len);
        return *this;
    }

    LogStream& LogStream::operator<<(const std::string& str)
    {
        if (!str.empty()) 
        {
            buffer_.append(str.c_str(), str.size());
        }

        return *this;
    }

    LogStream& LogStream::operator<<(const Buffer& buf)
    {
        if (buf.length()) 
        {
            buffer_.append(buf.data(), buf.length());
        }
        return *this;
    }

    LogStream& LogStream::operator<<(const void* ptr)
    {
        uintptr_t v = reinterpret_cast<uintptr_t>(ptr);
        if (buffer_.avail() >= kMaxNumericSize)
        {
            char* buf = buffer_.current();
            buf[0] = '0';
            buf[1] = 'x';
            size_t len = detail::convertHex(buf+2, v);
            buffer_.add(len+2);
        }
        return *this;
    }
}