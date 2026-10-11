#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>

namespace FastTrack {

// CAN Identifier Definition
constexpr uint32_t CAN_ID_COMMAND = 0x100;
constexpr uint32_t CAN_ID_STATUS  = 0x101;

// CAN Commands (Byte 0)
enum class PlayerCommand : uint8_t {
    IDLE     = 0x00,
    PLAY     = 0x01,
    PAUSE    = 0x02,
    NEXT     = 0x03,
    PREVIOUS = 0x04
};

// CAN Seek Commands (Byte 1)
enum class SeekCommand : uint8_t {
    NONE          = 0x00,
    FAST_FORWARD  = 0x05,
    FAST_BACKWARD = 0x06
};

// Player Playback States (Byte 0 of 0x101)
enum class PlaybackState : uint8_t {
    STOPPED = 0x00,
    PLAYING = 0x01,
    PAUSED  = 0x02,
    ERROR   = 0x03
};

// USB Health State (Bit in Byte 6)
enum class UsbState : uint8_t {
    DISCONNECTED = 0x00,
    SCANNING     = 0x01,
    READY        = 0x02
};

// Default paths and configurations
constexpr const char* DEFAULT_CAN_INTERFACE = "can0";
constexpr const char* DEFAULT_USB_MOUNT_POINT = "/media/usb0";
constexpr int CAN_STATUS_INTERVAL_MS = 100; // 100ms cyclic status

} // namespace FastTrack

#endif // CONSTANTS_H
