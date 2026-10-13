# HƯỚNG DẪN ĐẤU NỐI PHẦN CỨNG & NẠP FIRMWARE ESP32 CAN GATEWAY

## 1. Danh Mục Linh Kiện
- **ESP32 DevKit V1** (ESP-WROOM-32)
- **Module CAN Transceiver SN65HVD230** (hoặc VP230) chạy nguồn 3.3V
- **Module MCP2515 SPI CAN** (kết nối trên Raspberry Pi Target)
- **Dây nối cắm (Jumper wires) & Cặp dây xoắn đôi (Twisted Pair)**

---

## 2. Bảng Sơ Đồ Đấu Dây (Pinout Table)

### 2.1 ESP32 <-> SN65HVD230 (3.3V CAN Transceiver)
| Chân ESP32 | Chân SN65HVD230 | Chức năng |
| :--- | :--- | :--- |
| `3V3` | `3V3` (VCC) | Nguồn cấp 3.3V |
| `GND` | `GND` | Mass đất |
| `GPIO 5` | `CTX` (TXD) | TWAI Transmit Data |
| `GPIO 4` | `CRX` (RXD) | TWAI Receive Data |

### 2.2 SN65HVD230 <-> MCP2515 (Raspberry Pi Target)
| Chân SN65HVD230 | Chân MCP2515 | Ghi chú |
| :--- | :--- | :--- |
| `CAN_H` | `CAN_H` | Cặp dây xoắn đôi vi sai |
| `CAN_L` | `CAN_L` | Cặp dây xoắn đôi vi sai |
| `GND` (ESP32) | `GND` (Raspberry Pi) | **Bắt buộc nối Common Ground** |

### 2.3 Phím Cứng Vô Lăng (Optional Physical SWC)
| Chân ESP32 | Nút bấm | Hành vi |
| :--- | :--- | :--- |
| `GPIO 18` | Nút 1 (Chân còn lại nối GND) | Play / Pause |
| `GPIO 19` | Nút 2 (Chân còn lại nối GND) | Next Track |
| `GPIO 23` | Nút 3 (Chân còn lại nối GND) | Previous Track |

---

## 3. Lưu Ý Về Trở Đầu Cuối (Termination Resistor)
- Kiểm tra jumper trở 120Ω trên module SN65HVD230 và module MCP2515.
- Cả hai đầu bus phải bật trở 120Ω để tổng trở đường truyền đạt ~60Ω.

---

## 4. Hướng Dẫn Nạp Firmware Bằng PlatformIO (VSCode)
1. Mở thư mục `Hardware_Gateway_ESP32/firmware` trong VSCode.
2. Cắm cáp USB Type-C / Micro-USB từ ESP32 vào máy tính.
3. Chạy lệnh:
   ```bash
   pio run --target upload
   pio device monitor
   ```
4. Khi khởi động xong, ESP32 sẽ phát Wi-Fi:
   - **SSID:** `ESP32-CAN-GATEWAY`
   - **Mật khẩu:** `12345678`
   - **Địa chỉ truy cập Web Dashboard:** `http://192.168.4.1`
