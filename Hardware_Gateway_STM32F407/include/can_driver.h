/**
 * @file can_driver.h
 * @brief Driver giao tiếp bxCAN (CAN1) tốc độ 500 kbps cho STM32F407VGT6
 */

#ifndef CAN_DRIVER_H
#define CAN_DRIVER_H

#include "config.h"
#include "can_protocol.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Con trỏ hàm callback khi nhận được gói trạng thái Telemetry (CAN ID 0x101) từ Pi
 */
typedef void (*CanStatusReceivedCallback_t)(const CanStatusPacket_t *pStatus);

/**
 * @brief Khởi tạo ngoại vi bxCAN (CAN1) ở bitrate 500 kbps, cấu hình bộ lọc filter
 * @return true nếu khởi tạo và start driver thành công
 */
bool CAN_Driver_Init(void);

/**
 * @brief Gửi gói tin lệnh CAN ID 0x100 đến Raspberry Pi Target
 * @param cmdId Mã lệnh phát (Play/Pause/Next/Prev)
 * @param seekCmd Mã lệnh tua (None/FF/REW)
 * @param seekVal Số giây tua
 * @return true nếu đẩy frame vào Tx Mailbox thành công
 */
bool CAN_Driver_SendCommand(uint8_t cmdId, uint8_t seekCmd, uint8_t seekVal);

/**
 * @brief Đăng ký callback nhận dữ liệu Telemetry ID 0x101 từ Pi
 */
void CAN_Driver_RegisterStatusCallback(CanStatusReceivedCallback_t cb);

/**
 * @brief Kiểm tra lỗi bus CAN và khôi phục nếu cần
 */
void CAN_Driver_CheckBusHealth(void);

#ifdef __cplusplus
}
#endif

#endif /* CAN_DRIVER_H */
