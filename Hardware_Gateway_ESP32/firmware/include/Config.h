#pragma once

#include <Arduino.h>
#include <driver/gpio.h>

// ============================================================================
// CẤU HÌNH PHẦN CỨNG & THÔNG SỐ TRUYỀN THÔNG (FASTTRACK-01 CAN GATEWAY)
// ============================================================================

// --- CHÂN NGOẠI VI CAN TWAI (Two-Wire Automotive Interface) ---
#define CAN_TX_PIN              GPIO_NUM_5    // Nối CTX của transceiver SN65HVD230
#define CAN_RX_PIN              GPIO_NUM_4    // Nối CRX của transceiver SN65HVD230

// --- CHÂN NÚT BẤM VẬT LÝ VÔ LĂNG (Steering Wheel Controls - SWC) ---
#define SWC_BTN_PLAY_PIN        18            // Nút Play / Pause (Active LOW, Pull-up)
#define SWC_BTN_NEXT_PIN        19            // Nút Next Track (Active LOW, Pull-up)
#define SWC_BTN_PREV_PIN        23            // Nút Previous Track (Active LOW, Pull-up)
#define SWC_DEBOUNCE_MS         50            // Thời gian chống rung phím (ms)

// --- THÔNG SỐ CAN BUS ĐẶC TẢ THEO DBC ---
#define CAN_BAUDRATE_KBPS       500           // Bitrate: 500 kbps chuẩn Automotive
#define CAN_ID_COMMAND          0x100         // Frame lệnh điều khiển (ESP32 -> Target)
#define CAN_ID_STATUS           0x101         // Frame phản hồi trạng thái (Target -> ESP32)
#define CAN_DLC_STANDARD        8             // Data Length Code: 8 bytes

// --- CẤU HÌNH MẠNG WI-FI ACCESS POINT & WEB SERVER ---
#define WIFI_AP_SSID            "ESP32-CAN-GATEWAY"
#define WIFI_AP_PASS            "12345678"
#define HTTP_SERVER_PORT        80
#define SERIAL_MONITOR_BAUD     115200
