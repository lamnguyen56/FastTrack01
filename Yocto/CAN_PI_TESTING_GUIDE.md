# CẨM NANG CẤU HÌNH & KIỂM THỬ MẠNG CAN TRÊN RASPBERRY PI TARGET (YOCTO)
## Dự án FastTrack01 - Giao tiếp SocketCAN (can0) với STM32F407 Gateway

---

## 1. TỔNG QUAN KIẾN TRÚC CAN TRÊN RASPBERRY PI

Hệ điều hành Yocto Linux trên Raspberry Pi 3B/4B quản lý giao tiếp CAN theo kiến trúc nhiều lớp (Multi-layer Architecture):

```
+-----------------------------------------------------------------------------------+
|                        KIẾN TRÚC PHẦN MỀM CAN TRÊN RASPBERRY PI                   |
+-----------------------------------------------------------------------------------+
|                                                                                   |
|  [Qt HMI Application] (usb-video-player)                                         |
|    - Điều khiển phát video, nhận tín hiệu Play/Pause/Next/Prev/Seek               |
|    - Gửi Telemetry trạng thái phát (Play state, Time, Track)                      |
|           ^                                                                       |
|           |  Unix Domain Socket (/tmp/can_ipc.sock & /tmp/qt_app_ipc.sock)        |
|           v                                                                       |
|  [Platform Daemon] (can-daemon.service)                                           |
|    - Đóng gói / Giải mã khung tin CAN theo DBC (ID 0x100 Lệnh, ID 0x101 Status)   |
|    - Tự động chạy /usr/bin/setup_can.sh trước khi khởi động                       |
|           ^                                                                       |
|           |  Linux SocketCAN API (RAW Socket CAN, bitrate 500 kbps)               |
|           v                                                                       |
|  [Linux Kernel Subsystem] (SocketCAN 'can0')                                      |
|    - Driver: CONFIG_CAN_MCP251X=y (built-in kernel)                               |
|    - Subsystem: CONFIG_CAN=y, CONFIG_CAN_RAW=y                                    |
|           ^                                                                       |
|           |  Giao tiếp SPI0 (SPI BCM2835) + Ngắt phần cứng GPIO25                 |
|           v                                                                       |
|  [Phần Cứng Module MCP2515]                                                      |
|    - Chip MCP2515 CAN Controller + TJA1050 / VP230 Transceiver                   |
|    - Thạch anh (Oscillator): 8 MHz (8000000 Hz) hoặc 16 MHz                       |
|           |                                                                       |
|           +=========== [ Cáp xoắn đôi CAN_H / CAN_L + Trở 120Ω ] ===============> |
|                        [ Nối sang Module VP1050 của STM32F407 ]                   |
+-----------------------------------------------------------------------------------+
```

---

## 2. CẤU HÌNH CAN TRONG DỰ ÁN YOCTO (ĐÃ ĐƯỢC TÍCH HỢP)

Dự án Yocto đã có đầy đủ cấu hình driver và daemon:

1. **Kernel Driver Built-in:**
   File [platform.cfg](meta-usb-video-player/recipes-kernel/linux/files/platform.cfg) đã kích hoạt sẵn:
   ```ini
   CONFIG_SPI=y
   CONFIG_SPI_BCM2835=y
   CONFIG_CAN=y
   CONFIG_CAN_RAW=y
   CONFIG_CAN_DEV=y
   CONFIG_CAN_CALC_BITTIMING=y
   CONFIG_CAN_MCP251X=y
   ```

2. **Kích hoạt Device Tree Overlay cho Module MCP2515:**
   File [rpi-config_git.bbappend](meta-usb-video-player/recipes-bsp/bootfiles/rpi-config_git.bbappend) đã cấu hình tần số thạch anh **8 MHz** mặc định:
   ```bitbake
   ENABLE_SPI_BUS:raspberrypi3-64 = "1"
   CAN0_INTERRUPT_PIN:raspberrypi3-64 = "25"
   CAN_OSCILLATOR:raspberrypi3-64 = "8000000"
   ENABLE_CAN:raspberrypi3-64 = "${@'1' if d.getVar('CAN_OSCILLATOR').strip() else '0'}"
   ```
   *(Khi build image, meta-raspberrypi sẽ tự động tạo dòng `dtoverlay=mcp2515-can0,oscillator=8000000,interrupt=25` vào `/boot/config.txt`).*

3. **Tự Động Cấu Hình Bitrate 500 kbps:**
   Script [setup_can.sh](meta-usb-video-player/recipes-platform/can-daemon/files/setup_can.sh) được kích hoạt bởi [can-daemon.service](meta-usb-video-player/recipes-platform/can-daemon/files/can-daemon.service):
   - Đợi giao diện `can0` sẵn sàng (tối đa 30s).
   - Thiết lập tốc độ `500000 bit/s` (500 kbps), `restart-ms 100`, `txqueuelen 1000`.
   - Bật giao diện `can0` lên (`ip link set dev can0 up`).

---

## 3. CÁCH CẤU HÌNH NHANH TRÊN RASPBERRY PI ĐANG CHẠY (KHÔNG CẦN BUILD LẠI YOCTO)

Nếu bạn **đã có sẵn thẻ nhớ SD** đang chạy trên Raspberry Pi và muốn kiểm tra ngay:

### Bước 1: Kiểm tra & cập nhật file `config.txt`
Trên Raspberry Pi (chạy Yocto), bạn có thể dùng lệnh `sed` để sửa nhanh (hoặc dùng trình soạn thảo `vi /boot/config.txt` vì Yocto minimal không cài sẵn `nano`):
```bash
# Tự động cập nhật hoặc thêm cấu hình chuẩn cho MCP2515:
sed -i 's/dtoverlay=mcp2515-can0.*/dtoverlay=mcp2515-can0,oscillator=8000000,interrupt=25,spimaxfrequency=1000000/' /boot/config.txt
```

Đảm bảo file `/boot/config.txt` có các dòng sau:
```ini
dtparam=spi=on
dtoverlay=mcp2515-can0,oscillator=8000000,interrupt=25,spimaxfrequency=1000000
```
> [!IMPORTANT]
> - **Tần số thạch anh (`oscillator`):** Nhìn vào con thạch anh kim loại màu bạc trên module MCP2515:
>   - Nếu ghi **`8.000`** $\implies$ để `oscillator=8000000`.
>   - Nếu ghi **`16.000`** $\implies$ sửa thành `oscillator=16000000`.
> - **Hạ xung SPI (`spimaxfrequency=1000000`):** Khi dùng dây cắm jumper dài, xung 10MHz mặc định rất dễ bị méo tín hiệu gây lỗi `Probe failed, err=110` (*didn't enter in conf mode*). Bắt buộc hạ xuống 1MHz (1000000) để đảm bảo giao tiếp SPI ổn định tuyệt đối!

### Bước 2: Khởi động lại Raspberry Pi
```bash
reboot
```

---

## 4. QUY TRÌNH KIỂM THỬ & DEBUG TỪNG BƯỚC (STEP-BY-STEP DEBUG GUIDE)

Sau khi Pi khởi động lại, thực hiện theo thứ tự 5 bước dưới đây:

### Bước 1: Kiểm tra Kernel đã nhận diện chip MCP2515 chưa
Chạy lệnh xem log kernel:
```bash
dmesg | grep -Ei 'mcp251|spi|can'
```
**Kết quả mong đợi:**
```text
[    4.120450] mcp251x spi0.0 can0: MCP2515 successfully initialized.
```
*(Nếu thấy dòng `successfully initialized`, phần cứng SPI và chip MCP2515 đã kết nối hoàn hảo!)*

---

### Bước 2: Kiểm tra trạng thái giao diện mạng `can0`
Chạy lệnh:
```bash
ip -details link show can0
```
**Kết quả mong đợi:**
```text
3: can0: <NOARP,UP,LOWER_UP,ECHO> mtu 16 qdisc pfifo_fast state UP mode DEFAULT group default qlen 1000
    link/can  promiscuity 0 minmtu 0 maxmtu 0
    can state ERROR-ACTIVE (berr-counter tx 0 rx 0) restart-ms 100
          bitrate 500000 sample-point 0.875
          tq 125 prop-seg 6 phase-seg1 7 phase-seg2 2 sjw 1
```
> [!NOTE]
> - `state ERROR-ACTIVE`: Trạng thái lý tưởng, bus CAN hoàn toàn bình thường, không có lỗi.
> - `bitrate 500000`: Đúng chuẩn 500 kbps, khớp 100% với STM32F407.

---

### Bước 3: Lắng nghe frame CAN từ STM32 bằng `candump`
Chạy công cụ nghe bus CAN trên terminal Pi:
```bash
candump -tz can0
```
Bây giờ hãy nhấn các nút trên bo mạch STM32:
1. **Ấn Nút 1 (PE4 hoặc PA0 Onboard - PLAY / PAUSE):**
   ```text
   (000.000000) can0 100 [8] 01 00 00 01 02 00 00 00   <-- Lệnh PLAY (0x01)
   (001.420100) can0 100 [8] 02 00 00 02 04 00 00 00   <-- Lệnh PAUSE (0x02)
   ```
2. **Ấn Nút 2 (PE5 - NEXT / PREV):**
   - Ấn 1 lần:
     ```text
     (003.110200) can0 100 [8] 03 00 00 03 06 00 00 00   <-- Lệnh NEXT (0x03)
     ```
   - Ấn 2 lần nhanh (Double Click):
     ```text
     (004.850400) can0 100 [8] 04 00 00 04 08 00 00 00   <-- Lệnh PREV (0x04)
     ```
3. **Ấn Nút 3 (PE6 - TUA +10S / -10S):**
   - Ấn 1 lần:
     ```text
     (006.200300) can0 100 [8] 00 05 0A 05 14 00 00 00   <-- Lệnh TUA +10S (0x05, step 10)
     ```
   - Ấn 2 lần nhanh (Double Click):
     ```text
     (007.900100) can0 100 [8] 00 06 0A 06 16 00 00 00   <-- Lệnh TUA -10S (0x06, step 10)
     ```

---

### Bước 4: Kiểm thử truyền ngược Telemetry về STM32 bằng `cansend`
Từ terminal Raspberry Pi, phát 1 gói tin phản hồi trạng thái:
```bash
# Giả lập: State = PLAYING (0x01), Track = 2 (0x02), CurTime = 60s (0x003C), Total = 240s (0x00F0), UsbReady = 2 (0x02)
cansend can0 101#0102003C00F002AA
```
**Kiểm tra trên STM32F407:**
- Đèn **LED Xanh dương (LD6 - PD15)** sẽ **bật sáng ngay lập tức**, báo hiệu STM32 nhận diện Raspberry Pi đang phát video!

---

### Bước 5: Kiểm tra dịch vụ `can-daemon` và ứng dụng Qt Video Player
Dịch vụ `can-daemon` có nhiệm vụ làm cầu nối chuyển đổi các frame CAN từ SocketCAN sang Unix Domain Socket (`/tmp/can_ipc.sock`) để điều khiển ứng dụng phát video `usb-video-player`.

1. **Xem trạng thái daemon:**
   ```bash
   systemctl status can-daemon
   ```
   *(Hiển thị `Active: active (running)` là hoạt động tốt)*.

2. **Xem log hoạt động realtime của daemon:**
   ```bash
   journalctl -u can-daemon -f
   ```
   Khi ấn nút trên STM32, terminal sẽ in log nhận lệnh điều khiển.

3. **Xem log của ứng dụng phát video Qt HMI:**
   ```bash
   journalctl -u usb-video-player -f
   ```
   Bạn sẽ thấy log in:
   ```text
   [CanIpcClient] RX CMD: 1 SEEK: 0  --> Phát video!
   [CanIpcClient] RX CMD: 3 SEEK: 0  --> Nhảy bài tiếp theo!
   ```

---

## 5. BẢNG XỬ LÝ LỖI NHANH (TROUBLESHOOTING CHECKLIST)

| Hiện tượng | Nguyên nhân | Cách xử lý dứt điểm |
| :--- | :--- | :--- |
| **`Cannot find device "can0"`** | Chưa tải overlay MCP2515 hoặc lỗi kết nối chân SPI | 1. Kiểm tra lại `/boot/config.txt` đã có `dtoverlay=mcp2515-can0...`<br>2. Kiểm tra dây SPI: CS (Pin 24), MOSI (Pin 19), MISO (Pin 21), SCK (Pin 23), INT (Pin 22/GPIO25). |
| **`MCP251x didn't enter in conf mode after reset (err=110)`** | Tần số xung SPI mặc định (10MHz) quá cao gây suy hao tín hiệu trên dây cắm jumper dài | Thêm `spimaxfrequency=1000000` (1 MHz) vào dòng overlay trong `/boot/config.txt`. |
| **`Cannot initialize MCP2515. Wrong wiring? (err=19)`** | Lệch mức điện áp logic hoặc dây MISO/MOSI bị đảo chéo | 1. Thử cấp nguồn VCC từ chân 3.3V (Pin 1) hoặc kiểm tra lại mức điện áp.<br>2. Đảm bảo SI vào Pin 19, SO vào Pin 21 (không đảo chéo). |
| **`can state ERROR-PASSIVE` hoặc `BUS-OFF`** | 1. Chưa có dây Mass chung (GND)<br>2. Thiếu trở kết thúc 120Ω<br>3. Ngược dây CAN_H và CAN_L | 1. Nối 1 dây giữa chân `GND` STM32 và `GND` Raspberry Pi.<br>2. Đóng jumper 120Ω trên cả 2 module.<br>3. Đo điện trở giữa CAN_H và CAN_L khi tắt nguồn: phải đạt xấp xỉ **60Ω**. |
| **Gửi frame bị `Checksum mismatch! Dropping frame`** | Sai thuật toán Checksum | Mã nguồn STM32 và can-daemon đều sử dụng chung công thức: `(B0 + B1 + B2 + B3) & 0xFF`. |
| **candump không nhận được tin, nhưng đèn STM32 nháy xanh** | Sai bitrate do sai tần số thạch anh MCP2515 | Kiểm tra con thạch anh trên module MCP2515. Nếu là 16MHz mà cấu hình 8MHz (hoặc ngược lại) thì bitrate thực tế sẽ bị lệch gấp đôi (250k hoặc 1M thay vì 500k). Sửa lại `oscillator=16000000` hoặc `8000000` trong `config.txt`. |
