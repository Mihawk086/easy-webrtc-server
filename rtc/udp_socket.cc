#include "udp_socket.h"

#include <iostream>

#include "muduo/net/Channel.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/Socket.h"
#include "muduo/net/SocketsOps.h"

using namespace muduo;
using namespace muduo::net;

UdpSocket::UdpSocket(muduo::net::EventLoop* loop, std::string ip, uint16_t port)
    : loop_(loop), ip_(ip), port_(port) {}

UdpSocket::~UdpSocket() {
  int fd = channel_->fd();
  if (fd > 0) {
    sockets::close(fd);
    loop_->removeChannel(channel_.get());
  }
}

void UdpSocket::Start() {
  fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(ip_.c_str());
  addr.sin_port = ::htons(port_);
  int ret = ::bind(fd_, (struct sockaddr*)&addr, sizeof addr);
  if (ret != 0) {
    std::cout << "bind udp error" << std::endl;
  }
  {
    struct sockaddr addr;
    struct sockaddr_in* addr_v4;
    socklen_t addr_len = sizeof(addr);
    if (0 == ::getsockname(fd_, &addr, &addr_len)) {
      if (addr.sa_family == AF_INET) {
        addr_v4 = (sockaddr_in*)&addr;
        port_ = ntohs(addr_v4->sin_port);
      }
    }
  }

  channel_.reset(new Channel(loop_, fd_));
  channel_->setReadCallback([this](Timestamp time) { this->handleRead(); });
  channel_->enableReading();
  loop_->updateChannel(channel_.get());
}

int UdpSocket::Send(char* buf, int len, const sockaddr_in& remoteAddr) {
  int ret =
      sendto(fd_, (const char*)buf, len, 0, (struct sockaddr*)&remoteAddr, sizeof(remoteAddr));
  if (ret < 0) {
    std::cout << "send error" << std::endl;
    return -1;
  }
  return ret;
}

void UdpSocket::handleRead() {
  struct sockaddr_in remoteAddr;
  unsigned int nAddrLen = sizeof(remoteAddr);
  int recvLen = recvfrom(fd_, buf_, KBuffSize, 0, (struct sockaddr*)&remoteAddr, &nAddrLen);
  if (read_callback_) {
    read_callback_(buf_, recvLen, &remoteAddr);
  }
}
