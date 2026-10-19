/**
 * @file IpcChannel.h
 * @brief Unix datagram transport for the Qt peer.
 */

#pragma once

#include "Constants.h"

namespace canbus {

// Current transport: Unix datagrams. Message-queue migration is separate work.
/**
 * @brief Own the Unix datagram IPC endpoint used by the existing Qt client.
 * @note Packet layout and transport are unchanged; this is not a POSIX message queue.
 */
class IpcChannel {

public:
    /**
     * @brief Create an inactive endpoint.
     */
    IpcChannel() = default;

    /**
     * @brief Close the socket and remove its path only after a successful bind.
     */
    ~IpcChannel();

    /**
     * @brief Disallow copying the owned endpoint.
     */
    IpcChannel(const IpcChannel&) = delete;

    /**
     * @brief Disallow assigning the owned endpoint.
     */
    IpcChannel& operator=(const IpcChannel&) = delete;

    /**
     * @brief Bind the configured daemon Unix socket path.
     * @return True when creation and bind succeed.
     * @pre Single daemon instance; call once per instance.
     */
    bool open();

    /**
     * @brief Try to receive one status datagram of exactly the expected size.
     * @param[out] status Valid only on success.
     * @return False for no data, socket errors or a different payload length.
     */
    bool receive(FastTrack::CanStatusPacket& status);

    /**
     * @brief Forward a decoded command to the configured Qt socket.
     * @param command Command payload in the existing local IPC format.
     * @return True if the full payload is accepted; false also when the peer is absent.
     */
    bool send(const FastTrack::CanCommandPacket& command);

private:
    /**
     * @brief Owned Unix datagram descriptor, or -1 before creation.
     */
    int fd_ = -1;

    /**
     * @brief Whether this instance bound the socket path and must unlink it.
     */
    bool bound_ = false;
};

}
