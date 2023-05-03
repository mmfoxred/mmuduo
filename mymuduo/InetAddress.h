#pragma once
#include <netinet/in.h>

#include <string>

class InetAddress {
public:
    explicit InetAddress(std::string ipv4 = "127.0.0.1", short port = 6000);
    explicit InetAddress(const sockaddr_in& addr);

    std::string toIp() const;
    short toPort() const;
    std::string toIpPort() const;

    const sockaddr_in* getSockAddr() const { return &m_addr; }
    void setSockAddr(const sockaddr_in& sockaddr) { m_addr = sockaddr; }

private:
    sockaddr_in m_addr;
};