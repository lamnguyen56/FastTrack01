/**
 * @file CanApplication.h
 * @brief CAN daemon composition and event loop.
 */

#pragma once
namespace canbus {

/**
 * @brief Coordinate SocketCAN, protocol codec and IPC transport.
 */
class CanApplication {

public:
    /**
     * @brief Run the nonblocking CAN/IPC bridge until SIGINT or SIGTERM.
     * @return Zero on orderly shutdown, one when socket initialization fails.
     * @pre setup_can.sh has configured the CAN interface.
     */
    int run();
};

}
