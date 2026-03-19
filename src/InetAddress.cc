#include "mini_muduo/InetAddress.h"
#include <arpa/inet.h>
#include <cstdint>
#include <string>
#include <strings.h>

namespace mini_muduo 
{
    InetAddress::InetAddress(uint16_t port, std::string ip)
    {
        bzero(&addr_, sizeof(addr_));

        addr_.sin_family = AF_INET;
        addr_.sin_port = htons(port);
        addr_.sin_addr.s_addr = ip.empty() ? htonl(INADDR_ANY) : ::inet_addr(ip.c_str());
    }

    InetAddress::~InetAddress()
    {

    }

    std::string InetAddress::GetIp() const
    {
        return std::string(::inet_ntoa(addr_.sin_addr));
    }  

    std::string InetAddress::GetIpPort() const
    {
        // 1. 获取 IP 字符串
        char ip[64] = {0};
        ::inet_ntop(AF_INET, &addr_.sin_addr, ip, sizeof(ip));

        // 2. 获取端口号 (注意：网络字节序转主机字节序)
        uint16_t port = ntohs(addr_.sin_port);

        // 3. 格式化输出 "IP:Port"
        char buf[80] = {0};
        sprintf(buf, "%s:%d", ip, port);
        
        return buf;
    }

    uint16_t InetAddress::GetPort() const
    {
        return ntohs(addr_.sin_port);
    }


}