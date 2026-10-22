#include "CanIpcClient.h"
#include <QDebug>

#ifdef Q_OS_LINUX
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#endif

CanIpcClient::CanIpcClient(QObject *parent) : QObject(parent) {
#ifdef Q_OS_LINUX
    m_sockfd = socket(AF_UNIX, SOCK_DGRAM | SOCK_NONBLOCK, 0);
    if (m_sockfd < 0) {
        qWarning() << "[CanIpcClient] Cannot create IPC socket";
        return;
    }

    unlink(FastTrack::IPC_SOCKET_QT_CLIENT);
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, FastTrack::IPC_SOCKET_QT_CLIENT, sizeof(addr.sun_path) - 1);

    if (bind(m_sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        qWarning() << "[CanIpcClient] Cannot bind to" << FastTrack::IPC_SOCKET_QT_CLIENT;
        close(m_sockfd);
        m_sockfd = -1;
        return;
    }

    // Let the Qt event loop notify readable datagrams instead of blocking the UI
    // in recv(). A bound socket indicates local readiness, not CAN peer health.
    m_notifier = new QSocketNotifier(m_sockfd, QSocketNotifier::Read, this);
    connect(m_notifier, &QSocketNotifier::activated, this, &CanIpcClient::onSocketDataReady);

    m_connected = true;
    emit connectedChanged();
    qInfo() << "[CanIpcClient] Bound to" << FastTrack::IPC_SOCKET_QT_CLIENT
            << "- listening for CAN commands from can-daemon.";
#else
    qInfo() << "[CanIpcClient] Running on non-Linux platform (mock mode for QML dev).";
#endif
}

CanIpcClient::~CanIpcClient() {
#ifdef Q_OS_LINUX
    if (m_sockfd >= 0) {
        close(m_sockfd);
        unlink(FastTrack::IPC_SOCKET_QT_CLIENT);
    }
#endif
}

void CanIpcClient::onSocketDataReady() {
#ifdef Q_OS_LINUX
    FastTrack::CanCommandPacket cmd;
    // Process only a complete protocol packet; shorter datagrams are ignored.
    ssize_t bytes = recv(m_sockfd, &cmd, sizeof(cmd), 0);
    if (bytes != sizeof(cmd)) return;

    // Validate checksum
    uint8_t expected = FastTrack::calculateChecksum(cmd.commandId, cmd.seekCommand,
                                                    cmd.seekStepValue, cmd.rollingCounter);
    if (cmd.checksum != expected) {
        qWarning() << "[CanIpcClient] Checksum mismatch! Dropping frame.";
        return;
    }

    qDebug() << "[CanIpcClient] RX CMD:" << cmd.commandId << "SEEK:" << cmd.seekCommand;

    // PlayerApplication routes these signals to the shared PlayerController.
    switch (static_cast<FastTrack::PlayerCommand>(cmd.commandId)) {
        case FastTrack::PlayerCommand::PLAY:     emit commandPlay();  break;
        case FastTrack::PlayerCommand::PAUSE:    emit commandPause(); break;
        case FastTrack::PlayerCommand::NEXT:     emit commandNext();  break;
        case FastTrack::PlayerCommand::PREVIOUS: emit commandPrev();  break;
        default: break;
    }

    if (static_cast<FastTrack::SeekCommand>(cmd.seekCommand) == FastTrack::SeekCommand::FAST_FORWARD) {
        emit commandSeek(cmd.seekStepValue > 0 ? cmd.seekStepValue : 10);
    } else if (static_cast<FastTrack::SeekCommand>(cmd.seekCommand) == FastTrack::SeekCommand::FAST_BACKWARD) {
        emit commandSeek(-(cmd.seekStepValue > 0 ? cmd.seekStepValue : 10));
    }
#endif
}

void CanIpcClient::sendStatus(int playbackState, int trackIndex,
                              int curSec, int totalSec, int usbHealth) {
#ifdef Q_OS_LINUX
    if (m_sockfd < 0) return;

    // Preserve the common packet layout shared with can-daemon; the alive
    // counter advances for each send attempt and wraps at the uint8_t boundary.
    FastTrack::CanStatusPacket status;
    status.playbackState    = static_cast<uint8_t>(playbackState);
    status.trackIndex       = static_cast<uint8_t>(trackIndex);
    status.curTimeSec       = static_cast<uint16_t>(curSec);
    status.totalDurationSec = static_cast<uint16_t>(totalSec);
    status.usbHealth        = static_cast<uint8_t>(usbHealth);
    status.aliveCounter     = m_aliveCounter++;

    struct sockaddr_un srvAddr{};
    srvAddr.sun_family = AF_UNIX;
    std::strncpy(srvAddr.sun_path, FastTrack::IPC_SOCKET_CAN_DAEMON, sizeof(srvAddr.sun_path) - 1);

    sendto(m_sockfd, &status, sizeof(status), 0,
           reinterpret_cast<struct sockaddr*>(&srvAddr), sizeof(srvAddr));
#else
    Q_UNUSED(playbackState); Q_UNUSED(trackIndex);
    Q_UNUSED(curSec); Q_UNUSED(totalSec); Q_UNUSED(usbHealth);
#endif
}
