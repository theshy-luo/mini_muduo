#pragma once

#include <cstdint>
#include <stdint.h>
#include <string>
#include <netinet/in.h>

namespace mini_muduo 
{
    // 封装 IPv4 地址
    class InetAddress
    {
        public:
            explicit InetAddress(uint16_t port = 0, 
                std::string ip = "127.0.0.1");
            
            explicit InetAddress(const struct sockaddr_in& addr) :
            addr_(addr)
            {}

            ~InetAddress();

            std::string GetIp() const;
            std::string GetIpPort() const;
            uint16_t GetPort() const;

            sa_family_t family() const { return addr_.sin_family; }

            const struct sockaddr_in* GetSockAddr() const
            {
                return &addr_;
            }

            struct sockaddr_in* GetSockAddr()
            {
                return &addr_;
            }

            void SetSockAddr(const struct sockaddr_in& addr)
            {
                addr_ = addr;
            }

        private:
        struct sockaddr_in addr_;
    };
} // namespace mini_muduo