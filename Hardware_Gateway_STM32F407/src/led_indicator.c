/**
 * @file led_indicator.c
 * @brief Cài đặt module điều khiển LED trạng thái
 */

#include "led_indicator.h"

static uint32_t s_txBlinkStartMs = 0;
static uint8_t  s_txBlinkPhase   = 0; /* 0: idle, 1: single pulse ON, 2: double pulse 1 ON, 3: double pulse pause, 4: double pulse 2 ON */

void LED_Init(void) {
    LED_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = LED_PIN_GREEN | LED_PIN_ORANGE | LED_PIN_RED | LED_PIN_BLUE;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);

    // Ban đầu tắt toàn bộ LED
    HAL_GPIO_WritePin(LED_PORT, LED_PIN_GREEN | LED_PIN_ORANGE | LED_PIN_RED | LED_PIN_BLUE, GPIO_PIN_RESET);

    // Chớp test khởi động một vòng 4 LED để xác nhận phần cứng hoạt động tốt
    for (int i = 0; i < 2; i++) {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN_GREEN | LED_PIN_ORANGE | LED_PIN_RED | LED_PIN_BLUE, GPIO_PIN_SET);
        HAL_Delay(80);
        HAL_GPIO_WritePin(LED_PORT, LED_PIN_GREEN | LED_PIN_ORANGE | LED_PIN_RED | LED_PIN_BLUE, GPIO_PIN_RESET);
        HAL_Delay(80);
    }
}

void LED_SetButtonPressed(bool isPressed) {
    // LED Orange (LD3) bật sáng khi nút bấm đang được nhấn giữ
    HAL_GPIO_WritePin(LED_PORT, LED_PIN_ORANGE, isPressed ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void LED_TriggerTxBlink(void) {
    // Chớp 1 nhịp (cho Single Click)
    HAL_GPIO_WritePin(LED_PORT, LED_PIN_GREEN, GPIO_PIN_SET);
    s_txBlinkStartMs = HAL_GetTick();
    s_txBlinkPhase = 1;
}

void LED_TriggerTxDoubleBlink(void) {
    // Chớp 2 nhịp nhanh (cho Double Click)
    HAL_GPIO_WritePin(LED_PORT, LED_PIN_GREEN, GPIO_PIN_SET);
    s_txBlinkStartMs = HAL_GetTick();
    s_txBlinkPhase = 2;
}

void LED_SetError(bool isError) {
    // LED Red (LD5)
    HAL_GPIO_WritePin(LED_PORT, LED_PIN_RED, isError ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void LED_SetPlaybackState(PlaybackState_t state) {
    // LED Blue (LD6): Sáng nếu Pi đang PLAY, tắt nếu STOPPED/PAUSED
    if (state == PLAYBACK_STATE_PLAYING) {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN_BLUE, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(LED_PORT, LED_PIN_BLUE, GPIO_PIN_RESET);
    }
}

void LED_Update(uint32_t currentTickMs) {
    // Máy trạng thái điều khiển nhịp chớp LED xanh non-blocking
    if (s_txBlinkPhase == 1) {
        // Nhịp đơn: tắt sau 60ms
        if ((currentTickMs - s_txBlinkStartMs) >= LED_TX_BLINK_MS) {
            HAL_GPIO_WritePin(LED_PORT, LED_PIN_GREEN, GPIO_PIN_RESET);
            s_txBlinkPhase = 0;
        }
    } else if (s_txBlinkPhase == 2) {
        // Nhịp kép - Nhịp 1 ON 50ms
        if ((currentTickMs - s_txBlinkStartMs) >= 50) {
            HAL_GPIO_WritePin(LED_PORT, LED_PIN_GREEN, GPIO_PIN_RESET);
            s_txBlinkStartMs = currentTickMs;
            s_txBlinkPhase = 3; // chuyển sang pause giữa 2 nhịp
        }
    } else if (s_txBlinkPhase == 3) {
        // Nhịp kép - Pause OFF 50ms
        if ((currentTickMs - s_txBlinkStartMs) >= 50) {
            HAL_GPIO_WritePin(LED_PORT, LED_PIN_GREEN, GPIO_PIN_SET);
            s_txBlinkStartMs = currentTickMs;
            s_txBlinkPhase = 4; // nhịp 2 ON
        }
    } else if (s_txBlinkPhase == 4) {
        // Nhịp kép - Nhịp 2 ON 50ms
        if ((currentTickMs - s_txBlinkStartMs) >= 50) {
            HAL_GPIO_WritePin(LED_PORT, LED_PIN_GREEN, GPIO_PIN_RESET);
            s_txBlinkPhase = 0;
        }
    }
}
