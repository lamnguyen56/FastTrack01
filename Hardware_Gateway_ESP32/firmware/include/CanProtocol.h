#pragma once

#include <Arduino.h>
#include <driver/twai.h>
#include "Config.h"

// ============================================================================
// ĐẶC TẢ GIAO THỨC CAN THEO FILE DBC (VideoPlayer_CAN.dbc)
// ============================================================================

enum PlaybackState : uint8_t {
    STATE_STOPPED = 0,
    STATE_PLAYING = 1,
    STATE_PAUSED  = 2,
    STATE_ERROR   = 3
};

enum UsbHealthState : uint8_t {
    USB_DISCONNECTED = 0,
    USB_SCANNING     = 1,
    USB_READY        = 2
};

enum CommandType : uint8_t {
    CMD_IDLE = 0,
    CMD_PLAY = 1,
    CMD_PAUSE = 2,
    CMD_NEXT = 3,
    CMD_PREV = 4
};

enum SeekType : uint8_t {
    SEEK_NONE = 0,
    SEEK_FAST_FORWARD = 5,
    SEEK_FAST_BACKWARD = 6
};

// Struct đại diện dữ liệu Telemetry nhận từ Target (CAN ID 0x101)
struct CanStatusPacket {
    uint8_t  playbackState;      // Byte 0: 0=Stopped, 1=Playing, 2=Paused, 3=Error
    uint8_t  trackIndex;         // Byte 1: Chỉ số video (0..255)
    uint16_t curTimeSec;         // Byte 2..3: Thời gian phát hiện tại (Big Endian)
    uint16_t totalDurationSec;   // Byte 4..5: Tổng thời lượng video (Big Endian)
    uint8_t  usbHealth;          // Byte 6 (Bits 0..1): 0=No USB, 1=Scan, 2=Ready
    uint8_t  aliveCounter;       // Byte 7: Heartbeat Counter (0..255)
};

// Struct đại diện dữ liệu Lệnh điều khiển gửi đi (CAN ID 0x100)
struct CanCommandPacket {
    uint8_t commandId;           // Byte 0: Play/Pause/Next/Prev
    uint8_t seekCommand;         // Byte 1: Fast Forward/Rewind
    uint8_t seekStepValue;       // Byte 2: Bước nhảy giây (5s, 10s...)
    uint8_t rollingCounter;      // Byte 3 (Bits 0..3): 0..15
    uint8_t checksum;            // Byte 4: (B0 + B1 + B2 + B3) mod 256
};

// Tính toán Checksum theo chuẩn đặc tả của dự án
inline uint8_t calculateCanChecksum(uint8_t cmdId, uint8_t seekCmd, uint8_t seekVal, uint8_t counter) {
    return (uint8_t)((cmdId + seekCmd + seekVal + counter) & 0xFF);
}

// Đóng gói struct thành frame TWAI ID 0x100
inline void packCanCommand(uint8_t cmdId, uint8_t seekCmd, uint8_t seekVal, uint8_t &counter, twai_message_t &outMsg) {
    outMsg.identifier = CAN_ID_COMMAND;
    outMsg.extd = 0;             // Standard 11-bit identifier
    outMsg.rtr = 0;
    outMsg.data_length_code = CAN_DLC_STANDARD;

    counter = (counter + 1) & 0x0F;
    uint8_t chksum = calculateCanChecksum(cmdId, seekCmd, seekVal, counter);

    outMsg.data[0] = cmdId;
    outMsg.data[1] = seekCmd;
    outMsg.data[2] = seekVal;
    outMsg.data[3] = counter;
    outMsg.data[4] = chksum;
    outMsg.data[5] = 0x00;
    outMsg.data[6] = 0x00;
    outMsg.data[7] = 0x00;
}

// Giải mã frame TWAI ID 0x101 thành struct
inline bool unpackCanStatus(const twai_message_t &msg, CanStatusPacket &outStatus) {
    if (msg.identifier != CAN_ID_STATUS || msg.data_length_code != CAN_DLC_STANDARD) {
        return false;
    }

    outStatus.playbackState    = msg.data[0];
    outStatus.trackIndex       = msg.data[1];
    outStatus.curTimeSec       = (msg.data[2] << 8) | msg.data[3];      // Big Endian
    outStatus.totalDurationSec = (msg.data[4] << 8) | msg.data[5];      // Big Endian
    outStatus.usbHealth        = (msg.data[6] & 0x03);
    outStatus.aliveCounter     = msg.data[7];

    return true;
}
