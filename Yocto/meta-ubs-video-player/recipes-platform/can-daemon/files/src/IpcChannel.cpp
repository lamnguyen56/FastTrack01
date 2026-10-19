/**
 * @file IpcChannel.cpp
 * @brief Unix datagram transport for the Qt peer.
 */

#include "IpcChannel.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>

namespace canbus {

IpcChannel::~IpcChannel() {
    if (fd_ >= 0) {
        ::close(fd_);
    }

    if (bound_ == true) {
        ::unlink(FastTrack::IPC_SOCKET_CAN_DAEMON);
    }
}

bool IpcChannel::open() {
    fd_ = ::socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (fd_ < 0) {
        return false;
    }

    // Remove a stale endpoint from a previous run; the service must be singleton.
    ::unlink(FastTrack::IPC_SOCKET_CAN_DAEMON);
    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    std::strncpy(address.sun_path, FastTrack::IPC_SOCKET_CAN_DAEMON, sizeof(address.sun_path) - 1);
    bound_ = ::bind(fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;

    return bound_;
}

bool IpcChannel::receive(FastTrack::CanStatusPacket& status) {
    // MSG_TRUNC reports full datagram size so oversized payloads are rejected.
    return ::recv(fd_, &status, sizeof(status), MSG_DONTWAIT | MSG_TRUNC) == static_cast<ssize_t>(sizeof(status));
}

bool IpcChannel::send(const FastTrack::CanCommandPacket& command) {
    sockaddr_un client{};
    client.sun_family = AF_UNIX;
    std::strncpy(client.sun_path, FastTrack::IPC_SOCKET_QT_CLIENT, sizeof(client.sun_path) - 1);

    return ::sendto(fd_, &command, sizeof(command), 0,
    reinterpret_cast<sockaddr*>(&client), sizeof(client)) == static_cast<ssize_t>(sizeof(command));
}

}
