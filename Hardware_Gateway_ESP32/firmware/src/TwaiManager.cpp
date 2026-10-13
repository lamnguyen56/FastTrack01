#include "TwaiManager.h"

TwaiManager& TwaiManager::getInstance() {
    static TwaiManager instance;
    return instance;
}

TwaiManager::TwaiManager()
    : m_mutex(NULL)
    , m_rollingCounter(0)
    , m_isInitialized(false)
    , m_telemetryCb(nullptr)
    , m_logCb(nullptr)
{
}

TwaiManager::~TwaiManager() {
    if (m_isInitialized) {
        twai_stop();
        twai_driver_uninstall();
    }
    if (m_mutex) {
        vSemaphoreDelete(m_mutex);
    }
}

bool TwaiManager::begin() {
    m_mutex = xSemaphoreCreateMutex();
    if (!m_mutex) {
        Serial.println("[TWAI ERROR] Không thể tạo FreeRTOS Mutex!");
        return false;
    }

    // Cấu hình TWAI driver ở bitrate 500 kbps chuẩn Automotive
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t  t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t  f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        Serial.printf("[TWAI ERROR] Lỗi cài đặt driver TWAI (Code: 0x%X)\n", err);
        return false;
    }

    err = twai_start();
    if (err != ESP_OK) {
        Serial.printf("[TWAI ERROR] Lỗi kích hoạt TWAI (Code: 0x%X)\n", err);
        return false;
    }

    m_isInitialized = true;
    Serial.println("[TWAI] Khởi tạo Driver TWAI 500 kbps thành công.");

    // Tạo Task FreeRTOS chạy trên Core 0 với mức ưu tiên cao
    xTaskCreatePinnedToCore(
        rxTaskTrampoline,
        "TwaiRxTask",
        4096,
        this,
        5,
        NULL,
        0
    );

    return true;
}

bool TwaiManager::sendCommand(uint8_t cmdId, uint8_t seekCmd, uint8_t seekVal) {
    if (!m_isInitialized || !m_mutex) return false;

    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        Serial.println("[TWAI WARN] Không thể chiếm Mutex gửi lệnh CAN!");
        return false;
    }

    twai_message_t txMsg;
    packCanCommand(cmdId, seekCmd, seekVal, m_rollingCounter, txMsg);

    esp_err_t res = twai_transmit(&txMsg, pdMS_TO_TICKS(50));
    xSemaphoreGive(m_mutex);

    if (res == ESP_OK) {
        char logBuf[128];
        snprintf(logBuf, sizeof(logBuf), 
                 "TX 0x100 [8]: Cmd=%02X Seek=%02X Val=%02X Ctr=%d CRC=%02X", 
                 cmdId, seekCmd, seekVal, m_rollingCounter, txMsg.data[4]);
        Serial.printf("[TWAI] %s\n", logBuf);

        if (m_logCb) {
            m_logCb(logBuf, true);
        }
        return true;
    } else {
        Serial.printf("[TWAI ERROR] Lỗi truyền frame (Code: 0x%X)\n", res);
        return false;
    }
}

void TwaiManager::setTelemetryCallback(TelemetryCallback cb) {
    m_telemetryCb = cb;
}

void TwaiManager::setLogCallback(LogCallback cb) {
    m_logCb = cb;
}

void TwaiManager::rxTaskTrampoline(void *arg) {
    TwaiManager *self = static_cast<TwaiManager*>(arg);
    self->rxTaskLoop();
}

void TwaiManager::rxTaskLoop() {
    twai_message_t rxMsg;
    CanStatusPacket status;

    for (;;) {
        // Chờ nhận frame CAN từ ngắt phần cứng
        if (twai_receive(&rxMsg, pdMS_TO_TICKS(100)) == ESP_OK) {
            if (unpackCanStatus(rxMsg, status)) {
                // Kích hoạt callback đẩy dữ liệu Telemetry
                if (m_telemetryCb) {
                    m_telemetryCb(status);
                }

                // Ghi log định kỳ mỗi 1 giây (10 frame x 100ms) để không làm nghẽn giao diện
                if (status.aliveCounter % 10 == 0) {
                    char logBuf[128];
                    snprintf(logBuf, sizeof(logBuf),
                             "RX 0x101 [8]: State=%d Track=%d Time=%d/%ds USB=%d Alive=%d",
                             status.playbackState, status.trackIndex,
                             status.curTimeSec, status.totalDurationSec,
                             status.usbHealth, status.aliveCounter);
                    
                    if (m_logCb) {
                        m_logCb(logBuf, false);
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
