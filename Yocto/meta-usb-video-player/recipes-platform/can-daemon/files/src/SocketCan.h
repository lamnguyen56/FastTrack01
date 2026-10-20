/**
 * @file SocketCan.h
 * @brief Nonblocking Linux SocketCAN transport.
 */

#pragma once

#include <linux/can.h>

namespace canbus {

/**
 * @brief Own a nonblocking Linux CAN RAW socket.
 * @note Interface bitrate/link setup belongs to setup_can.sh, not this class.
 */
class SocketCan {

public:
    /**
     * @brief Create an instance with no open descriptor.
     */
    SocketCan() = default;

    /**
     * @brief Close any owned CAN socket.
     */
    ~SocketCan();

    /**
     * @brief Disallow copying the owned socket.
     */
    SocketCan(const SocketCan&) = delete;

    /**
     * @brief Disallow assigning the owned socket.
     */
    SocketCan& operator=(const SocketCan&) = delete;

    /**
     * @brief Open and bind a CAN RAW socket to the named interface.
     * @param interface Null-terminated Linux interface name, e.g. can0.
     * @return True if socket creation, interface lookup and bind succeed.
     * @pre Call once per instance.
     */
    bool open(const char* interface);

    /**
     * @brief Try to read one complete Classical CAN frame without blocking.
     * @param[out] frame Valid only when the method returns true.
     * @return True for a full frame; false for no data, errors or incomplete reads.
     */
    bool receive(can_frame& frame);

    /**
     * @brief Try to transmit a complete frame without blocking.
     * @param frame Frame to submit to the CAN socket.
     * @return True if accepted by the socket; this does not prove bus delivery.
     */
    bool send(const can_frame& frame);

private:
    /**
     * @brief Owned CAN descriptor, or -1 before creation.
     */
    int fd_ = -1;
};

}
