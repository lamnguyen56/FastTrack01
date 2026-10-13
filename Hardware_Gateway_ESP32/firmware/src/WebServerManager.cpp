#include "WebServerManager.h"
#include "WebAssets.h"
#include <LittleFS.h>

WebServerManager& WebServerManager::getInstance() {
    static WebServerManager instance;
    return instance;
}

WebServerManager::WebServerManager()
    : m_server(HTTP_SERVER_PORT)
    , m_ws("/ws")
    , m_littleFsMounted(false)
    , m_cmdCb(nullptr)
{
}

WebServerManager::~WebServerManager() {
}

bool WebServerManager::begin() {
    // 1. Khởi tạo Wi-Fi SoftAP
    WiFi.mode(WIFI_AP);
    WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);

    Serial.println("\n--------------------------------------------------");
    Serial.printf("[WIFI] Access Point kích hoạt: %s\n", WIFI_AP_SSID);
    Serial.printf("[WIFI] Mật khẩu: %s\n", WIFI_AP_PASS);
    Serial.print("[WIFI] Địa chỉ Web Dashboard: http://");
    Serial.println(WiFi.softAPIP());
    Serial.println("--------------------------------------------------");

    // 2. Thử nghiệm mount hệ thống file LittleFS
    m_littleFsMounted = LittleFS.begin(true);
    bool hasLittleFsAssets = false;

    if (m_littleFsMounted) {
        if (LittleFS.exists("/index.html")) {
            hasLittleFsAssets = true;
            Serial.println("[FS] Phát hiện tài nguyên web trong LittleFS (/index.html). Sử dụng file rời.");
            m_server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html").setCacheControl("max-age=3600");
        } else {
            Serial.println("[FS WARN] LittleFS mount thành công nhưng chưa có file. Chuyển sang WebAssets fallback.");
        }
    } else {
        Serial.println("[FS WARN] Không thể mount LittleFS. Chuyển sang WebAssets fallback.");
    }

    // Nếu không có file trong LittleFS, nạp trang giao diện dự phòng nhúng trực tiếp trong Flash
    if (!hasLittleFsAssets) {
        m_server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send_P(200, "text/html", FALLBACK_INDEX_HTML);
        });
    }

    // 3. Khởi tạo WebSocket Server
    m_ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, 
                        AwsEventType type, void *arg, uint8_t *data, size_t len) {
        this->onWebSocketEvent(server, client, type, arg, data, len);
    });

    m_server.addHandler(&m_ws);
    m_server.begin();
    Serial.println("[HTTP] Web Server đã khởi động trên cổng 80.");

    return true;
}

void WebServerManager::update() {
    m_ws.cleanupClients();
}

void WebServerManager::broadcastTelemetry(const CanStatusPacket &stat) {
    if (m_ws.count() == 0) return;

    char jsonBuf[180];
    snprintf(jsonBuf, sizeof(jsonBuf),
        "{\"type\":\"telemetry\",\"state\":%d,\"track\":%d,\"curTime\":%d,"
        "\"totalTime\":%d,\"usb\":%d,\"alive\":%d}",
        stat.playbackState,
        stat.trackIndex,
        stat.curTimeSec,
        stat.totalDurationSec,
        stat.usbHealth,
        stat.aliveCounter
    );

    m_ws.textAll(jsonBuf);
}

void WebServerManager::broadcastLog(const char* logMsg, bool isTx) {
    if (m_ws.count() == 0) return;

    char jsonBuf[200];
    snprintf(jsonBuf, sizeof(jsonBuf),
             "{\"type\":\"log\",\"msg\":\"%s\",\"isTx\":%s}",
             logMsg, isTx ? "true" : "false");

    m_ws.textAll(jsonBuf);
}

void WebServerManager::setCommandCallback(CommandCallback cb) {
    m_cmdCb = cb;
}

void WebServerManager::onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, 
                                        AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[WS] Client #%u kết nối từ IP: %s\n", client->id(), client->remoteIP().toString().c_str());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("[WS] Client #%u đã ngắt kết nối.\n", client->id());
    } else if (type == WS_EVT_DATA) {
        this->handleClientMessage(arg, data, len);
    }
}

void WebServerManager::handleClientMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = '\0';
        String msg = (char*)data;

        // Parse JSON dạng: {"action":"cmd","cmd":1,"seek":0,"val":0}
        int cmdIdx = msg.indexOf("\"cmd\":");
        int seekIdx = msg.indexOf("\"seek\":");
        int valIdx = msg.indexOf("\"val\":");

        if (cmdIdx > 0 && seekIdx > 0 && valIdx > 0) {
            uint8_t cmd  = msg.substring(cmdIdx + 6, msg.indexOf(",", cmdIdx)).toInt();
            uint8_t seek = msg.substring(seekIdx + 7, msg.indexOf(",", seekIdx)).toInt();
            uint8_t val  = msg.substring(valIdx + 6, msg.indexOf("}", valIdx)).toInt();

            if (m_cmdCb) {
                m_cmdCb(cmd, seek, val);
            }
        }
    }
}
