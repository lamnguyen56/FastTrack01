/**
 * @file Constants.h
 * @brief Shared CAN identifiers, local IPC payloads and platform defaults.
 * @note All peers must use compatible packet definitions. CAN payload encoding
 * is performed explicitly by ProtocolCodec; packed structs define local IPC.
 */
#ifndef CONSTANTS_H
#define CONSTANTS_H
#include <cstdint>
/**
 * @file Constants.h
 * @brief Hằng số hệ thống dùng chung giữa Platform (CAN, USB) và Application (Qt HMI).
 * Đồng bộ 100% với file DBC (VideoPlayer_CAN.dbc) và firmware ESP32 (CanProtocol.h).
 */
/**
 * @brief Shared definitions consumed by platform daemons and the Qt client.
 */
namespace FastTrack {

// ============================================================================
// CAN Protocol Constants (Khớp DBC & ESP32 TWAI)
// ============================================================================
constexpr uint32_t CAN_ID_COMMAND = 0x100;   ///< ESP32/CANoe -> Target (Lệnh điều khiển)
constexpr uint32_t CAN_ID_STATUS  = 0x101;   ///< Target -> ESP32/CANoe (Trạng thái phát)
constexpr int      CAN_DLC        = 8;

// CAN Commands (Byte 0 của ID 0x100)
/**
 * @brief Playback commands in command byte 0.
 */
enum class PlayerCommand : uint8_t {
    IDLE     = 0x00,
    PLAY     = 0x01,
    PAUSE    = 0x02,
    NEXT     = 0x03,
    PREVIOUS = 0x04
};

// CAN Seek Commands (Byte 1 của ID 0x100)
/**
 * @brief Seek action in command byte 1.
 */
enum class SeekCommand : uint8_t {
    NONE          = 0x00,
    FAST_FORWARD  = 0x05,
    FAST_BACKWARD = 0x06
};

// Player Playback States (Byte 0 của ID 0x101)
/**
 * @brief Playback state encoded in status byte 0.
 */
enum class PlaybackState : uint8_t {
    STOPPED = 0x00,
    PLAYING = 0x01,
    PAUSED  = 0x02,
    ERROR   = 0x03
};

// USB Health State (Byte 6 của ID 0x101)
/**
 * @brief USB state encoded in status byte 6.
 */
enum class UsbState : uint8_t {
    DISCONNECTED = 0x00,
    SCANNING     = 0x01,
    READY        = 0x02
};

// ============================================================================
// IPC Socket Constants (Unix Domain Socket giữa can-daemon và Qt App)
// ============================================================================
constexpr const char* IPC_SOCKET_CAN_DAEMON  = "/tmp/can_ipc.sock";      ///< can-daemon lắng nghe
constexpr const char* IPC_SOCKET_QT_CLIENT   = "/tmp/qt_app_ipc.sock";   ///< Qt App lắng nghe
// ============================================================================
// System Paths & Configurations
// ============================================================================
constexpr const char* DEFAULT_CAN_INTERFACE    = "can0";
constexpr int         CAN_BITRATE              = 500000;
constexpr int         CAN_RESTART_MS           = 100;
constexpr const char* DEFAULT_USB_MOUNT_POINT  = "/media/usb0";
constexpr int         CAN_STATUS_INTERVAL_MS   = 100;  ///< 100ms cyclic telemetry
// ============================================================================
// Binary IPC Packet Structs (truyền qua Unix Domain Socket, zero overhead)
// ============================================================================
#pragma pack(push, 1)
/// Gói lệnh điều khiển (CAN ID 0x100 -> IPC -> Qt App)
struct CanCommandPacket {
    uint8_t commandId;           ///< PlayerCommand
    uint8_t seekCommand;         ///< SeekCommand
    uint8_t seekStepValue;       ///< Bước nhảy giây (5s, 10s...)
    uint8_t rollingCounter;      ///< 0..15 chống duplicate frame
    uint8_t checksum;            ///< (B0 + B1 + B2 + B3) mod 256
};

/// Gói trạng thái phát (Qt App -> IPC -> CAN ID 0x101)
struct CanStatusPacket {
    uint8_t  playbackState;      ///< PlaybackState
    uint8_t  trackIndex;         ///< Index bài hát (0..255)
    uint16_t curTimeSec;         ///< Giây hiện tại (Big Endian khi gửi CAN)
    uint16_t totalDurationSec;   ///< Tổng thời lượng (Big Endian khi gửi CAN)
    uint8_t  usbHealth;          ///< UsbState
    uint8_t  aliveCounter;       ///< Heartbeat counter (0..255)
};

#pragma pack(pop)
/**
 * @brief Compute the existing additive checksum modulo 256 (not a CRC).
 * @param b0 Command identifier byte.
 * @param b1 Seek action byte.
 * @param b2 Seek value byte.
 * @param b3 Rolling counter byte.
 * @return Low eight bits of the sum of all four input bytes.
 * @note The current codec copies received checksums; this helper alone does not validate frames.
 */
inline uint8_t calculateChecksum(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) {
    return static_cast<uint8_t>((b0 + b1 + b2 + b3) & 0xFF);
}

} // namespace FastTrack
#endif // CONSTANTS_H
