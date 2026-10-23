/**
 * @file stm32f4xx_it.c
 * @brief Định nghĩa các trình phục vụ ngắt ngoại vi (ISR)
 */

#include "stm32f4xx_it.h"
#include "stm32f4xx_hal.h"

extern CAN_HandleTypeDef hcan1;

void NMI_Handler(void) {}

void HardFault_Handler(void) {
    while (1) {}
}

void MemManage_Handler(void) {
    while (1) {}
}

void BusFault_Handler(void) {
    while (1) {}
}

void UsageFault_Handler(void) {
    while (1) {}
}

void SVC_Handler(void) {}

void DebugMon_Handler(void) {}

void PendSV_Handler(void) {}

void SysTick_Handler(void) {
    HAL_IncTick();
}

/**
 * @brief Ngắt nhận tin nhắn tại FIFO 0 của CAN1
 */
void CAN1_RX0_IRQHandler(void) {
    HAL_CAN_IRQHandler(&hcan1);
}

/**
 * @brief Ngắt cảnh báo lỗi / Bus-off của CAN1
 */
void CAN1_SCE_IRQHandler(void) {
    HAL_CAN_IRQHandler(&hcan1);
}
