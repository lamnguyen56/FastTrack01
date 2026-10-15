/**
 * @file esp32_can_transmitter.ino
 * @brief ESP32 TWAI (Two-Wire Automotive Interface / CAN Bus) Simulator
 * @target ESP32 NodeMCU / ESP32-WROOM-32
 * @transceiver SN65HVD230 / VP230 (3.3V CAN Transceiver)
 *
 * Sơ đồ đấu nối phần cứng:
 *   ESP32 GPIO 5 (RX) ------> Transceiver CRX (RXD)
 *   ESP32 GPIO 4 (TX) ------> Transceiver CTX (TXD)
 *   ESP32 3V3         ------> Transceiver 3V3
 *   ESP32 GND         ------> Transceiver GND
 *
 *   Transceiver CANH  ------> Linux Board (MCP2515 CANH)  [Điện trở 120 Ohm giữa CANH-CANL]
 *   Transceiver CANL  ------> Linux Board (MCP2515 CANL)
 *   ESP32 GND         ------> Linux Board GND (Bắt buộc chung mass CAN)
 *
 * Định dạng CAN Message (Baudrate 500 kbps):
 *   - TX: ID 0x100 (Command to Linux Target)
 *         Byte 0: Command (1:Play, 2:Pause, 3:Next, 4:Prev, 5:FF, 6:REW)
 *         Byte 1: Alive Counter (0 - 15)
 *         Byte 2-6: Reserved (0x00)
 *         Byte 7: Checksum (Tổng XOR Byte 0..6)
 *
 *   - RX: ID 0x101 (Telemetry from Linux Target)
 *         Byte 0: PlaybackState (0:Stopped, 1:Playing, 2:Paused)
 *         Byte 1: TrackIndex
 *         Byte 2-3: CurrentPlayTime (giây, Big-Endian)
 *         Byte 4-5: TotalDuration (giây, Big-Endian)
 *         Byte 6: USB Status (0:None, 1:Mounted, 2:Error)
 *         Byte 7: Alive Counter / Checksum
 */

#include "driver/twai.h"

// Cấu hình chân GPIO cho ESP32 TWAI (CAN)
#define CAN_TX_PIN GPIO_NUM_4
#define CAN_RX_PIN GPIO_NUM_5

static uint8_t g_alive_counter = 0;

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 2000);

    Serial.println("\n=======================================================");
    Serial.println("  FASTTRACK / UBS AUTOMOTIVE - ESP32 CAN SIMULATOR    ");
    Serial.println("  Tốc độ: 500 kbps | Interface: TWAI (CAN 2.0B)       ");
    Serial.println("=======================================================");
    Serial.println("Phím tắt qua Serial Monitor:");
    Serial.println("  'p' : Gửi lệnh PLAY");
    Serial.println("  's' : Gửi lệnh PAUSE / STOP");
    Serial.println("  'n' : Gửi lệnh NEXT TRACK");
    Serial.println("  'b' : Gửi lệnh PREVIOUS TRACK");
    Serial.println("  'f' : Gửi lệnh FAST FORWARD (+10s)");
    Serial.println("  'r' : Gửi lệnh REWIND (-10s)");
    Serial.println("  'a' : Bật/Tắt chế độ tự động gửi lệnh ngẫu nhiên");
    Serial.println("=======================================================\n");

    // 1. Cấu hình phần cứng TWAI cho ESP32
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    twai_timing_config_t  t_config = TWAI_TIMING_CONFIG_500KBITS(); // Chuẩn 500 kbps theo can0
    twai_filter_config_t  f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

    // 2. Khởi tạo driver
    if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
        Serial.println("[OK] Cài đặt TWAI driver thành công!");
    } else {
        Serial.println("[ERROR] Cài đặt TWAI driver thất bại!");
        return;
    }

    // 3. Khởi động driver
    if (twai_start() == ESP_OK) {
        Serial.println("[OK] TWAI driver đang chạy ở 500 kbps. Sẵn sàng phát/nhận CAN!");
    } else {
        Serial.println("[ERROR] Khởi động TWAI driver thất bại!");
    }
}

// Hàm gửi gói tin CAN điều khiển ID 0x100
bool send_can_command(uint8_t cmd_code, const char* desc) {
    twai_message_t msg;
    msg.identifier = 0x100;
    msg.extd = 0; // Standard 11-bit CAN ID
    msg.rtr = 0;
    msg.data_length_code = 8;

    msg.data[0] = cmd_code;
    msg.data[1] = (g_alive_counter++) & 0x0F;
    msg.data[2] = 0x00;
    msg.data[3] = 0x00;
    msg.data[4] = 0x00;
    msg.data[5] = 0x00;
    msg.data[6] = 0x00;

    // Checksum = XOR byte 0 đến byte 6
    uint8_t cks = 0;
    for (int i = 0; i < 7; i++) {
        cks ^= msg.data[i];
    }
    msg.data[7] = cks;

    esp_err_t res = twai_transmit(&msg, pdMS_TO_TICKS(100));
    if (res == ESP_OK) {
        Serial.printf("[CAN TX] ID: 0x100 | Cmd: 0x%02X (%s) | Alive: %d | CKS: 0x%02X\n",
                      cmd_code, desc, msg.data[1], cks);
        return true;
    } else {
        Serial.printf("[CAN TX FAILED] Lỗi gửi CAN: 0x%X\n", res);
        return false;
    }
}

// Hàm nhận phản hồi Telemetry ID 0x101 từ Linux
void receive_can_status() {
    twai_message_t rx_msg;
    if (twai_receive(&rx_msg, 0) == ESP_OK) {
        if (rx_msg.identifier == 0x101 && rx_msg.data_length_code == 8) {
            uint8_t state = rx_msg.data[0];
            uint8_t track = rx_msg.data[1];
            uint16_t cur_sec = (rx_msg.data[2] << 8) | rx_msg.data[3];
            uint16_t tot_sec = (rx_msg.data[4] << 8) | rx_msg.data[5];
            uint8_t usb = rx_msg.data[6];

            const char* state_str = (state == 1) ? "PLAYING" : (state == 2 ? "PAUSED" : "STOPPED");
            Serial.printf("[CAN RX <- Target] Trạng thái: %s | Bài: #%d | %02d:%02d / %02d:%02d | USB: %s\n",
                          state_str, track + 1,
                          cur_sec / 60, cur_sec % 60,
                          tot_sec / 60, tot_sec % 60,
                          usb == 1 ? "MOUNTED (OK)" : "DISCONNECTED");
        }
    }
}

static bool g_auto_mode = false;
static unsigned long g_last_auto_time = 0;

void loop() {
    // 1. Kiểm tra nhận bản tin CAN trả về từ bo nhúng
    receive_can_status();

    // 2. Nhận phím điều khiển từ Serial
    if (Serial.available()) {
        char ch = Serial.read();
        switch (ch) {
            case 'p':
            case 'P':
                send_can_command(0x01, "PLAY");
                break;
            case 's':
            case 'S':
                send_can_command(0x02, "PAUSE / STOP");
                break;
            case 'n':
            case 'N':
                send_can_command(0x03, "NEXT TRACK");
                break;
            case 'b':
            case 'B':
                send_can_command(0x04, "PREV TRACK");
                break;
            case 'f':
            case 'F':
                send_can_command(0x05, "SEEK +10s (FF)");
                break;
            case 'r':
            case 'R':
                send_can_command(0x06, "SEEK -10s (REW)");
                break;
            case 'a':
            case 'A':
                g_auto_mode = !g_auto_mode;
                Serial.printf("[CHẾ ĐỘ TỰ ĐỘNG]: %s\n", g_auto_mode ? "BẬT (Mỗi 4 giây gửi 1 lệnh)" : "TẮT");
                break;
            default:
                break;
        }
    }

    // 3. Tự động gửi lệnh giả lập nếu bật auto_mode
    if (g_auto_mode && (millis() - g_last_auto_time > 4000)) {
        g_last_auto_time = millis();
        static int step = 0;
        step = (step + 1) % 4;
        if (step == 0) send_can_command(0x01, "AUTO PLAY");
        else if (step == 1) send_can_command(0x05, "AUTO FORWARD +10s");
        else if (step == 2) send_can_command(0x03, "AUTO NEXT TRACK");
        else if (step == 3) send_can_command(0x02, "AUTO PAUSE");
    }

    delay(20);
}
