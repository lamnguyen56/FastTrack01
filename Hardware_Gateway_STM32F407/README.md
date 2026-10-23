# BỘ MÔ PHỎNG NÚT ĐIỀU KHIỂN CAN STM32F407VGT6
## Dành Cho Dự Án Ô Tô Nhúng FastTrack01 (Điều Khiển USB Video Player)

> **Mục tiêu:** Thay thế bộ phát ESP32 (vốn phức tạp do phải duy trì Wi-Fi SoftAP, Web Server và Web Dashboard) bằng một nút điều khiển chuẩn Automotive độc lập trên vi điều khiển **STM32F407VGT6** (Cortex-M4, bxCAN phần cứng 500 kbps), truyền lệnh CAN trực tiếp bằng **nút nhấn vật lý (Button)** và hiển thị trạng thái qua **cụm đèn LED (LED Status)**.

---

## 1. So Sánh Kiến Trúc: ESP32 Gateway vs STM32 Gateway

| Tiêu chí | `Hardware_Gateway_ESP32` (Cũ) | `Hardware_Gateway_STM32F407` (Mới) |
| :--- | :--- | :--- |
| **Giao thức điều khiển** | Web Dashboard qua Wi-Fi / WebSockets | **Nút bấm vật lý (Button)** trực tiếp |
| **Độ trễ truyền tin** | 10 ~ 50 ms (Qua WebSocket $\to$ FreeRTOS $\to$ TWAI) | **< 1 ms** (Ngắt phần cứng bxCAN tức thời) |
| **Phản hồi trạng thái** | Giao diện Web HTML5/CSS3 trên điện thoại/PC | **Cụm 4 LED trạng thái trên bo mạch** |
| **Độ phức tạp phần mềm** | Cao (Wi-Fi AP, AsyncTCP, WebServer, LittleFS) | **Tinh gọn, chuẩn ô tô (Clean Embedded C)** |
| **Thời gian khởi động** | 2 ~ 4 giây (Khởi tạo Wi-Fi stack & Web Server) | **< 50 miligiây** (Chạy ngay khi cắm nguồn) |
| **Độ ổn định** | Phụ thuộc vào sóng Wi-Fi 2.4GHz | **Chuẩn công nghiệp Automotive 24/7** |

---

## 2. Cấu Trúc Thư Mục Tinh Gọn (Clean Directory Structure)

Thư mục đã được dọn sạch toàn bộ các file rác IDE, file tạm, build artifacts; chỉ giữ lại mã nguồn chuẩn C và tài liệu hướng dẫn:

```
Hardware_Gateway_STM32F407/
├── README.md                      # Tài liệu tổng quan dự án (File này)
├── GUIDE_SETUP_AND_WIRING.md      # 📘 HƯỚNG DẪN CHI TIẾT TỪ A-Z: Tạo project CubeIDE thủ công, đấu dây, test với Pi
│
├── docs/                          # Tài liệu kỹ thuật
│   └── pinout_diagram.md          # Sơ đồ khối nguyên lý và bảng chân chi tiết
│
├── include/                       # Các file Header C chuẩn (Clean Headers)
│   ├── button_manager.h           # Máy trạng thái FSM 5 trạng thái (Single/Double click, chống spam lần 3, 4)
│   ├── can_driver.h               # Giao diện điều khiển ngoại vi bxCAN (Init, Filter Bank, TX, RX)
│   ├── can_protocol.h             # Cấu trúc CAN frame ID 0x100, 0x101, Checksum và Rolling Counter
│   ├── config.h                   # Cấu hình GPIO (PB8/PB9, PE0..PE2), Bit Timing 500k, ngưỡng thời gian FSM
│   ├── led_indicator.h            # Giao diện điều khiển LED trạng thái (TX pulse, hold, error, play state)
│   ├── main.h                     # Header hệ thống
│   ├── stm32f4xx_hal_conf.h       # Cấu hình thư viện STM32 HAL
│   └── stm32f4xx_it.h             # Khai báo các vector ngắt ngoại vi
│
└── src/                           # Các file Source C chuẩn (Clean Source Code)
    ├── button_manager.c           # Cài đặt FSM quét 3 nút, lọc rung 50ms, chống spam phím
    ├── can_driver.c               # Cài đặt ngoại vi CAN1 ở tốc độ 500 kbps (Filter Bank 0, TX, RX IRQ)
    ├── led_indicator.c            # Điều khiển hiệu ứng chớp tắt non-blocking (single/double pulse)
    ├── main.c                     # Vòng lặp điều phối chính non-blocking
    └── stm32f4xx_it.c             # Trình phục vụ ngắt CAN1_RX0_IRQHandler, SysTick
```

---

## 3. Hoạt Động Của Nút Bấm & Cụm Đèn LED

### 3.1. Cụm 3 Nút Bấm Điều Khiển (Single & Double Click)
- **Nút 1 (`PE0` / hoặc `PA0` Onboard): PLAY / PAUSE**
  - **Ấn 1 lần:** Đảo trạng thái giữa `PLAY (0x01)` $\longleftrightarrow$ `PAUSE (0x02)`.
- **Nút 2 (`PE1`): NEXT / PREVIOUS**
  - **Ấn 1 lần:** Chuyển sang bài kế tiếp (`NEXT TRACK - 0x03`) $\to$ LED xanh lá chớp 1 nhịp.
  - **Ấn 2 lần (Double Click):** Quay lại bài trước (`PREVIOUS TRACK - 0x04`) $\to$ LED xanh lá chớp 2 nhịp nhanh.
  - 🛡️ **Khóa chống spam:** Tự động khóa và bỏ qua hoàn toàn các cú bấm dồn dập thứ 3, 4!
- **Nút 3 (`PE2`): TUA +10S / -10S**
  - **Ấn 1 lần:** Tua tiến 10 giây (`SEEK +10s` - `SeekCmd=0x05, Step=10`) $\to$ LED xanh lá chớp 1 nhịp.
  - **Ấn 2 lần (Double Click):** Tua lùi 10 giây (`SEEK -10s` - `SeekCmd=0x06, Step=10`) $\to$ LED xanh lá chớp 2 nhịp nhanh.
  - 🛡️ **Khóa chống spam:** Tự động khóa và bỏ qua hoàn toàn các cú bấm dồn dập thứ 3, 4!
- **Trải nghiệm người dùng:** Không cần phải nhấn giữ lâu, nhận diện ngay sau cú bấm thứ 2, có bộ lọc rung cơ học **50ms debounce** và cooldown **200ms**.

### 3.2. Cụm Đèn LED Báo Trạng Thái (LED Status)
- **LED Cam (LD3 - PD13):** Bật sáng ngay khi nút bấm đang được nhấn giữ (phản hồi xúc giác tức thì).
- **LED Xanh lá (LD4 - PD12):** Chớp sáng nhanh khi frame CAN ID `0x100` được truyền thành công lên đường bus.
- **LED Đỏ (LD5 - PD14):** Bật sáng cảnh báo nếu xảy ra lỗi đường truyền CAN (Bus-off, đứt cáp hoặc mất trở 120Ω).
- **LED Xanh dương (LD6 - PD15):** Đồng bộ trạng thái từ Raspberry Pi (bật sáng liên tục khi video đang phát - `PLAYING`).

---

## 4. Bắt Đầu Nhanh (Quick Start)

### Bước 1: Đấu Nối Phần Cứng
- **Module CAN VP1050 (TJA1050)** chạy nguồn **5V**:
  - Nối `VCC` $\to$ Chân **`5V`** trên STM32F407 (Header P1 Pin 5, 6).
  - Nối `GND` $\to$ Chân **`GND`** trên STM32F407.
  - Nối `TX` $\to$ Chân **`PB9`** (CAN1_TX).
  - Nối `RX` $\to$ Chân **`PB8`** (CAN1_RX - Chân 5V-Tolerant cắm trực tiếp an toàn).
- **Module MCP2515 trên Raspberry Pi:** Cắm vào chân Header 40-pin GPIO (SPI0).
- **Đường truyền CAN Bus:**
  - Nối `CAN_H` sang `CAN_H`, `CAN_L` sang `CAN_L`.
  - **CỰC KỲ QUAN TRỌNG:** Nối 1 dây **Common Ground** giữa `GND STM32` và `GND Raspberry Pi`.
  - Đóng jumper trở **120Ω** ở cả 2 đầu module.

### Bước 2: Tạo Project & Nạp Firmware
Do các phiên bản STM32CubeIDE có thể khác nhau và không hỗ trợ import trực tiếp từ file `.ioc` ngoại lai, quy trình chuẩn khuyến nghị là **tự tạo Project mới thủ công từ đầu trong STM32CubeIDE**:
1. Mở STM32CubeIDE $\to$ `File` $\to$ `New` $\to$ `STM32 Project` $\to$ Chọn `STM32F407VG` (hoặc `STM32F4DISCOVERY`).
2. Cấu hình nhanh SYS (Serial Wire), RCC (HSE Crystal), CAN1 (PB8/PB9, 500 kbps), GPIO (LED PD12-PD15, Nút PE0-PE2 Pull-Up), Clock (168MHz).
3. Nhấn `Ctrl + S` để sinh code, sau đó copy các file từ thư mục `include/` và `src/` vào project.
4. Bấm **Build** (`Ctrl + B`) và **Run** để nạp chương trình qua mạch nạp ST-Link.

👉 **Xem hướng dẫn chi tiết từng click chuột tại:** [GUIDE_SETUP_AND_WIRING.md](GUIDE_SETUP_AND_WIRING.md).

### Bước 3: Kiểm Thử Với Raspberry Pi Target
Trên Raspberry Pi, chạy terminal:
```bash
# Bật SocketCAN bitrate 500k
sudo ip link set can0 up type can bitrate 500000 restart-ms 100

# Lắng nghe dữ liệu
candump -tz can0
```
Bấm các nút trên STM32 và quan sát các gói tin CAN ID `0x100` xuất hiện tức thì!
