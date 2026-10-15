#ifndef CAN_IPC_CLIENT_H
#define CAN_IPC_CLIENT_H

#include <QObject>

#ifdef Q_OS_LINUX
#include <QSocketNotifier>
#endif

#include "Constants.h"

/**
 * @class CanIpcClient
 * @brief Kết nối tới can-daemon (Platform Layer) qua Unix Domain Socket
 *
 * Nhận lệnh điều khiển CAN (Play/Pause/Next/Prev/Seek) và phát Qt Signals cho QML.
 * Gửi trạng thái phát lại cho can-daemon để phát lên CAN bus (ID 0x101).
 *
 * Trên Windows: Chạy ở chế độ mock (không có UDS), tiện cho dev/test QML trên desktop.
 */
class CanIpcClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectedChanged)

public:
    explicit CanIpcClient(QObject *parent = nullptr);
    ~CanIpcClient();

    bool isConnected() const { return m_connected; }

    /// Được gọi từ QML/C++ để gửi telemetry ngược lại cho can-daemon
    Q_INVOKABLE void sendStatus(int playbackState, int trackIndex,
                                int curSec, int totalSec, int usbHealth);

signals:
    void commandPlay();
    void commandPause();
    void commandNext();
    void commandPrev();
    void commandSeek(int seconds);  // dương = FF, âm = REW
    void connectedChanged();

private slots:
    void onSocketDataReady();

private:
    int m_sockfd{-1};
    bool m_connected{false};
    uint8_t m_aliveCounter{0};

#ifdef Q_OS_LINUX
    QSocketNotifier *m_notifier{nullptr};
#endif
};

#endif // CAN_IPC_CLIENT_H
