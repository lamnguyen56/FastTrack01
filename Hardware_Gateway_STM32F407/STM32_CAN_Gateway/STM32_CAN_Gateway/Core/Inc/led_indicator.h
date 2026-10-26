/**
 * @file led_indicator.h
 * @brief Điều khiển các LED trạng thái (Button Press, TX Blink, Error, Telemetry)
 */

#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include "config.h"
#include "can_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Khởi tạo các chân GPIO cho 4 LED trên STM32F407
 */
void LED_Init(void);

/**
 * @brief Bật/Tắt LED báo trạng thái đang ấn nút (LED Orange - LD3)
 * @param isPressed true: Bật LED (đang ấn nút), false: Tắt LED (thả nút)
 */
void LED_SetButtonPressed(bool isPressed);

/**
 * @brief Kích hoạt chớp nháy nhanh LED xanh (LD4) báo đã truyền gói CAN thành công (1 nhịp cho Single Click)
 */
void LED_TriggerTxBlink(void);

/**
 * @brief Kích hoạt chớp nháy 2 nhịp nhanh LED xanh (LD4) báo cú Double Click đã thành công
 */
void LED_TriggerTxDoubleBlink(void);

/**
 * @brief Bật/Tắt LED đỏ (LD5) báo lỗi đường truyền CAN / Bus-off
 */
void LED_SetError(bool isError);

/**
 * @brief Cập nhật LED xanh dương (LD6) theo trạng thái Playback nhận từ Pi (ID 0x101)
 */
void LED_SetPlaybackState(PlaybackState_t state);

/**
 * @brief Hàm cập nhật thời gian chớp tắt non-blocking (gọi trong vòng lặp chính main loop)
 * @param currentTickMs Giá trị HAL_GetTick()
 */
void LED_Update(uint32_t currentTickMs);

#ifdef __cplusplus
}
#endif

#endif /* LED_INDICATOR_H */
