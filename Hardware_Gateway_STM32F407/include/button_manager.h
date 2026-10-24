/**
 * @file button_manager.h
 * @brief Quản lý quét nút nhấn vật lý, chống rung phím (debounce) và ánh xạ lệnh CAN
 */

#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include "config.h"
#include "can_protocol.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Con trỏ hàm callback khi có sự kiện nhấn nút phát sinh lệnh CAN
 */
typedef void (*ButtonCommandCallback_t)(uint8_t cmdId, uint8_t seekCmd, uint8_t seekVal);

/**
 * @brief Khởi tạo các chân GPIO nút bấm (Nút onboard PA0 và các nút SWC mở rộng)
 */
void Button_Init(void);

/**
 * @brief Đăng ký hàm callback nhận lệnh khi nút được nhấn
 */
void Button_RegisterCallback(ButtonCommandCallback_t cb);

/**
 * @brief Cập nhật trạng thái phát hiện tại từ Pi (để biết nút bấm nên gửi Play hay Pause)
 */
void Button_UpdatePlaybackState(PlaybackState_t state);

/**
 * @brief Hàm quét nút định kỳ non-blocking (gọi trong vòng lặp chính main loop)
 * @param currentTickMs Giá trị HAL_GetTick()
 */
void Button_Scan(uint32_t currentTickMs);

#ifdef __cplusplus
}
#endif

#endif /* BUTTON_MANAGER_H */
