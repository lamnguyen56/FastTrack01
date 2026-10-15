#pragma once

#include <Arduino.h>
#include <functional>
#include "Config.h"
#include "CanProtocol.h"

// ============================================================================
// CLASS QUẢN LÝ PHÍM CỨNG VÔ LĂNG VẬT LÝ (Steering Wheel Controls - SWC)
// ============================================================================

class SwcButtonManager {
public:
    typedef std::function<void(uint8_t cmd, uint8_t seek, uint8_t val)> ButtonActionCallback;

    static SwcButtonManager& getInstance();

    // Khởi tạo chân GPIO và Task FreeRTOS quét phím
    void begin();

    // Đăng ký callback khi có nút bấm được kích hoạt
    void setActionCallback(ButtonActionCallback cb);

    // Cập nhật trạng thái phát hiện tại để nút Play/Pause biết nên gửi lệnh 1 hay 2
    void updatePlaybackState(uint8_t state) { m_currentPlaybackState = state; }

private:
    SwcButtonManager();
    ~SwcButtonManager();
    SwcButtonManager(const SwcButtonManager&) = delete;
    SwcButtonManager& operator=(const SwcButtonManager&) = delete;

    static void buttonTaskTrampoline(void *arg);
    void buttonTaskLoop();

    ButtonActionCallback m_actionCb;
    uint8_t m_currentPlaybackState;
};
