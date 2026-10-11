# EMBEDDED LINUX USB VIDEO PLAYER CONTROLLED VIA CAN
## Hệ thống nhúng Automotive: Qt5/QML • SocketCAN • GStreamer • Vector CANoe

Dự án OJT phát triển ứng dụng giải trí trên xe (In-Vehicle Infotainment - IVI) chạy trên Embedded Linux, hỗ trợ cắm USB tự động quét media và điều khiển toàn diện thông qua mạng truyền thông CAN từ Vector CANoe.

---

### TÀI LIỆU KỸ THUẬT QUAN TRỌNG
- 📘 **Tài liệu Baseline & Hướng dẫn Kỹ thuật Toàn diện:** [PROJECT_BASELINE_AND_GUIDE.md](file:///d:/04_PROJECT/FastTrack01/PROJECT_BASELINE_AND_GUIDE.md) (hoặc xem trong [Docs/PROJECT_BASELINE_AND_GUIDE.md](file:///d:/04_PROJECT/FastTrack01/Docs/PROJECT_BASELINE_AND_GUIDE.md))
- 📊 **Slide Đề bài OJT gốc:** [Docs/OJT_USB_Video_Player_CAN_Qt5_QML 1 - Copy_1.pptx](file:///d:/04_PROJECT/FastTrack01/Docs/OJT_USB_Video_Player_CAN_Qt5_QML%201%20-%20Copy_1.pptx)
- 🗄️ **Cơ sở dữ liệu Vector CAN Database:** [CANoe_Simulation/Database/VideoPlayer_CAN.dbc](file:///d:/04_PROJECT/FastTrack01/CANoe_Simulation/Database/VideoPlayer_CAN.dbc)

---

### TỔNG QUAN PHÂN TẦNG VÀ CÔNG NGHỆ
1. **Target Hardware & Driver:** Raspberry Pi 3/4 + MCP2515 SPI CAN Controller + TJA1050/SN65HVD230 Transceiver.
2. **Hệ điều hành Nhúng:** **Yocto Project 4.0 LTS (Kirkstone)**, Kernel 5.15 LTS, EGLFS KMS/DRM (không dùng X11/Wayland).
3. **Multimedia Engine:** GStreamer 1.20 với plugins giải mã H.264/MP4 hardware-accelerated.
4. **HMI & Logic:** C++17, Qt 5.15 LTS (QtQuick, QtMultimedia, SocketCAN, libudev).
5. **Test & Validation Tool:** Vector CANoe với Graphic Panel `.xvp`, CAPL Test Automation, CAN Database `.dbc`.

---

### CẤU TRÚC THƯ MỤC DỰ ÁN
```
FastTrack01/
├── Docs/                  # Slide đề bài, tài liệu kiến trúc, đặc tả
├── Hardware_BSP/          # DTS overlay, kernel config, script bring-up can0
├── Yocto/                 # Custom Yocto layer (meta-fasttrack), recipes, systemd
├── App/                   # Mã nguồn C++ Backend & QML HMI Video Player
├── CANoe_Simulation/      # Database DBC, Panels, CAPL Scripts mô phỏng & test
└── Tests/                 # Kịch bản kiểm thử đơn vị & kiểm thử tích hợp
```

Xem chi tiết hướng dẫn thực hiện trong file [PROJECT_BASELINE_AND_GUIDE.md](file:///d:/04_PROJECT/FastTrack01/PROJECT_BASELINE_AND_GUIDE.md).
