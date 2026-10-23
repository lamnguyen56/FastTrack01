# SƠ ĐỒ ĐẤU DÂY CHI TIẾT (HARDWARE PINOUT & SCHEMATIC)
## STM32F407VGT6 Automotive CAN Gateway Node

---

## 1. SƠ ĐỒ NGUYÊN LÝ KẾT NỐI KHỐI (BLOCK DIAGRAM)

```
 +-------------------------------------------------------------------------------+
 |                              HỆ THỐNG PHẦN CỨNG                              |
 +-------------------------------------------------------------------------------+

 [STM32F407VGT6 Discovery]                     [VP1050 / TJA1050 Transceiver]
 +------------------------+                   +------------------------+
 |             5V (P1 Pin)|------------------>| VCC (5V)               |
 |                    GND |------------------>| GND                    |
 |          PB9 (CAN1_TX) |------------------>| TX (TXD)               |
 |     PB8 (CAN1_RX / 5V) |<------------------| RX (RXD / 5V TTL)      |
 |                        |                   |                        |
 |   [LEDs Onboard]       |                   |                 CAN_H  |--------+
 |   - PD12: Green (TX)   |                   |                 CAN_L  |----+   |
 |   - PD13: Orange (Btn) |                   +------------------------+    |   |
 |   - PD14: Red (Err)    |                                                 |   |
 |   - PD15: Blue (Play)  |                                                 |   |
 |                        |                                                 |   |
 |   [Cụm 3 Nút Bấm]      |                        Cặp cáp xoắn đôi         |   |
 |   - Nút 1: PE0 / PA0   |                     Trở đầu cuối 120Ω ở 2 đầu   |   |
 |     (Play / Pause)     |                                                 |   |
 |   - Nút 2: PE1         |                                                 |   |
 |     (Next / Prev)      |                                                 |   |
 |   - Nút 3: PE2         |                                                 |   |
 |     (Seek +10s / -10s) |                                                 |   |
 |                        |                                                 |   |
 |                    GND |=============================================+   |   |
 +------------------------+      DÂY NỐI MASS CHUNG (COMMON GROUND)     |   |   |
                                                                        |   |   |
                                                                        v   v   v
                                              +---------------------------------+
                                              | CAN_L   CAN_H               GND |
                                              |      [MCP2515 CAN Module]       |
                                              | VCC   GND   CS   SO   SI   SCK  |
                                              +---------------------------------+
                                                 |     |     |    |    |    |
                                                 |     |     |    |    |    | (SPI Bus)
                                                 v     v     v    v    v    v
                                              +---------------------------------+
                                              | 5V    GND  Pin24 Pin21 Pin19 Pin23
                                              |          [Raspberry Pi]         |
                                              |       Linux SocketCAN can0      |
                                              +---------------------------------+
```

---

## 2. BẢNG TRA CỨU CHÂN (PINOUT TABLE)

### 2.1 STM32F407 <---> Module CAN Transceiver VP1050 (TJA1050)
| Tên chân STM32F407 | Vị trí Header Discovery | Tên chân Module VP1050 | Ghi chú kỹ thuật |
| :--- | :--- | :--- | :--- |
| **`5V`** | Header P1 Pin 5, 6 | **`VCC`** (hoặc `5V`) | **Cấp nguồn 5V** (VP1050 là chip 5V) |
| **`GND`** | Header P1 Pin 1, 2 | **`GND`** | Nối mass logic chung |
| **`PB9`** | Header P2 Pin 43 | **`TX`** (hoặc `TXD`) | Chân phát dữ liệu CAN1 (AF9) |
| **`PB8`** | Header P2 Pin 44 | **`RX`** (hoặc `RXD`) | Chân nhận CAN1 (AF9). Chân PB8 của STM32 là **FT (5V-tolerant)**, cắm trực tiếp an toàn! |

---

### 2.2 Module MCP2515 <---> Raspberry Pi Target (40-Pin GPIO)
| Chân Module MCP2515 | Tên chân trên Raspberry Pi | Vị trí chân vật lý (Physical Pin) | Chức năng |
| :--- | :--- | :--- | :--- |
| **`VCC`** | `5V Power` | **Pin 2** (hoặc Pin 4) | Nguồn cấp 5V |
| **`GND`** | `Ground` | **Pin 6** (hoặc 9, 14, 20) | Nối mass Raspberry Pi |
| **`CS`** | `GPIO 8 (SPI0_CE0)` | **Pin 24** | Chip Select |
| **`SO`** (MISO) | `GPIO 9 (SPI0_MISO)`| **Pin 21** | Master In Slave Out |
| **`SI`** (MOSI) | `GPIO 10 (SPI0_MOSI)`| **Pin 19** | Master Out Slave In |
| **`SCK`** | `GPIO 11 (SPI0_SCLK)`| **Pin 23** | SPI Serial Clock |
| **`INT`** | `GPIO 25` | **Pin 22** | Ngắt phần cứng |

---

### 2.3 Module VP1050 (STM32) <---> Module MCP2515 (Raspberry Pi)
| Chân Module VP1050 | Chân Module MCP2515 | Yêu cầu kỹ thuật |
| :--- | :--- | :--- |
| **`CAN_H`** | **`CAN_H`** | Cặp dây xoắn đôi vi sai (High) |
| **`CAN_L`** | **`CAN_L`** | Cặp dây xoắn đôi vi sai (Low) |
| **`GND` (STM32 / VP1050)** | **`GND` (Raspberry Pi / MCP2515)** | **BẮT BUỘC: Nối dây Common Ground** |

---

### 2.3 Cụm 3 Nút Bấm Điều Khiển (Port E)
| Tên chân STM32F407 | Chức năng nút bấm | Kiểu đấu dây & Hành vi |
| :--- | :--- | :--- |
| **`PE0`** *(hoặc `PA0`)* | **Nút 1: PLAY / PAUSE** | 1 chân nút nối `PE0`, chân còn lại nối `GND` (Active LOW)<br>*(Hoặc dùng trực tiếp nút xanh `PA0` có sẵn trên board Discovery)*<br>$\to$ Nhấn 1 lần: Đảo trạng thái Play $\leftrightarrow$ Pause |
| **`PE1`** | **Nút 2: NEXT / PREVIOUS** | 1 chân nút nối `PE1`, chân còn lại nối `GND` (Active LOW)<br>$\to$ **Ấn 1 lần:** `NEXT TRACK`<br>$\to$ **Ấn 2 lần (Double Click):** `PREVIOUS TRACK`<br>🛡️ *Khóa chống spam: Bỏ qua lần ấn thứ 3, 4* |
| **`PE2`** | **Nút 3: TUA +10S / -10S** | 1 chân nút nối `PE2`, chân còn lại nối `GND` (Active LOW)<br>$\to$ **Ấn 1 lần:** `TUA TIẾN +10S` (Fast Forward)<br>$\to$ **Ấn 2 lần (Double Click):** `TUA LÙI -10S` (Fast Backward)<br>🛡️ *Khóa chống spam: Bỏ qua lần ấn thứ 3, 4* |
