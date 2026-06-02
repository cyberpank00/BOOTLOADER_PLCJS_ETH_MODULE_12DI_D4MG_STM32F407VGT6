/**
 * @file  boot_jump.c
 * @brief Deinitialize peripherals and transfer control to the application.
 */

#include "boot_jump.h"
#include "stm32f4xx_hal.h"

typedef void (*app_entry_t)(void);

void boot_jump_to_app(uint32_t app_base)
{
    const uint32_t *vectors = (const uint32_t *)app_base;
    uint32_t msp            = vectors[0];
    uint32_t reset_handler  = vectors[1];

    /* 1. Stop SysTick */
    SysTick->CTRL = 0u;
    SysTick->LOAD = 0u;
    SysTick->VAL  = 0u;

    /* 2. Disable all interrupts and clear pending bits */
    for (uint32_t i = 0; i < 8u; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFu;
        NVIC->ICPR[i] = 0xFFFFFFFFu;
    }

    /* 3. Deinitialize HAL (resets peripherals) */
    HAL_RCC_DeInit();
    HAL_DeInit();

    /* 4. Relocate the vector table */
    SCB->VTOR = app_base;

    /* 5. Set Main Stack Pointer */
    __set_MSP(msp);

    /* 6. Ensure all memory accesses complete before branching */
    __DSB();
    __ISB();

    /* 7. Jump to application Reset_Handler */
    app_entry_t app_entry = (app_entry_t)reset_handler;
    app_entry();

    /* Should never reach here */
    for (;;) {}
}
