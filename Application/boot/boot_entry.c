/**
 * @file  boot_entry.c
 * @brief Boot entry decision logic.
 */

#include "boot_entry.h"
#include "flash_map.h"
#include "app_validate.h"
#include "stm32f4xx_hal.h"

/* FACT_RES button: PC6, active-low with internal pull-up. */
#define SERVICE_BTN_PORT    GPIOC
#define SERVICE_BTN_PIN     GPIO_PIN_6

bool boot_entry_button_pressed(void)
{
    return HAL_GPIO_ReadPin(SERVICE_BTN_PORT, SERVICE_BTN_PIN) == GPIO_PIN_RESET;
}

bool boot_entry_should_stay(const metadata_t *meta)
{
    /* 1. Service button held at startup */
    if (boot_entry_button_pressed()) {
        return true;
    }

    /* 2. Installation was interrupted — must finish or retry */
    if (meta->install_in_progress) {
        return true;
    }

    /* 3. Explicit request to enter bootloader */
    if (meta->install_requested) {
        return true;
    }

    /* 4. Firmware reception was in progress */
    if (meta->boot_state == BOOT_RECEIVE_FW ||
        meta->boot_state == BOOT_PREPARE_UPDATE) {
        return true;
    }

    /* 5. Application not marked valid */
    if (!meta->app_valid) {
        return true;
    }

    /* 6. Quick sanity check on vectors */
    if (!app_validate_vectors(APP_FLASH_BASE)) {
        return true;
    }

    return false;
}
