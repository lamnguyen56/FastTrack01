#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <functional>
#include "Config.h"
#include "CanProtocol.h"

// ============================================================================
// CLASS QUẢN LÝ WEB SERVER & WEBSOCKET EMBEDDED HMI
// ============================================================================

class WebServerManager {
public:
    typedef std::function<void(uint8_t cmd, uint8_t seek, uint8_t val)> CommandCallback;

    static WebServerManager& getInstance();

    // Khởi tạo Wi-Fi SoftAP, Mount LittleFS và WebServer/WebSocket
    bool begin();

    // Vòng lặp định kỳ dọn dẹp client ngắt kết nối
    void update();

    // Phát tán gói tin Telemetry tới tất cả Web clients đang kết nối
    void broadcastTelemetry(const CanStatusPacket &stat);

    // Phát tán log tin nhắn tới cửa sổ Live CAN Sniffer
    void broadcastLog(const char* logMsg, bool isTx);

    // Đăng ký callback khi nhận lệnh từ Web UI
    void setCommandCallback(CommandCallback cb);

private:
    WebServerManager();
    ~WebServerManager();
    WebServerManager(const WebServerManager&) = delete;
    WebServerManager& operator=(const WebServerManager&) = delete;

    void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                         AwsEventType type, void *arg, uint8_t *data, size_t len);
    void handleClientMessage(void *arg, uint8_t *data, size_t len);

    AsyncWebServer m_server;
    AsyncWebSocket m_ws;
    bool m_littleFsMounted;
    CommandCallback m_cmdCb;
};
