# HƯỚNG DẪN CẤU HÌNH, ĐẤU NỐI & NẠP FIRMWARE STM32F407 CAN GATEWAY
## Dự án FastTrack01 - Điều Khiển USB Video Player Qua Mạng CAN Ô Tô

---

## 1. TỔNG QUAN KIẾN TRÚC & MỤC TIÊU DỰ ÁN

Trong kiến trúc ban đầu của thư mục `Hardware_Gateway_ESP32`, nút điều khiển sử dụng vi điều khiển ESP32 kèm theo bộ giao diện Web Dashboard thời gian thực (Wi-Fi SoftAP, WebSockets, LittleFS) và mô phỏng giao diện HMI Cockpit. Mặc dù rất trực quan trên trình duyệt, mô hình này có độ phức tạp cao do phải duy trì stack Wi-Fi và Web Server.

Khi cần **đơn giản hóa tối đa**, loại bỏ toàn bộ các lớp Web/Wi-Fi phức tạp và tập trung vào **tính chất chuẩn công nghiệp Automotive**, dự án được chuyển đổi sang sử dụng vi điều khiển **STM32F407VGT6**:
- **Trực tiếp & Tức thời:** Điều khiển thuần túy bằng nút nhấn vật lý (**Button**), ấn nút là truyền tức thì khung tin CAN đến Raspberry Pi.
- **Phản hồi thị giác trực quan (LED Status):**
  - **LED Cam (LD3 - PD13):** Bật sáng ngay lập tức khi có nút đang được nhấn (phản hồi xúc giác).
  - **LED Xanh lá (LD4 - PD12):** Chớp sáng sắc nét khi khung tin CAN ID `0x100` được truyền thành công vào mailbox phần cứng.
  - **LED Đỏ (LD5 - PD14):** Bật sáng cảnh báo khi bus CAN bị lỗi (Bus-off, đứt dây, sai bitrate).
  - **LED Xanh dương (LD6 - PD15):** Đồng bộ trạng thái từ Raspberry Pi (sáng khi Pi đang `PLAYING` video, tắt khi `PAUSED` hoặc `STOPPED`).
- **Phần cứng ô tô chuyên dụng:** Sử dụng ngoại vi phần cứng **bxCAN (Basic Extended CAN 2.0B)** của STM32, hoạt động ở tốc độ **500 kbps**, đảm bảo độ trễ truyền gói tin dưới 1 miligiây.

```
+-----------------------------------------------------------------------------------+
|                            KIẾN TRÚC HỆ THỐNG TRUYỀN CAN                          |
+-----------------------------------------------------------------------------------+
|                                                                                   |
|  [ CỤM 3 NÚT NHẤN VẬT LÝ ]                                                         |
|   - Nút 1 (PE4 / PA0): PLAY / PAUSE (Ấn 1 lần để đảo trạng thái)                  |
|   - Nút 2 (PE5): NEXT (Ấn 1 lần) / PREVIOUS (Ấn 2 lần) - Chặn lần 3, 4            |
|   - Nút 3 (PE6): TUA +10S (Ấn 1 lần) / TUA -10S (Ấn 2 lần) - Chặn lần 3, 4         |
|            |                                                                      |
|            v                                                                      |
|  +-------------------------+       CAN1_TX (PB9)      +------------------------+  |
|  |     STM32F407VGT6       |------------------------->|     CAN Transceiver    |  |
|  |   (Core Cortex-M4)      |<-------------------------|       VP1050 (5V)      |  |
|  |                         |       CAN1_RX (PB8)      |      (TJA1050 chuẩn)   |  |
|  |   LED Status:           |                          +------------------------+  |
|  |   - LD3 (Orange): Press |                                      |   |           |
|  |   - LD4 (Green):  TX OK |                                CAN_H |   | CAN_L     |
|  |   - LD5 (Red):    Error |                                      |   |           |
|  |   - LD6 (Blue):   Play  |                          (Cặp cáp xoắn đôi 120Ω)     |
|  +-------------------------+                                      |   |           |
|                                                                   v   v           |
|  +-------------------------+       SPI Bus (spi0.0)   +------------------------+  |
|  |      Raspberry Pi       |<-------------------------|    Module MCP2515      |  |
|  |     (Linux Target)      |------------------------->|   + CAN Transceiver    |  |
|  |                         |                          |     (TJA1050/VP230)    |  |
|  |   SocketCAN 'can0'      |                          +------------------------+  |
|  |   can-daemon (ID 0x100) |                                                      |
|  |   usb-video-player (QML)|   BẮT BUỘC: DÂY NỐI CHUNG MASS ĐẤT (COMMON GROUND)   |
|  |   -> Phát Video ra HDMI |<====================================================>|
|  +-------------------------+                                                      |
+-----------------------------------------------------------------------------------+
```

---

## 2. DANH MỤC THIẾT BỊ PHẦN CỨNG CẦN CHUẨN BỊ

| STT | Thiết bị | Mô tả chi tiết | Ghi chú |
| :---: | :--- | :--- | :--- |
| **1** | **Kit STM32F407VGT6** | Bo mạch STM32F4DISCOVERY (hoặc kit STM32F407VGT6 Core Board) | Đã có sẵn vi điều khiển CAN phần cứng (bxCAN) |
| **2** | **Module CAN VP1050 (TJA1050)** | Module CAN Transceiver tốc độ cao (High-speed CAN) chạy nguồn 5V | Nối trực tiếp với ngoại vi CAN1 (PB8/PB9) của STM32 |
| **3** | **Bộ Target Raspberry Pi** | Raspberry Pi 3B / 3B+ / 4B chạy bản build Yocto FastTrack01 | Giao tiếp SocketCAN `can0` |
| **4** | **Module CAN MCP2515** | Module điều khiển CAN qua giao tiếp SPI tích hợp sẵn transceiver | Cắm vào chân Header 40-pin GPIO của Raspberry Pi |
| **5** | **Dây nối** | Cáp xoắn đôi (CAN_H, CAN_L) và dây nối mass chung (Common GND) | Khuyến nghị cáp xoắn chống nhiễu |

---

## 3. SƠ ĐỒ ĐẤU NỐI DÂY PHẦN CỨNG CHI TIẾT (WIRING DIAGRAM)

### 3.1. Đấu nối STM32F407VGT6 <---> Module CAN Transceiver VP1050 (TJA1050)

> [!IMPORTANT]
> **Lưu ý đặc biệt về điện áp của Module VP1050:**
> - Chip **VP1050 / TJA1050** là transceiver chuẩn chạy **nguồn 5V** (khác với dòng SN65HVD230 chạy 3.3V). Vì vậy chân `VCC` của module phải nối vào chân **`5V`** trên bo mạch STM32F407.
> - **Mức logic tín hiệu an toàn:** Chân `PB8` và `PB9` trên vi điều khiển STM32F407 là các chân **FT (Five-Volt Tolerant - Chịu được điện áp 5V)** theo datasheet của STMicroelectronics. Do đó, tín hiệu ngõ ra 5V từ chân RX của module VP1050 có thể cắm **trực tiếp** vào chân `PB8` của STM32 hoàn toàn an toàn mà không sợ cháy chip!

| Chân trên STM32F407 | Chân trên Module VP1050 | Chức năng | Ghi chú kỹ thuật |
| :--- | :--- | :--- | :--- |
| **`5V`** *(Header P1 Pin 5, 6)* | **`VCC`** (hoặc `5V`) | Nguồn cấp 5V | **Bắt buộc cấp 5V** để VP1050 phát đủ điện áp vi sai chuẩn |
| **`GND`** *(Header P1 Pin 1, 2)* | **`GND`** | Nối mass logic | Mass chung hệ thống |
| **`PB9`** *(Header P2 Pin 43)* | **`TX`** (hoặc `TXD`, `CTX`) | CAN1 Transmit Data | Tín hiệu 3.3V từ STM32 kích hoạt mức logic HIGH của VP1050 |
| **`PB8`** *(Header P2 Pin 44)* | **`RX`** (hoặc `RXD`, `CRX`) | CAN1 Receive Data | Chân PB8 của STM32 chịu được 5V (FT), cắm trực tiếp an toàn |

---

### 3.2. Đấu nối Module MCP2515 <---> Raspberry Pi Target (40-Pin GPIO)

Vì vi xử lý của Raspberry Pi không có phần cứng CAN tích hợp, Pi sẽ điều khiển module **MCP2515** thông qua giao tiếp SPI:

| Chân Module MCP2515 | Chân GPIO Raspberry Pi | Số chân vật lý (Pin Header) | Chức năng SPI |
| :--- | :--- | :--- | :--- |
| **`VCC`** | `5V Power` | **Pin 2** (hoặc Pin 4) | Nguồn nuôi module 5V |
| **`GND`** | `Ground` | **Pin 6** (hoặc Pin 9, 14, 20) | Mass đất Raspberry Pi |
| **`CS`** | `GPIO 8 (SPI0_CE0_N)` | **Pin 24** | Chip Select SPI |
| **`SO`** (MISO) | `GPIO 9 (SPI0_MISO)` | **Pin 21** | Dữ liệu từ MCP2515 về Pi |
| **`SI`** (MOSI) | `GPIO 10 (SPI0_MOSI)`| **Pin 19** | Lệnh từ Pi gửi sang MCP2515 |
| **`SCK`** | `GPIO 11 (SPI0_SCLK)`| **Pin 23** | Xung nhịp đồng hồ SPI |
| **`INT`** | `GPIO 25` | **Pin 22** | Chân ngắt báo có frame CAN mới |

---

### 3.3. Đấu nối Đường Truyền CAN Bus Giữa Module VP1050 và Module MCP2515

| Chân trên Module VP1050 (STM32) | Chân trên Module MCP2515 (Raspberry Pi) | Yêu cầu kỹ thuật |
| :--- | :--- | :--- |
| **`CAN_H`** | **`CAN_H`** | Cặp dây xoắn đôi vi sai (CAN High) |
| **`CAN_L`** | **`CAN_L`** | Cặp dây xoắn đôi vi sai (CAN Low) |
| **`GND` (STM32 / VP1050)** | **`GND` (Raspberry Pi / MCP2515)** | **BẮT BUỘC: Nối dây Common Ground** |

> [!CAUTION]
> **2 ĐIỀU KIỆN SỐNG CÒN ĐỂ MẠNG CAN HOẠT ĐỘNG:**
> 1. **DÂY COMMON GROUND:** Nối 1 dây giữa chân `GND` của STM32/VP1050 và chân `GND` của Raspberry Pi/MCP2515. Nếu thiếu dây này, sai lệch điện áp đất sẽ khiến chip VP1050 không thể nhận diện được bit Recessive/Dominant, dẫn đến đèn đỏ LED5 sáng (Bus-Off).
> 2. **TRỞ KẾT THÚC 120Ω:** 
>    - Cắm jumper 120Ω (thường có ký hiệu `J1` hoặc `120R`) trên module MCP2515.
>    - Đóng jumper trở 120Ω (ký hiệu `R` hoặc `120R`) trên module VP1050.
>    - Khi tắt nguồn, lấy đồng hồ VOM đo giữa dây `CAN_H` và `CAN_L` phải đạt xấp xỉ **60Ω**.

---

### 3.4. Bảng Chân Nút Bấm & Đèn LED Trên STM32F407

#### A. Cụm Đèn LED Trạng Thái (Có sẵn trên STM32F4 Discovery)
| Đèn LED | Chân MCU | Màu sắc | Chức năng hiển thị |
| :--- | :--- | :--- | :--- |
| **LD3** | `PD13` | **Cam (Orange)** | **Button Pressed:** Sáng liên tục trong suốt thời gian người dùng ấn giữ nút |
| **LD4** | `PD12` | **Xanh lá (Green)** | **TX Success:** Chớp nháy nhanh (60ms) mỗi khi gửi thành công 1 frame CAN ID 0x100 |
| **LD5** | `PD14` | **Đỏ (Red)** | **CAN Error:** Bật sáng khi bus CAN bị lỗi (Bus-off, NACK, mất trở kết thúc) |
| **LD6** | `PD15` | **Xanh dương (Blue)** | **Playback State:** Sáng khi Pi đang PLAY video; Tắt khi PAUSED/STOPPED |

#### B. Cụm 3 Nút Bấm Điều Khiển
| Nút bấm | Chân MCU | Chế độ ngõ vào | Hành vi điều khiển |
| :--- | :--- | :--- | :--- |
| **Nút 1: PLAY / PAUSE** | `PE4` *(và đồng bộ nút `PA0` Onboard)* | Active LOW (Pull-up nội) *(PA0: Active HIGH)* | **Ấn 1 lần để đảo trạng thái phát:**<br>- Nếu đang dừng/tạm dừng $\to$ Gửi lệnh `PLAY` (`0x01`)<br>- Nếu đang phát $\to$ Gửi lệnh `PAUSE` (`0x02`)<br>*(Có khóa chống rung & cooldown 200ms)* |
| **Nút 2: NEXT / PREVIOUS** | `PE5` | Active LOW (Pull-up nội) | **Không cần nhấn giữ - Phân biệt theo số lần bấm:**<br>- **Ấn 1 lần:** Nhảy video kế tiếp (`NEXT TRACK` - `0x03`) $\to$ LED xanh chớp 1 nhịp.<br>- **Ấn 2 lần (Double Click):** Quay lại video trước (`PREV TRACK` - `0x04`) $\to$ LED xanh chớp 2 nhịp nhanh.<br>🛡️ **Rào chắn chống spam:** Khóa tức thì sau cú bấm thứ 2, bỏ qua hoàn toàn các cú bấm thứ 3, 4! |
| **Nút 3: TUA +10S / -10S** | `PE6` | Active LOW (Pull-up nội) | **Không cần nhấn giữ - Phân biệt theo số lần bấm:**<br>- **Ấn 1 lần:** Tua tiến 10 giây (`SEEK +10s` - `SeekCmd=0x05, Step=10`) $\to$ LED xanh chớp 1 nhịp.<br>- **Ấn 2 lần (Double Click):** Tua lùi 10 giây (`SEEK -10s` - `SeekCmd=0x06, Step=10`) $\to$ LED xanh chớp 2 nhịp nhanh.<br>🛡️ **Rào chắn chống spam:** Khóa tức thì sau cú bấm thứ 2, bỏ qua hoàn toàn các cú bấm thứ 3, 4! |

---

## 4. CÔNG THỨC CẤU HÌNH BIT TIMING CAN 500 KBPS TRÊN STM32F407

Để đảm bảo STM32 giao tiếp đồng bộ 100% với giao diện `can0` của Raspberry Pi ở bitrate **500 kbps**, các thông số thanh ghi CAN Bit Timing được tính toán như sau:

1. **Xung nhịp bus ngoại vi:**
   - Thạch anh ngoài: $f_{OSC} = 8\text{ MHz}$
   - Bộ nhân tần PLL: $SYSCLK = 168\text{ MHz}$
   - Bộ chia xung APB1 Prescaler = 4:
     $$f_{PCLK1} = \frac{168\text{ MHz}}{4} = 42\text{ MHz}$$
   - Ngoại vi `CAN1` nhận trực tiếp xung nhịp $f_{PCLK1} = 42\text{ MHz}$.

2. **Tính toán thời gian 1 bit (Nominal Bit Time):**
   - Bitrate mong muốn: $500\text{ kbps} \implies T_{bit} = \frac{1}{500.000} = 2.000\text{ ns} = 2.0\ \mu\text{s}$.

3. **Phân bổ Time Quanta ($T_q$):**
   - Chọn bộ chia tần số Prescaler: $BRP = 6$.
   - Độ dài một đơn vị thời gian Time Quantum:
     $$T_q = \frac{BRP}{f_{PCLK1}} = \frac{6}{42.000.000\text{ Hz}} = 142.857\text{ ns}$$
   - Tổng số Time Quanta trong một bit time:
     $$N_q = \frac{T_{bit}}{T_q} = \frac{2000\text{ ns}}{142.857\text{ ns}} = 14\ T_q$$

4. **Phân chia các đoạn trong khung bit (Bit Segment Allocation):**
   - Đoạn đồng bộ (**Sync_Seg**): $1\ T_q$ (Cố định theo chuẩn ISO)
   - Đoạn định thời 1 (**BS1**): $11\ T_q$
   - Đoạn định thời 2 (**BS2**): $2\ T_q$
   - Độ bù nhảy đồng bộ (**SJW**): $1\ T_q$
   - **Tổng cộng:** $1 + 11 + 2 = 14\ T_q$.
   - **Vị trí điểm lấy mẫu (Sample Point):**
     $$\text{Sample Point} = \frac{1 + 11}{14} = \frac{12}{14} \approx 85.71\%$$
   *(Điểm lấy mẫu 85.7% hoàn toàn trùng khớp với khuyến nghị của CiA - CAN in Automation cho môi trường mạng ô tô).*

---

## 5. ĐẶC TẢ GIAO THỨC CAN THEO FILE DBC (FASTTRACK01)

### 5.1. Khung Tin Lệnh Điều Khiển: CAN ID 0x100 (STM32 -> Raspberry Pi)
- **Chuẩn ID:** Standard 11-bit (`0x100`)
- **DLC:** 8 bytes
- **Cấu trúc Payload:**

| Byte | Tên trường dữ liệu | Kiểu dữ liệu | Ý nghĩa giá trị |
| :---: | :--- | :---: | :--- |
| **Byte 0** | `CommandID` | `uint8` | `0x00`: IDLE<br>`0x01`: PLAY<br>`0x02`: PAUSE<br>`0x03`: NEXT TRACK<br>`0x04`: PREVIOUS TRACK |
| **Byte 1** | `SeekCommand` | `uint8` | `0x00`: NONE<br>`0x05`: FAST FORWARD (+X giây)<br>`0x06`: FAST BACKWARD (-X giây) |
| **Byte 2** | `SeekStepValue` | `uint8` | Bước nhảy thời gian (ví dụ: `10` giây) |
| **Byte 3** | `RollingCounter`| `uint8` | Bộ đếm vòng `0..15` (tự động tăng 1 mỗi lần gửi lệnh, chống lặp frame) |
| **Byte 4** | `Checksum` | `uint8` | Công thức: `(Byte0 + Byte1 + Byte2 + Byte3) mod 256` |
| **Byte 5** | Reserved | `uint8` | `0x00` |
| **Byte 6** | Reserved | `uint8` | `0x00` |
| **Byte 7** | Reserved | `uint8` | `0x00` |

---

### 5.2. Khung Tin Phản Hồi Trạng Thái: CAN ID 0x101 (Raspberry Pi -> STM32)
- **Chuẩn ID:** Standard 11-bit (`0x101`)
- **DLC:** 8 bytes
- **Chu kỳ gửi từ Pi:** 100 ms / frame
- **Cấu trúc Payload:**

| Byte | Tên trường dữ liệu | Ý nghĩa |
| :---: | :--- | :--- |
| **Byte 0** | `PlaybackState` | `0`: STOPPED, `1`: PLAYING, `2`: PAUSED, `3`: ERROR |
| **Byte 1** | `TrackIndex` | Chỉ số bài hát hiện tại (`0..255`) |
| **Byte 2-3** | `CurrentPlayTime` | Thời gian đang phát tính bằng giây (Big-Endian) |
| **Byte 4-5** | `TotalDuration` | Tổng thời lượng video tính bằng giây (Big-Endian) |
| **Byte 6** | `UsbHealth` | `0`: Disconnected, `1`: Scanning, `2`: Ready |
| **Byte 7** | `AliveCounter` | Nhịp tim Heartbeat của Pi (`0..255`) |

---

## 6. HƯỚNG DẪN TẠO & CẤU HÌNH PROJECT THỦ CÔNG TỪ ĐẦU TRONG STM32CUBEIDE (A-Z)

Dưới đây là quy trình chuẩn xác 100% để bạn tự tạo mới dự án từ đầu trong **STM32CubeIDE**, cấu hình ngoại vi đồ họa bằng STM32CubeMX, ghép các file mã nguồn có sẵn và nạp chương trình lên chip STM32F407.

---

### Bước 1: Khởi Tạo Dự Án Mới (Target Selector)
1. Mở phần mềm **STM32CubeIDE**.
2. Trên thanh menu trên cùng, chọn:
   $$\text{File} \longrightarrow \text{New} \longrightarrow \text{\textbf{STM32 Project}}$$
   *(Hoặc trên trang chào mừng ban đầu, bấm nút lớn: **`Start new STM32 project`**)*.
3. Hộp thoại **Target Selector** hiện ra:
   - **Nếu dùng kit STM32F4 Discovery:** Chuyển sang tab **Board Selector** $\to$ Tại ô tìm kiếm gõ `STM32F4DISCOVERY` $\to$ Click chọn bo mạch $\to$ Bấm nút **`Next`**.
   - **Nếu dùng bo mạch rời:** Chuyển sang tab **MCU/MPU Selector** $\to$ Gõ `STM32F407VG` $\to$ Click chọn chip `STM32F407VGTx` $\to$ Bấm nút **`Next`**.
4. Thiết lập thông tin dự án:
   - **Project Name:** `STM32_CAN_Gateway` (hoặc tên tùy bạn đặt).
   - **Targeted Language:** Chọn **`C`**.
   - **Targeted Binary Type:** `Executable`.
   - **Targeted Project Type:** `STM32Cube`.
5. Bấm nút **`Finish`**.
6. Khi có hộp thoại hỏi: *"Initialize all peripherals with their default mode?"* $\implies$ **CHỌN `No`** *(Chọn `No` để cấu hình sạch sẽ từ đầu, tránh bị tự động gán các chân thừa của chip âm thanh hoặc cảm biến gia tốc)*.

---

### Bước 2: Cấu Hình Hệ Thống Cơ Bản (System Core: SYS & RCC)
Giao diện đồ họa chip vi điều khiển (`.ioc`) xuất hiện trên màn hình:
1. Trong danh mục phân cấp bên trái, mở **System Core** $\to$ Click chọn **`SYS`**:
   - Tại mục **Debug**: Đổi từ *No Debug* sang **`Serial Wire`** *(CỰC KỲ QUAN TRỌNG: Lựa chọn này kích hoạt 2 chân nạp SWDIO/SWCLK để ST-Link luôn nạp và debug được, tránh bị khóa chip)*.
2. Mở **System Core** $\to$ Click chọn **`RCC`**:
   - Tại mục **High Speed Clock (HSE)**: Chọn **`Crystal/Ceramic Resonator`** *(Kích hoạt thạch anh ngoài 8MHz có sẵn trên kit)*.

---

---

### Bước 3: Cấu Hình Xung Nhịp Hệ Thống (Clock Configuration Tab) - BẮT BUỘC LÀM TRƯỚC!

> [!IMPORTANT]
> **TẠI SAO PHẢI CẤU HÌNH CLOCK TRƯỚC KHI CẤU HÌNH CAN1?**
> Ngoại vi CAN1 nằm trên đường bus **APB1**. 
> - Mặc định khi vừa tạo project, xung nhịp APB1 chỉ là **16 MHz** (sử dụng dao động nội HSI). Với xung 16 MHz, nếu bạn nhập Prescaler=6, BS1=11, BS2=2 thì công thức $\frac{16.000.000}{6 \times (1 + 11 + 2)} = \frac{16.000.000}{84} \approx \mathbf{190.476\text{ bit/s}}$!
> - Vì vậy, bạn **PHẢI** cấu hình Clock đưa **APB1 lên 42 MHz** trước, sau đó khi sang CAN1 nhập tham số thì CubeMX mới tính ra đúng chuẩn **500.000 bit/s** (500 kbps)!

Chuyển sang tab **Clock Configuration** ở thanh ngang phía trên:
1. Tại khối nguồn **HSE (MHz)** bên trái: Nhập số **`8`**.
2. Tích chọn nút radio chuyển sang nguồn **`HSE`** (thay vì HSI).
3. Tại ô **`HCLK (MHz)`** ở giữa màn hình: Gõ số **`168`** rồi nhấn phím **Enter** trên bàn phím.
4. STM32CubeMX sẽ tự động giải hệ số chia và nhân xung nhịp ($M=8, N=336, P=2$).
5. 👁️ **Kiểm tra kỹ:** Nhìn vào nhánh **`APB1 peripheral clocks (MHz)`** đảm bảo hiển thị đúng số **`42`** MHz!

---

### Bước 4: Cấu Hình Ngoại Vi CAN1 Tốc Độ 500 kbps (Connectivity)
Quay trở lại tab **Pinout & Configuration** ở thanh ngang phía trên:
1. Trong danh mục phân cấp bên trái, mở **Connectivity** $\to$ Click chọn **`CAN1`**:
   - Tích chọn ô: **`Activated`**.
2. **Đổi chân sang PB8 / PB9 (Pinout Mapping):**
   - Mặc định CubeMX có thể gắn CAN1 vào chân PA11/PA12 (chân này trùng với cổng USB OTG).
   - Hãy nhìn vào sơ đồ chân chip bên phải:
     - Tìm chân **`PB8`** $\to$ Click chuột trái $\to$ Chọn **`CAN1_RX`**.
     - Tìm chân **`PB9`** $\to$ Click chuột trái $\to$ Chọn **`CAN1_TX`**.
3. **Cấu hình bảng tham số định thời (Parameter Settings):**
   Chuyển sang tab **Parameter Settings** bên dưới mục CAN1 và nhập chính xác:
   - `Prescaler (for Time Quantum)`: **`6`**
   - `Time Quanta in Bit Segment 1`: **`11 Times`**
   - `Time Quanta in Bit Segment 2`: **`2 Times`**
   - `Sync Jump Width`: **`1 Time`**
   - 👁️ **Kiểm tra dòng `Baud Rate`:** Nhờ APB1 đã là 42 MHz ở Bước 3, phần mềm tự động tính ra hiển thị chữ màu xanh lá:
     $$\text{Baud Rate} = \frac{42.000.000\text{ Hz}}{6 \times (1 + 11 + 2)} = \frac{42.000.000}{84} = \mathbf{500000\text{ bit/s}}\quad (500\text{ kbps chuẩn!})$$
     *(Nếu bạn vẫn thấy hiện `190476 bit/s`, hãy kiểm tra lại Bước 3 xem APB1 đã đạt 42 MHz chưa!)*
   - `Automatic Bus-Off Management`: Chọn **`Enable`** *(Tự hồi phục khi đường truyền CAN gặp lỗi)*.
   - `Automatic Retransmission`: Chọn **`Enable`** *(Tự gửi lại nếu Raspberry Pi chưa kịp ACK)*.
4. **Bật ngắt nhận tin nhắn (NVIC Settings):**
   - Chuyển sang tab **NVIC Settings** của CAN1:
   - Tích chọn **`Enabled`** ở dòng: **`CAN1 RX0 interrupt`** *(Preemption Priority: 2)*.
   - Tích chọn **`Enabled`** ở dòng: **`CAN1 SCE interrupt`** *(Preemption Priority: 3)*.

---

### Bước 5: Cấu Hình Chân Cho 4 Đèn LED & Cụm 3 Nút Bấm (GPIO)
Trên hình ảnh chip vi điều khiển bên phải, click chuột trái vào các chân để gán chức năng:
1. **Cụm 4 Đèn LED Trạng Thái (Output):**
   - Click chân **`PD12`** $\to$ Chọn **`GPIO_Output`** (Click phải $\to$ Enter User Label: `LED_GREEN`).
   - Click chân **`PD13`** $\to$ Chọn **`GPIO_Output`** (User Label: `LED_ORANGE`).
   - Click chân **`PD14`** $\to$ Chọn **`GPIO_Output`** (User Label: `LED_RED`).
   - Click chân **`PD15`** $\to$ Chọn **`GPIO_Output`** (User Label: `LED_BLUE`).
2. **Cụm 3 Nút Bấm Điều Khiển (Input Pull-Up):**
   > [!IMPORTANT]
   > **Lưu ý phần cứng đặc biệt trên STM32F4 Discovery:**
   > Các chân `PE0` và `PE1` bị nối ngầm vào chân ngắt `INT1`, `INT2` của cảm biến gia tốc LIS3DSH có sẵn trên mạch. Do đó, chúng ta sử dụng các chân tự do hoàn toàn **`PE4`**, **`PE5`**, **`PE6`** (nằm liền kề trên Header P2) để tránh bị cảm biến ghì mức 0V!
   - Click chân **`PA0`** $\to$ Chọn **`GPIO_Input`** (User Label: `BTN_USER`).
   - Click chân **`PE4`** $\to$ Chọn **`GPIO_Input`** (User Label: `BTN_PLAY_PAUSE`).
   - Click chân **`PE5`** $\to$ Chọn **`GPIO_Input`** (User Label: `BTN_NEXT_PREV`).
   - Click chân **`PE6`** $\to$ Chọn **`GPIO_Input`** (User Label: `BTN_SEEK`).
3. **Bật trở kéo lên nội (Pull-up) cho 3 nút:**
   - Vào lại mục **System Core** bên trái $\to$ Click chọn **`GPIO`**:
   - Trong bảng danh sách, chọn lần lượt 3 dòng `PE4`, `PE5`, `PE6`:
   - Ở cột **GPIO Pull-up/Pull-down**, đổi từ `No pull-up and no pull-down` thành **`Pull-up`**.

---

### Bước 6: Cấu Hình Sinh Code (Project Manager Tab)
Chuyển sang tab **Project Manager** ở thanh ngang trên cùng:
1. Chọn mục **Code Generator** ở thanh bên trái:
   - Tích chọn: **`Copy only the necessary library files`**.
   - Tích chọn: **`Generate peripheral initialization as a pair of '.c/.h' files per peripheral`** *(Tách riêng can.c/h, gpio.c/h giúp code gọn gàng chuẩn công nghiệp)*.
2. Nhấn tổ hợp phím **`Ctrl + S`** (hoặc bấm biểu tượng Bánh răng) $\to$ Hộp thoại hiện ra: *"Do you want to generate Code?"* $\to$ Bấm **`Yes`**.

---

### Bước 7: Ghép Các File Mã Nguồn Vào Dự Án
Sau khi CubeMX sinh code xong, bạn copy các file mã nguồn logic từ thư mục [Hardware_Gateway_STM32F407](.) vào project vừa tạo:
1. **Copy các file Header vào thư mục `Core/Inc/` của project vừa tạo:**
   - [include/config.h](include/config.h) *(Chứa định nghĩa chân GPIO, bitrate 500k và thời gian FSM)*
   - [include/can_protocol.h](include/can_protocol.h) *(Định nghĩa khung tin CAN 0x100, 0x101, Checksum, Counter)*
   - [include/button_manager.h](include/button_manager.h) *(Giao diện FSM quản lý nút bấm Single/Double click)*
   - [include/led_indicator.h](include/led_indicator.h) *(Giao diện điều khiển 4 LED trạng thái)*

2. **Copy các file Source vào thư mục `Core/Src/` của project vừa tạo:**
   - [src/button_manager.c](src/button_manager.c) *(Xử lý máy trạng thái nút, chống rung, chống spam)*
   - [src/led_indicator.c](src/led_indicator.c) *(Xử lý chớp nháy LED non-blocking)*

3. **Tích hợp vào file `Core/Src/main.c`:**
   Mở file `main.c` và dán code vào đúng các vùng comment đánh dấu của CubeMX (để không bị mất khi Generate Code lại):
   - **Thêm Includes** (Tại vị trí `USER CODE BEGIN Includes`):
     ```c
     /* USER CODE BEGIN Includes */
     #include "config.h"
     #include "can_protocol.h"
     #include "button_manager.h"
     #include "led_indicator.h"
     /* USER CODE END Includes */
     ```

   - **Định nghĩa hàm gửi CAN** (Tại vị trí `USER CODE BEGIN 0`):
     ```c
     /* USER CODE BEGIN 0 */
     static void OnButtonCommandTriggered(uint8_t cmd, uint8_t seek, uint8_t val) {
         CAN_TxHeaderTypeDef txHeader = {0};
         txHeader.StdId = CAN_ID_COMMAND; // 0x100
         txHeader.IDE   = CAN_ID_STD;
         txHeader.RTR   = CAN_RTR_DATA;
         txHeader.DLC   = 8;
         static uint8_t counter = 0;
         uint8_t data[8];
         CanProtocol_PackCommand(cmd, seek, val, &counter, data);
         uint32_t mailbox;
         if (HAL_CAN_AddTxMessage(&hcan1, &txHeader, data, &mailbox) == HAL_OK) {
             LED_TriggerTxBlink(); // Chớp nháy LED xanh lá báo truyền thành công
             LED_SetError(false);
         } else {
             LED_SetError(true);   // Bật LED đỏ nếu mailbox bị nghẽn
         }
     }
     /* USER CODE END 0 */
     ```

   - **Khởi động ngoại vi** (Trong hàm `main()`, tại vị trí `USER CODE BEGIN 2` sau `MX_CAN1_Init()`):
     ```c
     /* USER CODE BEGIN 2 */
     LED_Init();
     Button_Init();
     
     // 1. Cấu hình Filter Bank 0 nhận toàn bộ frame CAN
     CAN_FilterTypeDef sFilterConfig = {0};
     sFilterConfig.FilterBank = 0;
     sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
     sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
     sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
     sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;
     HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig);
     
     // 2. Bật ngoại vi CAN1 và kích hoạt ngắt nhận tin nhắn
     HAL_CAN_Start(&hcan1);
     HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_ERROR | CAN_IT_BUSOFF);
     
     // 3. Đăng ký callback khi ấn nút -> gửi lệnh CAN
     Button_RegisterCallback(OnButtonCommandTriggered);
     /* USER CODE END 2 */
     ```

   - **Vòng lặp chính** (Tại vị trí `USER CODE BEGIN WHILE`):
     ```c
     /* USER CODE BEGIN WHILE */
     while (1)
     {
         uint32_t now = HAL_GetTick();
         Button_Scan(now);    // Quét cụm 3 nút (Single / Double Click)
         LED_Update(now);     // Cập nhật chớp tắt LED trạng thái
         HAL_Delay(2);
         /* USER CODE END WHILE */
     ```

4. **Xử lý ngắt nhận Telemetry từ Raspberry Pi (File `Core/Src/stm32f4xx_it.c`):**
   Mở file `stm32f4xx_it.c`, thêm hàm callback vào cuối file tại vùng `USER CODE BEGIN 1`:
   ```c
   /* USER CODE BEGIN 1 */
   void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
       if (hcan->Instance == CAN1) {
           CAN_RxHeaderTypeDef rxHeader;
           uint8_t rxData[8];
           if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK) {
               if (rxHeader.StdId == 0x101) {
                   // Nhận phản hồi trạng thái từ Pi -> Cập nhật LED xanh dương
                   LED_SetPlaybackState((PlaybackState_t)rxData[0]);
                   Button_UpdatePlaybackState((PlaybackState_t)rxData[0]);
               }
           }
       }
   }
   /* USER CODE END 1 */
   ```

---

### Bước 8: Biên Dịch Và Nạp Firmware
1. Bấm tổ hợp phím **`Ctrl + B`** (hoặc chọn menu `Project` $\to$ `Build All`).
2. Kiểm tra cửa sổ Console: Hiển thị **`0 errors, 0 warnings`**.
3. Cắm cáp USB ST-Link từ bo mạch STM32F4 Discovery vào máy tính.
4. Bấm nút **Run** (biểu tượng mũi tên tam giác màu xanh) trên thanh Toolbar $\to$ Chương trình sẽ tự động nạp thẳng vào bộ nhớ Flash của STM32F407!

---

## 7. QUY TRÌNH KIỂM THỬ THỰC TẾ VỚI RASPBERRY PI TARGET

> 📘 **Cẩm nang chi tiết cho phía Raspberry Pi Yocto:** Xem thêm tài liệu chuyên sâu tại [Yocto/CAN_PI_TESTING_GUIDE.md](../Yocto/CAN_PI_TESTING_GUIDE.md).

### Bước 0: Kích Hoạt Phần Cứng MCP2515 Trên Raspberry Pi (`config.txt`)
Trước khi có thể mở giao diện `can0`, kernel Linux trên Raspberry Pi cần được nạp Device Tree Overlay cho chip MCP2515:

1. **Nếu bạn đang dùng thẻ nhớ chạy sẵn:** Trên console của Pi (chạy Yocto), sửa cấu hình boot bằng lệnh `sed` (hoặc dùng trình soạn thảo `vi`):
   ```bash
   # Cách 1: Tự động ghi cấu hình bằng lệnh sed (nhanh nhất)
   sed -i 's/dtoverlay=mcp2515-can0.*/dtoverlay=mcp2515-can0,oscillator=8000000,interrupt=25,spimaxfrequency=1000000/' /boot/config.txt
   
   # Hoặc nếu chưa có dòng này, thêm thủ công bằng vi (/boot/config.txt):
   # dtparam=spi=on
   # dtoverlay=mcp2515-can0,oscillator=8000000,interrupt=25,spimaxfrequency=1000000
   ```
   > [!TIP]
   > **Tại sao cần `spimaxfrequency=1000000` (1 MHz)?**
   > Khi dùng dây cắm jumper nối từ Header Pi sang module MCP2515, dây dài gây suy hao xung clock SPI 10MHz mặc định $\to$ gây lỗi timeout `err=110` (*didn't enter in conf mode*). Hạ xuống 1MHz giúp đường truyền SPI ổn định tuyệt đối!
   *(Lưu ý: Nếu thạch anh trên module MCP2515 ghi số `16.000` thì đổi thành `oscillator=16000000`).*
   Sau đó khởi động lại Pi: `reboot`.

2. **Nếu build bản OS mới bằng Yocto:**
   Hệ thống Yocto trong thư mục [Yocto/meta-usb-video-player/recipes-bsp/bootfiles/rpi-config_git.bbappend](../Yocto/meta-usb-video-player/recipes-bsp/bootfiles/rpi-config_git.bbappend) đã được cấu hình sẵn `CAN_OSCILLATOR = "8000000"` và `ENABLE_CAN = "1"`, image sinh ra sẽ tự động kích hoạt CAN!

---

### Bước 1: Kiểm Tra Driver & Bật Giao Tiếp SocketCAN Trên Raspberry Pi
1. **Kiểm tra kernel đã nhận chip MCP2515:**
   ```bash
   dmesg | grep -Ei 'mcp251|spi|can'
   ```
   *(Phải nhìn thấy dòng: `mcp251x spi0.0 can0: MCP2515 successfully initialized`)*.

2. **Kích hoạt giao tiếp SocketCAN `can0` ở bitrate 500 kbps:**
   ```bash
   # Thiết lập bitrate 500 kbps và bật giao diện can0
   sudo ip link set can0 down
   sudo ip link set can0 up type can bitrate 500000 restart-ms 100

   # Kiểm tra trạng thái interface
   ip -details link show can0
   ```
   *(Khi thấy `state ERROR-ACTIVE`, đường truyền vật lý đã sẵn sàng!)*

---

### Bước 2: Quan sát dữ liệu khi ấn nút trên STM32 (Bằng công cụ candump)
Trên Raspberry Pi, chạy lệnh lắng nghe:
```bash
candump -tz can0
```

Bây giờ hãy thử nghiệm ấn từng nút trên cụm 3 nút:

1. **Thử Nút 1 (PE4 hoặc PA0 Onboard) - Lệnh PLAY / PAUSE:**
   - Đèn LED Cam (LD3) bật sáng khi ấn giữ nút.
   - Khi nhả nút, LED Xanh lá (LD4) chớp nháy xác nhận gửi thành công.
   - **Ấn lần 1 (Gửi PLAY):**
     ```text
     (000.000000) can0 100 [8] 01 00 00 01 02 00 00 00
     ```
     *(Giải thích: Cmd=0x01 (PLAY), Counter=0x01, Checksum = (1+0+0+1) = 0x02).*
   - **Ấn lần 2 (Gửi PAUSE):**
     ```text
     (001.250320) can0 100 [8] 02 00 00 02 04 00 00 00
     ```
     *(Giải thích: Cmd=0x02 (PAUSE), Counter=0x02, Checksum = (2+0+0+2) = 0x04).*

2. **Thử Nút 2 (PE5) - Lệnh NEXT / PREVIOUS (Ấn 1 lần / Ấn 2 lần):**
   - **Ấn 1 lần:** Sau 350ms xác nhận cú ấn đơn $\to$ Gửi NEXT TRACK (`0x03`), LED xanh chớp 1 nhịp:
     ```text
     (002.810140) can0 100 [8] 03 00 00 03 06 00 00 00
     ```
   - **Ấn 2 lần nhanh (Double Click trong vòng 350ms):** Ngay tại cú bấm thứ hai, gửi tức thì PREVIOUS TRACK (`0x04`), LED xanh chớp 2 nhịp nhanh:
     ```text
     (004.102550) can0 100 [8] 04 00 00 04 08 00 00 00
     ```
   - 🛡️ **Kiểm tra chống spam:** Thử bấm liên tục 3, 4 lần nhanh $\to$ Hệ thống chỉ nhận đúng lệnh Double Click ở 2 cú bấm đầu, toàn bộ các cú bấm thứ 3, 4 tiếp theo bị hủy hoàn toàn!

3. **Thử Nút 3 (PE6) - Lệnh TUA +10S / -10S (Ấn 1 lần / Ấn 2 lần):**
   - **Ấn 1 lần:** Sau 350ms xác nhận cú ấn đơn $\to$ Gửi TUA TIẾN +10S (`SeekCmd=0x05, Step=10`), LED xanh chớp 1 nhịp:
     ```text
     (005.520410) can0 100 [8] 00 05 0A 05 14 00 00 00
     ```
     *(Giải thích: Cmd=0, Seek=0x05 (FF), Step=10 (0x0A), Counter=5, Checksum = (0+5+10+5) = 20 = 0x14).*
   - **Ấn 2 lần nhanh (Double Click trong vòng 350ms):** Ngay tại cú bấm thứ hai, gửi tức thì TUA LÙI -10S (`SeekCmd=0x06, Step=10`), LED xanh chớp 2 nhịp nhanh:
     ```text
     (007.210980) can0 100 [8] 00 06 0A 06 16 00 00 00
     ```
     *(Giải thích: Cmd=0, Seek=0x06 (REW), Step=10 (0x0A), Counter=6, Checksum = (0+6+10+6) = 22 = 0x16).*
   - 🛡️ **Kiểm tra chống spam:** Thử bấm dồn dập 3, 4 lần $\to$ Hệ thống bỏ qua các cú bấm thừa, không phát sinh frame sai lệch!

---

### Bước 3: Thử nghiệm gửi phản hồi Telemetry từ Pi về STM32
Để kiểm tra khả năng nhận tin của STM32, từ Raspberry Pi hãy gửi thử 1 frame trạng thái mô phỏng:
```bash
# Gửi trạng thái: Đang Play (0x01), Track số 3 (0x03), thời lượng 100s / 300s, USB Ready (0x02)
cansend can0 101#01030064012C02FF
```
**Kết quả trên STM32F407:**
- Đèn **LED Xanh dương (LD6 - PD15)** trên STM32 sẽ **bật sáng** ngay lập tức, chứng minh STM32 đã nhận diện chính xác Raspberry Pi đang ở trạng thái `PLAYING`!

---

### Bước 4: Chạy toàn bộ hệ thống phát video hoàn chỉnh
Khởi chạy dịch vụ `can-daemon` và ứng dụng `usb-video-player` trên Raspberry Pi:
```bash
# Chạy can-daemon
systemctl start can-daemon

# Cắm USB có chứa video MP4 vào cổng USB Raspberry Pi
# Ứng dụng Qt HMI sẽ hiển thị trên màn hình HDMI
# Ấn nút PA0 trên STM32 để điều khiển Play/Pause/Next/Prev trực tiếp trên màn hình!
```

---

## 8. CẨM NANG CHẨN ĐOÁN & KHẮC PHỤC SỰ CỐ (TROUBLESHOOTING)

| Hiện tượng | Nguyên nhân chính | Cách xử lý dứt điểm |
| :--- | :--- | :--- |
| **LED Cam (LD3) sáng liên tục dù đã rút hết nút bấm** | Chân `PE0`, `PE1` trên kit Discovery bị nối ngầm vào chân ngắt `INT1`, `INT2` của cảm biến gia tốc MEMS LIS3DSH (kéo chân xuống 0V) | **Bắt buộc dùng cụm chân tự do `PE4`, `PE5`, `PE6`** (Header P2 Pin 7, 8, 9) và bật trở kéo lên nội `Pull-up` trong STM32CubeMX! |
| **Pi báo `didn't enter in conf mode after reset (err=110)`** | Tần số xung SPI mặc định (10MHz) quá cao gây suy hao tín hiệu trên dây cắm jumper dài | Thêm `spimaxfrequency=1000000` (1 MHz) vào dòng overlay trong `/boot/config.txt`. |
| **Pi báo `can state ERROR-PASSIVE`** | Mức điện áp vi sai CAN bị trôi do thiếu mass đất hoặc thiếu trở | 1. **Bắt buộc nối 1 dây GND giữa STM32 và Raspberry Pi**<br>2. Đóng jumper 120Ω trên cả module VP1050 và MCP2515<br>3. Đảm bảo VP1050 được cấp nguồn **5V**. |
| **LED Đỏ (LD5) sáng liên tục khi vừa bật nguồn hoặc vừa ấn nút** | 1. Chưa nối chung mass đất (**Common Ground**)<br>2. Thiếu trở kết thúc 120Ω<br>3. Đảo ngược dây CAN_H và CAN_L | 1. Cắm 1 dây từ `GND STM32` sang `GND Raspberry Pi`<br>2. Đóng jumper 120Ω trên cả 2 đầu module<br>3. Kiểm tra nối đúng CAN_H sang CAN_H, CAN_L sang CAN_L |
| **`candump can0` không nhận được bất kỳ frame nào** | 1. Sai bitrate (Pi chạy 250k hoặc 1M thay vì 500k)<br>2. Module MCP2515 dùng thạch anh khác (16MHz thay vì 8MHz)<br>3. Chân PB8/PB9 chưa được cắm chắc | 1. Chạy lại `ip link set can0 up type can bitrate 500000`<br>2. Kiểm tra file cấu hình overlay `config.txt` trên Pi (`dtoverlay=mcp2515-can0,oscillator=...`) |
| **Pi báo `Checksum mismatch! Dropping frame`** | Thuật toán Checksum bị sai lệch | Mã nguồn STM32 này đã sử dụng đúng công thức: `(Byte0 + Byte1 + Byte2 + Byte3) & 0xFF`, hoàn toàn tương thích với file `Constants.h` của Pi |
| **Nút bấm bị nhảy nhiều lệnh cùng 1 lúc** | Hiện tượng rung phím cơ học (contact bounce) | Mã nguồn đã tích hợp sẵn thuật toán lọc rung `BUTTON_DEBOUNCE_MS = 50ms`, đảm bảo 1 lần nhấn chỉ kích hoạt đúng 1 frame |
