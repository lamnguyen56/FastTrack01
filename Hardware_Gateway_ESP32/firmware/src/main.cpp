/**
 * @file main.cpp
 * @brief Entry point for ESP32 Automotive CAN Gateway Simulator
 * @project FastTrack01 - USB Video Player Controlled via CAN
 * @platform ESP32 DevKit V1 (ESP-WROOM-32)
 */

#include <Arduino.h>
#include "Config.h"
#include "CanProtocol.h"
#include "TwaiManager.h"
#include "WebServerManager.h"
#include "SwcButtonManager.h"

void setup() {
    Serial.begin(SERIAL_MONITOR_BAUD);
    delay(500);

    Serial.println("\n========================================================");
    Serial.println("   ESP32 AUTOMOTIVE CAN GATEWAY SIMULATOR STARTING...   ");
    Serial.println("========================================================");

    // 1. Khởi tạo ngoại vi CAN TWAI (500 kbps, Task lắng nghe ngắt RX Core 0)
    TwaiManager &can = TwaiManager::getInstance();
    if (!can.begin()) {
        Serial.println("[SYSTEM FATAL] Khởi tạo CAN TWAI thất bại!");
    }

    // 2. Khởi tạo Wi-Fi SoftAP, LittleFS và Web Server / WebSocket
    WebServerManager &web = WebServerManager::getInstance();
    if (!web.begin()) {
        Serial.println("[SYSTEM FATAL] Khởi tạo Web Server thất bại!");
    }

    // 3. Khởi tạo cụm nút bấm cơ học vô lăng (SWC Pins, Task quét phím Core 1)
    SwcButtonManager &swc = SwcButtonManager::getInstance();
    swc.begin();

    // 4. Liên kết Callback giữa các phân hệ độc lập:

    // (A) Khi CAN nhận telemetry ID 0x101 từ Target -> Đẩy lên Web Dashboard & cập nhật trạng thái SWC
    can.setTelemetryCallback([&web, &swc](const CanStatusPacket &stat) {
        web.broadcastTelemetry(stat);
        swc.updatePlaybackState(stat.playbackState);
    });

    // (B) Khi CAN phát sinh log TX / RX -> Đẩy lên Live CAN Sniffer trên Web UI
    can.setLogCallback([&web](const char* logMsg, bool isTx) {
        web.broadcastLog(logMsg, isTx);
    });

    // (C) Khi người dùng bấm nút trên Web Dashboard -> Gửi frame CAN ID 0x100
    web.setCommandCallback([&can](uint8_t cmd, uint8_t seek, uint8_t val) {
        can.sendCommand(cmd, seek, val);
    });

    // (D) Khi người dùng bấm phím cứng vô lăng (SWC Buttons) -> Gửi frame CAN ID 0x100
    swc.setActionCallback([&can](uint8_t cmd, uint8_t seek, uint8_t val) {
        can.sendCommand(cmd, seek, val);
    });

    Serial.println("[SYSTEM] Hệ thống Gateway đã kết nối thông suốt và sẵn sàng hoạt động!");
}

void loop() {
    // Định kỳ dọn dẹp kết nối WebSocket của các client đã đóng
    WebServerManager::getInstance().update();
    vTaskDelay(pdMS_TO_TICKS(100));
}
