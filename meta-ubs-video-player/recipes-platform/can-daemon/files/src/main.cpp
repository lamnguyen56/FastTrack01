/**
 * @file main.cpp
 * @brief CAN Daemon - Tầng Platform xử lý SocketCAN can0
 *
 * Chức năng:
 *   1. Tự động cấu hình & bring-up interface can0 (500 kbps, restart-ms 100)
 *   2. Nhận frame CAN ID 0x100 từ ESP32/CANoe -> chuyển tiếp binary struct tới Qt App qua UDS
 *   3. Nhận status struct từ Qt App qua UDS -> đóng gói & phát frame CAN ID 0x101
 *
 * IPC: Unix Domain Socket (SOCK_DGRAM)
 *   - can-daemon lắng nghe tại: /tmp/can_ipc.sock
 *   - Qt App lắng nghe tại:     /tmp/qt_app_ipc.sock
 */

#include <iostream>
#include <cstring>
#include <chrono>
#include <thread>
#include <csignal>
#include <atomic>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include "Constants.h"

static std::atomic<bool> g_running{true};

static void signalHandler(int) {
    g_running = false;
}

// ============================================================================
// Tự động cấu hình interface can0 lên 500 kbps (thay thế setup_can.sh)
// ============================================================================
static void bringUpCanInterface() {
    std::cout << "[can-daemon] Configuring " << FastTrack::DEFAULT_CAN_INTERFACE
              << " @ " << FastTrack::CAN_BITRATE << " bps...\n";
    system("ip link set can0 down 2>/dev/null");
    system("ip link set can0 type can bitrate 500000 restart-ms 100 2>/dev/null");
    system("ip link set can0 txqueuelen 1000 2>/dev/null");
    system("ip link set can0 up 2>/dev/null");
}

// ============================================================================
// Khởi tạo SocketCAN raw socket
// ============================================================================
static int openCanSocket() {
    int sock = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (sock < 0) {
        std::cerr << "[can-daemon] ERROR: Cannot open SocketCAN socket\n";
        return -1;
    }

    struct ifreq ifr;
    std::strncpy(ifr.ifr_name, FastTrack::DEFAULT_CAN_INTERFACE, IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "[can-daemon] ERROR: Interface " << FastTrack::DEFAULT_CAN_INTERFACE << " not found\n";
        close(sock);
        return -1;
    }

    struct sockaddr_can addr{};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[can-daemon] ERROR: Cannot bind to can0\n";
        close(sock);
        return -1;
    }

    return sock;
}

// ============================================================================
// Khởi tạo Unix Domain Socket (SOCK_DGRAM) cho IPC
// ============================================================================
static int openIpcSocket() {
    int sock = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (sock < 0) return -1;

    unlink(FastTrack::IPC_SOCKET_CAN_DAEMON);
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, FastTrack::IPC_SOCKET_CAN_DAEMON, sizeof(addr.sun_path) - 1);

    if (bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}

int main() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "=========================================================\n";
    std::cout << " CAN Daemon (Platform Layer)                             \n";
    std::cout << " Protocol: SocketCAN can0 @ 500 kbps (ISO 11898-2)       \n";
    std::cout << " IPC:      Unix Domain Socket (" << FastTrack::IPC_SOCKET_CAN_DAEMON << ")\n";
    std::cout << "=========================================================\n";

    bringUpCanInterface();

    int canSock = openCanSocket();
    int ipcSock = openIpcSocket();
    if (canSock < 0 || ipcSock < 0) {
        std::cerr << "[can-daemon] FATAL: Cannot initialize sockets. Exiting.\n";
        return 1;
    }

    std::cout << "[can-daemon] Running. Bridging CAN <-> IPC...\n";

    while (g_running) {
        // ---- CAN RX -> IPC TX (ESP32 gửi lệnh -> Qt App nhận) ----
        struct can_frame rxFrame;
        int nbytes = recv(canSock, &rxFrame, sizeof(rxFrame), MSG_DONTWAIT);
        if (nbytes > 0 && rxFrame.can_id == FastTrack::CAN_ID_COMMAND && rxFrame.can_dlc >= 5) {
            FastTrack::CanCommandPacket cmd;
            cmd.commandId      = rxFrame.data[0];
            cmd.seekCommand    = rxFrame.data[1];
            cmd.seekStepValue  = rxFrame.data[2];
            cmd.rollingCounter = rxFrame.data[3];
            cmd.checksum       = rxFrame.data[4];

            // Chuyển tiếp tới Qt App qua IPC
            struct sockaddr_un clientAddr{};
            clientAddr.sun_family = AF_UNIX;
            std::strncpy(clientAddr.sun_path, FastTrack::IPC_SOCKET_QT_CLIENT, sizeof(clientAddr.sun_path) - 1);
            sendto(ipcSock, &cmd, sizeof(cmd), 0,
                   reinterpret_cast<struct sockaddr*>(&clientAddr), sizeof(clientAddr));
        }

        // ---- IPC RX -> CAN TX (Qt App gửi status -> ESP32 nhận) ----
        FastTrack::CanStatusPacket status;
        int ipcBytes = recv(ipcSock, &status, sizeof(status), MSG_DONTWAIT);
        if (ipcBytes == sizeof(status)) {
            struct can_frame txFrame{};
            txFrame.can_id  = FastTrack::CAN_ID_STATUS;
            txFrame.can_dlc = FastTrack::CAN_DLC;
            txFrame.data[0] = status.playbackState;
            txFrame.data[1] = status.trackIndex;
            txFrame.data[2] = (status.curTimeSec >> 8) & 0xFF;       // Big Endian
            txFrame.data[3] = status.curTimeSec & 0xFF;
            txFrame.data[4] = (status.totalDurationSec >> 8) & 0xFF; // Big Endian
            txFrame.data[5] = status.totalDurationSec & 0xFF;
            txFrame.data[6] = status.usbHealth;
            txFrame.data[7] = status.aliveCounter;

            write(canSock, &txFrame, sizeof(txFrame));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    close(canSock);
    close(ipcSock);
    unlink(FastTrack::IPC_SOCKET_CAN_DAEMON);
    std::cout << "[can-daemon] Stopped.\n";
    return 0;
}
