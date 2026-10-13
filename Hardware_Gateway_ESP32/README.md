# BỘ GIẢ LẬP ESP32 AUTOMOTIVE CAN GATEWAY & WEB DASHBOARD
## Nút Điều Khiển Mạng CAN Ô Tô Cho Dự Án FastTrack01

> **Mục tiêu:** Thay thế công cụ Vector CANoe đắt đỏ và cồng kềnh bằng một bo mạch nhúng vi điều khiển **ESP32** độc lập, tích hợp ngoại vi phần cứng **TWAI (CAN 500 kbps)** và **Embedded Web Dashboard thời gian thực** (Wi-Fi SoftAP / WebSockets) để điều khiển toàn diện trình phát video trên Raspberry Pi (Linux Target).

---

## 1. Cấu Trúc Thư Mục Phân Tách Rõ Ràng (Clean Modular Architecture)

Bộ mã nguồn được phân tách hoàn toàn giữa **Tầng Giao Diện Người Dùng (Frontend Web HMI)** và **Tầng Firmware Nhúng (Embedded C++ Backend)**:

```
Hardware_Gateway_ESP32/
├── README.md                          # Tài liệu hướng dẫn toàn diện từ A-Z (File này)
│
├── docs/                              # Tài liệu kỹ thuật phần cứng
│   └── pinout_schematic.md            # Sơ đồ nguyên lý đấu dây chi tiết
│
├── web_dashboard/                     # 🌐 BỘ GIAO DIỆN WEB HMI ĐỘC LẬP (Mở trực tiếp trên PC)
│   ├── index.html                     # Giao diện HTML5 Automotive Cockpit
│   ├── style.css                      # Styling CSS3 Dark Glassmorphism, Responsive Mobile/PC
│   └── app.js                         # Logic WebSocket, Telemetry Gauge, Live Sniffer & Mock Mode
│
└── firmware/                          # ⚡ BỘ MÃ NGUỒN FIRMWARE C++ MODULAR CHO ESP32
    ├── platformio.ini                 # Cấu hình PlatformIO (board esp32dev, littlefs, libs)
    ├── data/                          # Tài nguyên web nạp vào bộ nhớ flash LittleFS của ESP32
    │   ├── index.html
    │   ├── style.css
    │   └── app.js
    ├── include/                       # Các file Header C++ (Giao diện module)
    │   ├── Config.h                   # Cấu hình chân GPIO, Bitrate 500kbps, Wi-Fi SSID
    │   ├── CanProtocol.h              # Struct Frame 0x100, 0x101, thuật toán CRC và Rolling Counter
    │   ├── TwaiManager.h              # Class quản lý ngoại vi phần cứng CAN TWAI
    │   ├── WebServerManager.h         # Class quản lý Wi-Fi AP, LittleFS và AsyncWebSocket
    │   ├── SwcButtonManager.h         # Class quản lý nút bấm vật lý vô lăng (SWC)
    │   └── WebAssets.h                # Giao diện dự phòng nhúng sẵn trong Flash (Fallback an toàn)
    └── src/                           # Các file Source C++ (Cài đặt module)
        ├── TwaiManager.cpp            # Xử lý ngắt phần cứng TWAI RX/TX và đa nhiệm FreeRTOS
        ├── WebServerManager.cpp       # Xử lý Web Server, WebSocket route và broadcast JSON
        ├── SwcButtonManager.cpp       # Quét nút bấm vật lý và thuật toán chống rung phím (debounce)
        └── main.cpp                   # Entry point tinh gọn, điều phối callback giữa các module
```

---

## 2. Hướng Dẫn Thử Nghiệm Web Dashboard Trên Máy Tính (Mock Mode)

Bạn **không cần cắm ESP32** vẫn có thể mở và tùy biến giao diện:
1. Mở thư mục [web_dashboard/](file:///d:/04_PROJECT/FastTrack01/Hardware_Gateway_ESP32/web_dashboard/).
2. Nhấp đúp chuột vào file [index.html](file:///d:/04_PROJECT/FastTrack01/Hardware_Gateway_ESP32/web_dashboard/index.html) để mở trong Google Chrome, Microsoft Edge hoặc Safari.
3. Trang web tự động nhận diện chạy cục bộ từ máy tính và kích hoạt **Mock Simulation Mode**:
   - Bộ đếm thời gian video tự chạy tiến trình.
   - Bấm nút `▶ PLAY / ⏸ PAUSE`, `+10s`, `-10s`, `NEXT`, `PREV` sẽ thấy frame mô phỏng xuất hiện trong cửa sổ *Live CAN Bus Traffic Sniffer*.
   - Cho phép các bạn làm giao diện tinh chỉnh màu sắc, CSS, hiệu ứng mà không cần nạp lại chip ESP32!

---

## 3. Hướng Dẫn Biên Dịch & Nạp Code Lên ESP32

### Phương Án 1: Sử Dụng VS Code + PlatformIO (Khuyến Nghị Tuyệt Đối)

1. **Chuẩn bị:** Cài đặt Extension **PlatformIO IDE** trong Visual Studio Code.
2. **Mở dự án:**
   - Trong VS Code, chọn `File` $\to$ `Open Folder...` $\to$ chọn thư mục `Hardware_Gateway_ESP32/firmware`.
3. **Cắm phần cứng:**
   - Cắm cáp USB nối bo mạch ESP32 vào máy tính.
4. **Biên dịch và nạp Firmware C++:**
   - Mở Terminal trong VS Code và chạy lệnh:
     ```bash
     pio run --target upload
     ```
5. **Nạp giao diện Web vào LittleFS (Tùy chọn):**
   - Chạy lệnh sau để tải toàn bộ thư mục `data/` vào bộ nhớ flash LittleFS:
     ```bash
     pio run --target uploadfs
     ```
   *(Lưu ý: Nếu bạn quên chạy lệnh này, hệ thống sẽ tự động dùng giao diện dự phòng tích hợp sẵn trong `WebAssets.h`, đảm bảo web vẫn hoạt động bình thường!)*
6. **Xem log Serial Monitor:**
   ```bash
   pio device monitor
   ```

---

### Phương Án 2: Sử Dụng Arduino IDE 2.x

1. **Cài đặt Board:** Vào `Tools` $\to$ `Board Manager`, cài gói `esp32 by Espressif Systems`.
2. **Cài đặt Thư viện:**
   - Vào `Library Manager`, cài đặt thư viện `ESPAsyncWebServer` và `AsyncTCP`.
3. **Mở mã nguồn:**
   - Mở file `firmware/src/main.cpp`.
4. **Cấu hình:** Chọn Board `ESP32 Dev Module`, chọn đúng cổng COM.
5. **Nạp Code:** Bấm nút **Upload**.

---

## 4. Hướng Dẫn Đấu Nối Phần Cứng

Xem tài liệu chi tiết có sơ đồ hình vẽ tại [docs/pinout_schematic.md](file:///d:/04_PROJECT/FastTrack01/Hardware_Gateway_ESP32/docs/pinout_schematic.md).

### Tóm Tắt Đấu Nối Giữa ESP32 Và Module Transceiver SN65HVD230 (3.3V):
- `ESP32 3V3` $\to$ `SN65HVD230 3V3 (VCC)`
- `ESP32 GND` $\to$ `SN65HVD230 GND`
- `ESP32 GPIO 5` (TX) $\to$ `SN65HVD230 CTX`
- `ESP32 GPIO 4` (RX) $\to$ `SN65HVD230 CRX`

### Đấu Nối CAN Bus Sang Raspberry Pi Target (Module MCP2515):
- `SN65HVD230 CAN_H` $\to$ Cáp xoắn đôi $\to$ `MCP2515 CAN_H` (Raspberry Pi)
- `SN65HVD230 CAN_L` $\to$ Cáp xoắn đôi $\to$ `MCP2515 CAN_L` (Raspberry Pi)
- **CỰC KỲ QUAN TRỌNG:** Nối 1 dây từ `GND của ESP32` sang chân `GND của Raspberry Pi` (Pin 6/9/14) để cân bằng điện thế đất chung (Common Ground).
- Kiểm tra jumper trở **120Ω** đã được đóng ở cả 2 đầu module (SN65HVD230 và MCP2515).

---

## 5. Hướng Dẫn Sử Dụng & Vận Hành Thực Tế

1. **Khởi động:**
   - Cấp nguồn cho Raspberry Pi (Linux Target) và ESP32.
   - Script trên Pi tự động kích hoạt `can0` ở 500 kbps (`setup_can.sh`).
2. **Kết nối Wi-Fi:**
   - Dùng Điện thoại thông minh, Máy tính bảng hoặc Laptop quét Wi-Fi.
   - Kết nối vào mạng Wi-Fi:
     - **Tên Wi-Fi (SSID):** `ESP32-CAN-GATEWAY`
     - **Mật khẩu (Password):** `12345678`
3. **Mở Web Dashboard:**
   - Mở trình duyệt bất kỳ (Chrome/Safari/Edge), nhập địa chỉ:
     ```
     http://192.168.4.1
     ```
   - Biểu tượng trạng thái góc phải sẽ chuyển sang màu xanh: `WS: CONNECTED`.
4. **Thực hiện điều khiển:**
   - Cắm USB chứa video vào Raspberry Pi $\to$ Đèn USB trên Web Dashboard đổi sang `READY (Mounted)`.
   - Bấm nút `▶ PLAY` trên Web Dashboard $\to$ Video trên màn hình QML bắt đầu phát; đồng thời thanh thời gian trên Web Dashboard chạy theo thời gian thực!
   - Bấm `+10s` / `-10s` / `NEXT` / `PREV` $\to$ Video nhảy track và thời lượng đồng bộ tức thì.
   - Bấm nút **🚀 CHẠY KỊCH BẢN KIỂM THỬ TỰ ĐỘNG (SMOKE TEST)** để tự động thực hiện chuỗi test mô phỏng kịch bản kiểm thử xe hơi chuyên nghiệp!
