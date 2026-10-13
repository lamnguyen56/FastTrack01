#pragma once

#include <Arduino.h>
#include <driver/twai.h>
#include <functional>
#include "Config.h"
#include "CanProtocol.h"

// ============================================================================
// CLASS QUẢN LÝ NGOẠI VI CAN TWAI (Two-Wire Automotive Interface)
// ============================================================================

class TwaiManager {
public:
    typedef std::function<void(const CanStatusPacket&)> TelemetryCallback;
    typedef std::function<void(const char* logMsg, bool isTx)> LogCallback;

    static TwaiManager& getInstance();

    // Khởi tạo ngoại vi TWAI ở 500kbps và tạo task nhận frame
    bool begin();

    // Đóng gói và truyền frame CAN ID 0x100
    bool sendCommand(uint8_t cmdId, uint8_t seekCmd, uint8_t seekVal);

    // Đăng ký callback khi nhận telemetry ID 0x101
    void setTelemetryCallback(TelemetryCallback cb);

    // Đăng ký callback khi có log frame TX / RX
    void setLogCallback(LogCallback cb);

    // Lấy Rolling Counter hiện tại
    uint8_t getRollingCounter() const { return m_rollingCounter; }

private:
    TwaiManager();
    ~TwaiManager();
    TwaiManager(const TwaiManager&) = delete;
    TwaiManager& operator=(const TwaiManager&) = delete;

    static void rxTaskTrampoline(void *arg);
    void rxTaskLoop();

    SemaphoreHandle_t m_mutex;
    uint8_t m_rollingCounter;
    bool m_isInitialized;

    TelemetryCallback m_telemetryCb;
    LogCallback m_logCb;
};
