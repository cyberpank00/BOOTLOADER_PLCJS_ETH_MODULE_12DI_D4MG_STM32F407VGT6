/**
 * @file  boot_jump.h
 * @brief Jump-to-application logic.
 */
#ifndef BOOT_JUMP_H
#define BOOT_JUMP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Deinitialize peripherals and jump to the application at @p app_base.
 * This function does not return.
 */
void boot_jump_to_app(uint32_t app_base) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif /* BOOT_JUMP_H */
