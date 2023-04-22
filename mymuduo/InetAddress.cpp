#include "InetAddress.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/socket.h>

#include <iostream>
#include <string>

InetAddress::InetAddress(short port, std::string ipv4) {
    bzero(&m_addr, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    m_addr.sin_addr.s_addr = inet_addr(ipv4.c_str());
}

InetAddress::InetAddress(const sockaddr_in &addr) : m_addr(addr) {}

std::string InetAddress::toIp() const {
    char buf[33] = {0};  // ipv4地址 32bit
    inet_ntop(AF_INET, &m_addr.sin_addr, buf, sizeof(buf));
    return buf;
}

short InetAddress::toPort() const { return ntohs(m_addr.sin_port); }
std::string InetAddress::toIpPort() const {
    return toIp() + ":" + std::to_string(toPort());
}

// int main() {
//     InetAddress addr(6000);
//     std::cout << addr.toIpPort() << "\t" << addr.toIp() << "\t" <<
//     addr.toPort()
//               << std::endl;
//     return 0;
// }