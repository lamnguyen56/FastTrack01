/**
 * @file main.h
 * @brief Header chính cho STM32F407 CAN Gateway Simulator
 */

#ifndef MAIN_H
#define MAIN_H

#include "stm32f4xx_hal.h"
#include "config.h"
#include "can_protocol.h"
#include "can_driver.h"
#include "button_manager.h"
#include "led_indicator.h"

#ifdef __cplusplus
extern "C" {
#endif

void SystemClock_Config(void);
void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */
