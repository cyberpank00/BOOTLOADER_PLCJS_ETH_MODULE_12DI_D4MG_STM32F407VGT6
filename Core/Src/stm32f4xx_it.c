/**
 * @file  stm32f4xx_it.c
 * @brief Bootloader interrupt handlers (bare-metal, no RTOS).
 */

#include "stm32f4xx_it.h"
#include "stm32f4xx_hal.h"

extern ETH_HandleTypeDef heth;

void NMI_Handler(void)         { for (;;) {} }
void HardFault_Handler(void)   { for (;;) {} }
void MemManage_Handler(void)   { for (;;) {} }
void BusFault_Handler(void)    { for (;;) {} }
void UsageFault_Handler(void)  { for (;;) {} }
void SVC_Handler(void)         { }
void DebugMon_Handler(void)    { }
void PendSV_Handler(void)      { }

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void ETH_IRQHandler(void)
{
    HAL_ETH_IRQHandler(&heth);
}
