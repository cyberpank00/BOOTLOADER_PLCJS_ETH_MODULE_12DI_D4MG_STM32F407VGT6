/**
 * @file  gpio.c
 * @brief Bootloader GPIO init — only LED, button, and Ethernet-reset pins.
 */

#include "gpio.h"
#include "main.h"

void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gi = {0};

    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /* ETHRST (PD11) — output, start low */
    HAL_GPIO_WritePin(ETHRST_GPIO_Port, ETHRST_Pin, GPIO_PIN_RESET);
    gi.Pin   = ETHRST_Pin;
    gi.Mode  = GPIO_MODE_OUTPUT_PP;
    gi.Pull  = GPIO_NOPULL;
    gi.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(ETHRST_GPIO_Port, &gi);

    /* STAT_LED (PC6) — output */
    HAL_GPIO_WritePin(STAT_LED_GPIO_Port, STAT_LED_Pin, GPIO_PIN_RESET);
    gi.Pin   = STAT_LED_Pin;
    gi.Mode  = GPIO_MODE_OUTPUT_PP;
    gi.Pull  = GPIO_NOPULL;
    gi.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STAT_LED_GPIO_Port, &gi);

    /* FACT_RES / service button (PC8) — input with pull-up */
    gi.Pin  = FACT_RES_Pin;
    gi.Mode = GPIO_MODE_INPUT;
    gi.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(FACT_RES_GPIO_Port, &gi);

    /* ETHINT (PB1) — input, active interrupt from PHY */
    gi.Pin  = ETHINT_Pin;
    gi.Mode = GPIO_MODE_INPUT;
    gi.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ETHINT_GPIO_Port, &gi);
}
