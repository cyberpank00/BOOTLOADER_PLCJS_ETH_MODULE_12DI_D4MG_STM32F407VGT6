/**
 * @file  led_indication.c
 * @brief STAT_LED (PC6) patterns for bootloader states.
 */

#include "led_indication.h"
#include "stm32f4xx_hal.h"

#define LED_PORT    GPIOC
#define LED_PIN     GPIO_PIN_6

static led_pattern_t s_pattern = LED_PATTERN_OFF;
static uint32_t      s_last_toggle;
static uint8_t       s_burst_count;

void led_indication_init(void)
{
    s_pattern     = LED_PATTERN_OFF;
    s_last_toggle = 0;
    s_burst_count = 0;
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
}

void led_indication_set(led_pattern_t pattern)
{
    if (pattern != s_pattern) {
        s_pattern     = pattern;
        s_burst_count = 0;
        s_last_toggle = HAL_GetTick();

        if (pattern == LED_PATTERN_ERROR) {
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
        } else if (pattern == LED_PATTERN_OFF) {
            HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);
        }
    }
}

void led_indication_poll(uint32_t now_ms)
{
    uint32_t elapsed = now_ms - s_last_toggle;

    switch (s_pattern) {
    case LED_PATTERN_IDLE:
        if (elapsed >= 500u) {
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
            s_last_toggle = now_ms;
        }
        break;

    case LED_PATTERN_RECEIVING:
        if (elapsed >= 100u) {
            HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
            s_last_toggle = now_ms;
        }
        break;

    case LED_PATTERN_INSTALLING:
        if (s_burst_count < 6u) {
            if (elapsed >= 80u) {
                HAL_GPIO_TogglePin(LED_PORT, LED_PIN);
                s_last_toggle = now_ms;
                s_burst_count++;
            }
        } else {
            if (elapsed >= 600u) {
                s_burst_count = 0;
                s_last_toggle = now_ms;
            }
        }
        break;

    case LED_PATTERN_ERROR:
    case LED_PATTERN_OFF:
    default:
        break;
    }
}
