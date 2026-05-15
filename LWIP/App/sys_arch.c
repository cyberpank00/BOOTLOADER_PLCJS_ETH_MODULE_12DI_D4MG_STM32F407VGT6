/**
 * @file  sys_arch.c
 * @brief Minimal sys_arch for LwIP NO_SYS = 1 mode.
 */

#include "lwip/opt.h"
#include "stm32f4xx_hal.h"

#if NO_SYS

u32_t sys_now(void)
{
    return HAL_GetTick();
}

#endif /* NO_SYS */
