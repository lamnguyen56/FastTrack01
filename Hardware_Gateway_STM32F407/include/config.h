/**
 * @file config.h
 * @brief Cấu hình phần cứng, GPIO, CAN bit timing cho STM32F407VGT6 Gateway
 * @project FastTrack01 - USB Video Player Controlled via CAN
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * 1. CẤU HÌNH XUNG CLOCK & CAN BIT TIMING (500 KBPS)
 * ============================================================================
 * Với thạch anh ngoài HSE = 8 MHz:
 * - SYSCLK = 168 MHz
 * - HCLK   = 168 MHz
 * - APB1 (PCLK1) = 42 MHz (Ngoại vi CAN1 gắn trên bus APB1)
 *
 * Tính toán CAN Bit Timing cho 500 kbps:
 *   Bit Time = 1 / 500.000 = 2 us
 *   Tq = Prescaler / PCLK1 = 6 / 42.000.000 = 142.857 ns
 *   Số lượng Time Quanta (Nq) = 1 (Sync) + 11 (BS1) + 2 (BS2) = 14 Tq
 *   Tbit = 14 * Tq = 14 * (6 / 42 MHz) = 2.0 us -> Đúng chuẩn 500 kbps!
 *   Sample Point = (1 + 11) / 14 = 85.7% (Đúng tiêu chuẩn Automotive CAN)
 * ============================================================================ */
#define CAN_PRESCALER_VAL       6
#define CAN_SJW_VAL             CAN_SJW_1TQ
#define CAN_BS1_VAL             CAN_BS1_11TQ
#define CAN_BS2_VAL             CAN_BS2_2TQ

/* ============================================================================
 * 2. CẤU HÌNH CHÂN GPIO NGOẠI VI CAN1
 * ============================================================================
 * Chọn 1 trong 2 cặp chân phổ biến trên STM32F407VGT6:
 * Cặp 1 (Mặc định): PB8 (RX) / PB9 (TX) - Rất phổ biến, tránh xung đột chip audio
 * Cặp 2: PD0 (RX) / PD1 (TX) - Chân nằm trên hàng header Discovery
 * ============================================================================ */
#define USE_CAN1_PB8_PB9        1   /* Đặt 1 để dùng PB8/PB9, đặt 0 để dùng PD0/PD1 */

#if USE_CAN1_PB8_PB9
    #define CAN1_RX_PORT        GPIOB
    #define CAN1_RX_PIN         GPIO_PIN_8
    #define CAN1_TX_PORT        GPIOB
    #define CAN1_TX_PIN         GPIO_PIN_9
    #define CAN1_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOB_CLK_ENABLE()
#else
    #define CAN1_RX_PORT        GPIOD
    #define CAN1_RX_PIN         GPIO_PIN_0
    #define CAN1_TX_PORT        GPIOD
    #define CAN1_TX_PIN         GPIO_PIN_1
    #define CAN1_GPIO_CLK_ENABLE()   __HAL_RCC_GPIOD_CLK_ENABLE()
#endif

/* ============================================================================
 * 3. CẤU HÌNH CỤM 3 NÚT BẤM ĐIỀU KHIỂN (3-BUTTON SINGLE / DOUBLE CLICK)
 * ============================================================================
 * - NÚT 1: PLAY / PAUSE (Chân PE0 và đồng bộ nút Onboard PA0)
 *   + Nhấn 1 lần: Đảo trạng thái Play <-> Pause (0x01 <-> 0x02)
 *
 * - NÚT 2: NEXT / PREVIOUS (Chân PE1)
 *   + Nhấn 1 lần: NEXT TRACK (0x03)
 *   + Nhấn 2 lần: PREVIOUS TRACK (0x04)
 *   + Rào chặn chống spam: Bỏ qua hoàn toàn các lần nhấn thứ 3, 4
 *
 * - NÚT 3: TUA NHANH +10S / -10S (Chân PE2)
 *   + Nhấn 1 lần: TUA TIẾN +10s (Fast Forward: SeekCmd=0x05, Step=10)
 *   + Nhấn 2 lần: TUA LÙI -10s  (Fast Backward: SeekCmd=0x06, Step=10)
 *   + Rào chặn chống spam: Bỏ qua hoàn toàn các lần nhấn thứ 3, 4
 * ============================================================================ */
/* Nút Onboard PA0 (User Button - Active HIGH) */
#define BTN_USER_PORT           GPIOA
#define BTN_USER_PIN            GPIO_PIN_0
#define BTN_USER_ACTIVE_STATE   GPIO_PIN_SET    /* Active HIGH */
#define BTN_USER_CLK_ENABLE()   __HAL_RCC_GPIOA_CLK_ENABLE()

/* Cụm 3 nút cứng ngoại vi (Port E - Active LOW với Pull-up nội) */
#define BTN_CLUSTER_PORT        GPIOE
#define BTN_CLUSTER_CLK_ENABLE() __HAL_RCC_GPIOE_CLK_ENABLE()
#define BTN_PIN_PLAY_PAUSE      GPIO_PIN_0      /* Nút 1: Play / Pause */
#define BTN_PIN_NEXT_PREV       GPIO_PIN_1      /* Nút 2: Next (1 lần) / Prev (2 lần) */
#define BTN_PIN_SEEK            GPIO_PIN_2      /* Nút 3: +10s (1 lần) / -10s (2 lần) */

/* Tham số thời gian máy trạng thái nút bấm (FSM Timings) */
#define BUTTON_DEBOUNCE_MS          50          /* Lọc rung phím cơ học: 50ms */
#define BUTTON_DOUBLE_CLICK_MS      350         /* Cửa sổ chờ cú click thứ 2: 350ms */
#define BUTTON_COOLDOWN_MS          200         /* Thời gian khóa (cooldown) chặn click thứ 3, 4: 200ms */
#define BUTTON_LONG_HOLD_THRESHOLD  500         /* Giữ phím > 500ms thì nhả ra gửi ngay Single Click */

/* ============================================================================
 * 4. CẤU HÌNH ĐÈN LED CHỈ BÁO TRẠNG THÁI (LED STATUS)
 * ============================================================================
 * Sử dụng 4 LED có sẵn trên STM32F4DISCOVERY (Port GPIOD):
 * - LD4 (Green - PD12):  LED TX Activity (Nhấp nháy khi gửi frame CAN thành công)
 * - LD3 (Orange - PD13): LED Button Status (Bật sáng khi nút đang được ấn)
 * - LD5 (Red - PD14):    LED Error (Bật sáng khi lỗi bus CAN / gửi thất bại)
 * - LD6 (Blue - PD15):   LED Playback State (Sáng khi Target Raspberry Pi đang Play)
 * ============================================================================ */
#define LED_PORT                GPIOD
#define LED_CLK_ENABLE()        __HAL_RCC_GPIOD_CLK_ENABLE()

#define LED_PIN_GREEN           GPIO_PIN_12     /* LD4: CAN TX Activity */
#define LED_PIN_ORANGE          GPIO_PIN_13     /* LD3: Button Pressed Indicator */
#define LED_PIN_RED             GPIO_PIN_14     /* LD5: CAN Bus Error */
#define LED_PIN_BLUE            GPIO_PIN_15     /* LD6: Playback State / Telemetry Heartbeat */

#define LED_TX_BLINK_MS         60              /* Độ dài chớp nháy của LED TX (ms) */

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
