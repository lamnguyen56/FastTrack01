/**
 * @file can_protocol.h
 * @brief Định nghĩa giao thức truyền thông CAN cho nút STM32F407 Gateway
 * @project FastTrack01 - USB Video Player Controlled via CAN
 * @note Hoàn toàn tương thích và đồng bộ 100% với:
 *       - DBC Database: VideoPlayer_CAN.dbc
 *       - Raspberry Pi Target: Constants.h & ProtocolCodec.cpp
 *       - ESP32 Reference: CanProtocol.h
 */

#ifndef CAN_PROTOCOL_H
#define CAN_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 1. CÁC ĐỊNH DANH VÀ THÔNG SỐ CAN BUS (THEO DBC & CONSTANTS.H)
 * ============================================================================ */
#define CAN_ID_COMMAND          0x100       /**< STM32 -> Raspberry Pi (Lệnh điều khiển) */
#define CAN_ID_STATUS           0x101       /**< Raspberry Pi -> STM32 (Trạng thái phát) */
#define CAN_DLC_STANDARD        8           /**< Chiều dài dữ liệu chuẩn: 8 bytes */
#define CAN_BAUDRATE_500KBPS    500000      /**< Tốc độ mạng CAN chuẩn: 500 kbps */

/* ============================================================================
 * 2. MÃ LỆNH ĐIỀU KHIỂN (BYTE 0 - COMMAND ID CỦA CAN ID 0x100)
 * ============================================================================ */
typedef enum {
    CAN_CMD_IDLE      = 0x00,               /**< Không có lệnh (Idle) */
    CAN_CMD_PLAY      = 0x01,               /**< Phát video */
    CAN_CMD_PAUSE     = 0x02,               /**< Tạm dừng video */
    CAN_CMD_NEXT      = 0x03,               /**< Chuyển sang video kế tiếp */
    CAN_CMD_PREVIOUS  = 0x04                /**< Quay lại video trước */
} CanCommand_t;

/* ============================================================================
 * 3. MÃ LỆNH TUA NHANH (BYTE 1 - SEEK COMMAND CỦA CAN ID 0x100)
 * ============================================================================ */
typedef enum {
    CAN_SEEK_NONE          = 0x00,          /**< Không tua */
    CAN_SEEK_FAST_FORWARD  = 0x05,          /**< Tua tiến (+seekStepValue giây) */
    CAN_SEEK_FAST_BACKWARD = 0x06           /**< Tua lùi (-seekStepValue giây) */
} CanSeekCommand_t;

/* ============================================================================
 * 4. TRẠNG THÁI PHÁT (BYTE 0 CỦA CAN ID 0x101 TỪ TARGET)
 * ============================================================================ */
typedef enum {
    PLAYBACK_STATE_STOPPED = 0x00,          /**< Dừng phát */
    PLAYBACK_STATE_PLAYING = 0x01,          /**< Đang phát */
    PLAYBACK_STATE_PAUSED  = 0x02,          /**< Đang tạm dừng */
    PLAYBACK_STATE_ERROR   = 0x03           /**< Lỗi phát video */
} PlaybackState_t;

/* ============================================================================
 * 5. TRẠNG THÁI USB STORAGE (BYTE 6 CỦA CAN ID 0x101 TỪ TARGET)
 * ============================================================================ */
typedef enum {
    USB_STATE_DISCONNECTED = 0x00,          /**< Chưa cắm USB */
    USB_STATE_SCANNING     = 0x01,          /**< Đang quét media trong USB */
    USB_STATE_READY        = 0x02           /**< USB sẵn sàng, đã nhận danh sách video */
} UsbState_t;

/* ============================================================================
 * 6. CẤU TRÚC GÓI TIN CAN LỆNH ĐIỀU KHIỂN (CAN ID 0x100)
 * ============================================================================ */
#pragma pack(push, 1)
typedef struct {
    uint8_t commandId;                      /**< Byte 0: CanCommand_t */
    uint8_t seekCommand;                    /**< Byte 1: CanSeekCommand_t */
    uint8_t seekStepValue;                  /**< Byte 2: Số giây tua (VD: 10s) */
    uint8_t rollingCounter;                 /**< Byte 3: Bộ đếm vòng (0..15) */
    uint8_t checksum;                       /**< Byte 4: Checksum bảo vệ toàn vẹn */
    uint8_t reserved[3];                    /**< Byte 5..7: Dành riêng (0x00) */
} CanCommandPacket_t;

/* ============================================================================
 * 7. CẤU TRÚC GÓI TIN TRẠNG THÁI TELEMETRY (CAN ID 0x101)
 * ============================================================================ */
typedef struct {
    uint8_t  playbackState;                 /**< Byte 0: PlaybackState_t */
    uint8_t  trackIndex;                    /**< Byte 1: Vị trí video hiện tại (0..255) */
    uint16_t curTimeSec;                    /**< Byte 2..3: Thời gian đã phát (Big-Endian) */
    uint16_t totalDurationSec;              /**< Byte 4..5: Tổng thời lượng (Big-Endian) */
    uint8_t  usbHealth;                     /**< Byte 6: Trạng thái USB (Bits 0..1) */
    uint8_t  aliveCounter;                  /**< Byte 7: Heartbeat Counter (0..255) */
} CanStatusPacket_t;
#pragma pack(pop)

/* ============================================================================
 * 8. HÀM TÍNH TOÁN CHECKSUM VÀ ĐÓNG/GIẢI MÃ GÓI TIN CAN
 * ============================================================================ */

/**
 * @brief Tính Checksum cộng dồn Modulo 256 chuẩn theo FastTrack01
 * @formula Checksum = (Byte0 + Byte1 + Byte2 + Byte3) & 0xFF
 */
static inline uint8_t CanProtocol_CalculateChecksum(uint8_t cmdId, uint8_t seekCmd, 
                                                    uint8_t seekVal, uint8_t counter) {
    return (uint8_t)((cmdId + seekCmd + seekVal + counter) & 0xFF);
}

/**
 * @brief Đóng gói buffer CAN 8 bytes cho lệnh điều khiển CAN ID 0x100
 * @param cmdId Mã lệnh phát (Play/Pause/Next/Prev)
 * @param seekCmd Mã lệnh tua (None/FF/REW)
 * @param seekVal Số giây tua
 * @param pCounter Con trỏ bộ đếm vòng (sẽ tự động tăng 0..15 sau mỗi lần đóng gói)
 * @param outData Buffer mảng 8 bytes đầu ra
 */
static inline void CanProtocol_PackCommand(uint8_t cmdId, uint8_t seekCmd, uint8_t seekVal, 
                                          uint8_t *pCounter, uint8_t outData[8]) {
    *pCounter = (*pCounter + 1) & 0x0F;
    uint8_t chksum = CanProtocol_CalculateChecksum(cmdId, seekCmd, seekVal, *pCounter);

    outData[0] = cmdId;
    outData[1] = seekCmd;
    outData[2] = seekVal;
    outData[3] = *pCounter;
    outData[4] = chksum;
    outData[5] = 0x00;
    outData[6] = 0x00;
    outData[7] = 0x00;
}

/**
 * @brief Giải mã dữ liệu 8 bytes nhận từ CAN ID 0x101
 * @param inData Buffer 8 bytes nhận từ CAN bus
 * @param outStatus Con trỏ struct chứa kết quả giải mã
 * @return true nếu giải mã thành công
 */
static inline bool CanProtocol_UnpackStatus(const uint8_t inData[8], CanStatusPacket_t *outStatus) {
    if (!inData || !outStatus) return false;

    outStatus->playbackState    = inData[0];
    outStatus->trackIndex       = inData[1];
    outStatus->curTimeSec       = (uint16_t)(((uint16_t)inData[2] << 8) | inData[3]);
    outStatus->totalDurationSec = (uint16_t)(((uint16_t)inData[4] << 8) | inData[5]);
    outStatus->usbHealth        = inData[6] & 0x03;
    outStatus->aliveCounter     = inData[7];

    return true;
}

#ifdef __cplusplus
}
#endif

#endif /* CAN_PROTOCOL_H */
