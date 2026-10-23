/**
 * @file can_driver.c
 * @brief Cài đặt ngoại vi phần cứng bxCAN cho STM32F407VGT6
 */

#include "can_driver.h"
#include "led_indicator.h"
#include <string.h>

/* Khai báo biến ngoại vi CAN1 */
CAN_HandleTypeDef hcan1;

static CanStatusReceivedCallback_t s_statusCallback = NULL;
static uint8_t                    s_rollingCounter = 0;
static bool                       s_isInitialized  = false;

/* ============================================================================
 * HAL MSP INIT: BẬT CLOCK, CẤU HÌNH GPIO CHÂN CAN VÀ NGẮT NVIC
 * ============================================================================ */
__weak void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan) {
    if (hcan->Instance == CAN1) {
        // 1. Kích hoạt xung nhịp cho CAN1 và Port GPIO tương ứng
        __HAL_RCC_CAN1_CLK_ENABLE();
        CAN1_GPIO_CLK_ENABLE();

        // 2. Cấu hình chân Alternate Function AF9 (CAN1)
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        GPIO_InitStruct.Pin = CAN1_RX_PIN | CAN1_TX_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_CAN1;
        HAL_GPIO_Init(CAN1_RX_PORT, &GPIO_InitStruct);

        // 3. Cấu hình ngắt nhận tin nhắn (RX0) và ngắt lỗi (SCE)
        HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);

        HAL_NVIC_SetPriority(CAN1_SCE_IRQn, 3, 0);
        HAL_NVIC_EnableIRQ(CAN1_SCE_IRQn);
    }
}

__weak void HAL_CAN_MspDeInit(CAN_HandleTypeDef *hcan) {
    if (hcan->Instance == CAN1) {
        __HAL_RCC_CAN1_CLK_DISABLE();
        HAL_GPIO_DeInit(CAN1_RX_PORT, CAN1_RX_PIN | CAN1_TX_PIN);
        HAL_NVIC_DisableIRQ(CAN1_RX0_IRQn);
        HAL_NVIC_DisableIRQ(CAN1_SCE_IRQn);
    }
}

/* ============================================================================
 * KHỞI TẠO NGOẠI VI CAN1 Ở TỐC ĐỘ 500 KBPS
 * ============================================================================ */
bool CAN_Driver_Init(void) {
    hcan1.Instance = CAN1;
    hcan1.Init.Prescaler = CAN_PRESCALER_VAL;
    hcan1.Init.Mode = CAN_MODE_NORMAL;
    hcan1.Init.SyncJumpWidth = CAN_SJW_VAL;
    hcan1.Init.TimeSeg1 = CAN_BS1_VAL;
    hcan1.Init.TimeSeg2 = CAN_BS2_VAL;
    hcan1.Init.TimeTriggeredMode = DISABLE;
    hcan1.Init.AutoBusOff = ENABLE;          // Tự động hồi phục sau bus-off
    hcan1.Init.AutoWakeUp = DISABLE;
    hcan1.Init.AutoRetransmission = ENABLE;  // Tự động truyền lại nếu NACK
    hcan1.Init.ReceiveFifoLocked = DISABLE;
    hcan1.Init.TransmitFifoPriority = DISABLE;

    if (HAL_CAN_Init(&hcan1) != HAL_OK) {
        LED_SetError(true);
        return false;
    }

    // Cấu hình Bộ Lọc CAN Filter Bank 0 (Chấp nhận ID 0x101 và toàn bộ tin nhắn hợp lệ)
    CAN_FilterTypeDef sFilterConfig = {0};
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;  // Chấp nhận tất cả khung tin để debug/nhận status
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = CAN_FILTER_ENABLE;
    sFilterConfig.SlaveStartFilterBank = 14;

    if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK) {
        LED_SetError(true);
        return false;
    }

    // Khởi động ngoại vi CAN1
    if (HAL_CAN_Start(&hcan1) != HAL_OK) {
        LED_SetError(true);
        return false;
    }

    // Kích hoạt ngắt nhận tin nhắn tại FIFO 0 và ngắt báo lỗi
    HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING | CAN_IT_ERROR | CAN_IT_BUSOFF);

    s_isInitialized = true;
    LED_SetError(false);
    return true;
}

/* ============================================================================
 * GỬI KHUNG TIN ĐIỀU KHIỂN (CAN ID 0x100)
 * ============================================================================ */
bool CAN_Driver_SendCommand(uint8_t cmdId, uint8_t seekCmd, uint8_t seekVal) {
    if (!s_isInitialized) return false;

    // Kiểm tra xem còn Mailbox truyền trống không
    if (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) == 0) {
        LED_SetError(true);
        return false;
    }

    CAN_TxHeaderTypeDef txHeader = {0};
    txHeader.StdId = CAN_ID_COMMAND;        // 0x100
    txHeader.ExtId = 0x00;
    txHeader.IDE   = CAN_ID_STD;            // Khung chuẩn 11-bit
    txHeader.RTR   = CAN_RTR_DATA;          // Khung dữ liệu
    txHeader.DLC   = CAN_DLC_STANDARD;      // 8 bytes
    txHeader.TransmitGlobalTime = DISABLE;

    uint8_t txData[8];
    // Đóng gói frame CAN kèm Rolling Counter (0..15) và Checksum
    CanProtocol_PackCommand(cmdId, seekCmd, seekVal, &s_rollingCounter, txData);

    uint32_t txMailbox = 0;
    HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox);

    if (status == HAL_OK) {
        // Chớp nháy LED xanh báo truyền thành công
        LED_TriggerTxBlink();
        LED_SetError(false);
        return true;
    } else {
        LED_SetError(true);
        return false;
    }
}

/* ============================================================================
 * ĐĂNG KÝ CALLBACK & KIỂM TRA TRẠNG THÁI SỨC KHỎE BUS
 * ============================================================================ */
void CAN_Driver_RegisterStatusCallback(CanStatusReceivedCallback_t cb) {
    s_statusCallback = cb;
}

void CAN_Driver_CheckBusHealth(void) {
    if (!s_isInitialized) return;

    uint32_t err = HAL_CAN_GetError(&hcan1);
    if (err != HAL_CAN_ERROR_NONE) {
        LED_SetError(true);
    } else {
        LED_SetError(false);
    }
}

/* ============================================================================
 * CALLBACK NGẮT NHẬN CAN (CAN_RX_FIFO0_MSG_PENDING)
 * ============================================================================ */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
    if (hcan->Instance == CAN1) {
        CAN_RxHeaderTypeDef rxHeader = {0};
        uint8_t rxData[8] = {0};

        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK) {
            // Nếu nhận đúng khung tin Status Telemetry ID 0x101 từ Target Raspberry Pi
            if (rxHeader.StdId == CAN_ID_STATUS && rxHeader.DLC == CAN_DLC_STANDARD) {
                CanStatusPacket_t statusPacket;
                if (CanProtocol_UnpackStatus(rxData, &statusPacket)) {
                    // Cập nhật LED xanh dương theo trạng thái Playback của Pi
                    LED_SetPlaybackState((PlaybackState_t)statusPacket.playbackState);

                    // Báo sang module nút bấm để cập nhật state
                    if (s_statusCallback) {
                        s_statusCallback(&statusPacket);
                    }
                }
            }
        }
    }
}

/* ============================================================================
 * CALLBACK BÁO LỖI CAN (CAN_ERROR_CALLBACK)
 * ============================================================================ */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan) {
    if (hcan->Instance == CAN1) {
        LED_SetError(true);
    }
}
