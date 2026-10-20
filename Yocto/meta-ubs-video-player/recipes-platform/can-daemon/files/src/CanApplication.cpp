/**
 * @file CanApplication.cpp
 * @brief CAN daemon composition and event loop.
 */

#include "CanApplication.h"
#include "SocketCan.h"
#include "IpcChannel.h"
#include "ProtocolCodec.h"
#include <csignal>
#include <chrono>
#include <thread>
#include "Logger.h"

namespace canbus {

namespace {
/**
 * @brief Signal-safe request to terminate the bridge loop.
 */
volatile std::sig_atomic_t running = 1;

/**
 * @brief Request shutdown; resource cleanup runs outside signal context.
 */
void stop(int) {
    running = 0;
}

}

int CanApplication::run() {
    running = 1;
    std::signal(SIGINT, stop);
    std::signal(SIGTERM, stop);

    // Interface configuration belongs to setup_can.sh (systemd ExecStartPre).
    SocketCan can;
    IpcChannel ipc;
    if (can.open(FastTrack::DEFAULT_CAN_INTERFACE) == false || ipc.open() == false) {
        LOG_E("[can] Cannot initialize CAN/IPC sockets");
        return 1;
    }
    LOG_I("CAN service started: ", FastTrack::DEFAULT_CAN_INTERFACE);
    while (running != 0) {
        can_frame frame{};
        FastTrack::CanCommandPacket command{};
        // CAN command path: transport -> codec -> existing Qt IPC endpoint.
        if (can.receive(frame) && ProtocolCodec::decodeCommand(frame, command)) {
            ipc.send(command);
        }
        FastTrack::CanStatusPacket status{};
        // Status path: IPC snapshot -> big-endian CAN payload -> SocketCAN.
        if (ipc.receive(status)) {
            can.send(ProtocolCodec::encodeStatus(status));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return 0;
}

}
