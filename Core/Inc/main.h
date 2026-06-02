/**
 * @file  main.h
 * @brief Bootloader main header — GPIO defines and prototypes.
 */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

void Error_Handler(void);
void SystemClock_Config(void);

/* GPIO pin aliases (matching main application) */
#define ETHINT_Pin          GPIO_PIN_1
#define ETHINT_GPIO_Port    GPIOB

#define ETHRST_Pin          GPIO_PIN_11
#define ETHRST_GPIO_Port    GPIOD

#define STAT_LED_Pin        GPIO_PIN_8
#define STAT_LED_GPIO_Port  GPIOC

#define FACT_RES_Pin        GPIO_PIN_6
#define FACT_RES_GPIO_Port  GPIOC

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
