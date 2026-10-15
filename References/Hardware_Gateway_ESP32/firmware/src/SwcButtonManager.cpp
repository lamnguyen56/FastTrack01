#include "SwcButtonManager.h"

SwcButtonManager& SwcButtonManager::getInstance() {
    static SwcButtonManager instance;
    return instance;
}

SwcButtonManager::SwcButtonManager()
    : m_actionCb(nullptr)
    , m_currentPlaybackState(STATE_STOPPED)
{
}

SwcButtonManager::~SwcButtonManager() {
}

void SwcButtonManager::begin() {
    pinMode(SWC_BTN_PLAY_PIN, INPUT_PULLUP);
    pinMode(SWC_BTN_NEXT_PIN, INPUT_PULLUP);
    pinMode(SWC_BTN_PREV_PIN, INPUT_PULLUP);

    Serial.println("[SWC] Khởi tạo các chân phím cứng vô lăng (GPIO 18, 19, 23).");

    // Tạo Task FreeRTOS chạy trên Core 1 để quét phím
    xTaskCreatePinnedToCore(
        buttonTaskTrampoline,
        "SwcBtnTask",
        2048,
        this,
        2,
        NULL,
        1
    );
}

void SwcButtonManager::setActionCallback(ButtonActionCallback cb) {
    m_actionCb = cb;
}

void SwcButtonManager::buttonTaskTrampoline(void *arg) {
    SwcButtonManager *self = static_cast<SwcButtonManager*>(arg);
    self->buttonTaskLoop();
}

void SwcButtonManager::buttonTaskLoop() {
    for (;;) {
        // 1. Phím Play / Pause
        if (digitalRead(SWC_BTN_PLAY_PIN) == LOW) {
            vTaskDelay(pdMS_TO_TICKS(SWC_DEBOUNCE_MS));
            if (digitalRead(SWC_BTN_PLAY_PIN) == LOW) {
                // Đảo trạng thái: Nếu đang phát thì gửi Pause (0x02), ngược lại gửi Play (0x01)
                uint8_t targetCmd = (m_currentPlaybackState == STATE_PLAYING) ? CMD_PAUSE : CMD_PLAY;
                if (m_actionCb) {
                    m_actionCb(targetCmd, SEEK_NONE, 0);
                }
                while (digitalRead(SWC_BTN_PLAY_PIN) == LOW) {
                    vTaskDelay(pdMS_TO_TICKS(20));
                }
            }
        }

        // 2. Phím Next Track
        if (digitalRead(SWC_BTN_NEXT_PIN) == LOW) {
            vTaskDelay(pdMS_TO_TICKS(SWC_DEBOUNCE_MS));
            if (digitalRead(SWC_BTN_NEXT_PIN) == LOW) {
                if (m_actionCb) {
                    m_actionCb(CMD_NEXT, SEEK_NONE, 0);
                }
                while (digitalRead(SWC_BTN_NEXT_PIN) == LOW) {
                    vTaskDelay(pdMS_TO_TICKS(20));
                }
            }
        }

        // 3. Phím Previous Track
        if (digitalRead(SWC_BTN_PREV_PIN) == LOW) {
            vTaskDelay(pdMS_TO_TICKS(SWC_DEBOUNCE_MS));
            if (digitalRead(SWC_BTN_PREV_PIN) == LOW) {
                if (m_actionCb) {
                    m_actionCb(CMD_PREV, SEEK_NONE, 0);
                }
                while (digitalRead(SWC_BTN_PREV_PIN) == LOW) {
                    vTaskDelay(pdMS_TO_TICKS(20));
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
