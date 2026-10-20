/**
 * @file SocketCan.cpp
 * @brief Nonblocking Linux SocketCAN transport.
 */

#include "SocketCan.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <cstring>
#include "Logger.h"

namespace canbus {

SocketCan::~SocketCan() {
    if (fd_ >= 0) {
        ::close(fd_);
    }
}

bool SocketCan::open(const char* interface) {
    // Nonblocking I/O keeps a missing frame or congested bus from freezing IPC.
    fd_ = ::socket(PF_CAN, SOCK_RAW | SOCK_NONBLOCK, CAN_RAW);
    if (fd_ < 0) {
        return false;
    }
    ifreq request{};
    std::strncpy(request.ifr_name, interface, IFNAMSIZ - 1);
    if (::ioctl(fd_, SIOCGIFINDEX, &request) < 0) {
        return false;
    }
    sockaddr_can address{};
    address.can_family = AF_CAN;
    address.can_ifindex = request.ifr_ifindex;

    return ::bind(fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
}

bool SocketCan::receive(can_frame& frame) {
    return ::recv(fd_, &frame, sizeof(frame), MSG_DONTWAIT) == static_cast<ssize_t>(sizeof(frame));
}

bool SocketCan::send(const can_frame& frame) {
    if (::write(fd_, &frame, sizeof(frame)) == static_cast<ssize_t>(sizeof(frame))) {
        return true;
    }
    LOG_E("[can] Frame transmission failed");

    return false;
}

}
