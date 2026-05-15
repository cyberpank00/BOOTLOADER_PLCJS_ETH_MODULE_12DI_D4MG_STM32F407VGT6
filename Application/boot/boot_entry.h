/**
 * @file  boot_entry.h
 * @brief Boot entry decision logic.
 */
#ifndef BOOT_ENTRY_H
#define BOOT_ENTRY_H

#include <stdbool.h>
#include "metadata.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Decide whether the bootloader should remain active.
 *
 * Returns true (stay in bootloader) when at least one of:
 *   1. Service button pressed at startup
 *   2. Application image absent or invalid
 *   3. Metadata flag: enter-bootloader requested
 *   4. Install was interrupted and needs completion
 *
 * Returns false if the application is valid and no service mode requested.
 */
bool boot_entry_should_stay(const metadata_t *meta);

/** Check if the physical service button is pressed (active-low on PC8). */
bool boot_entry_button_pressed(void);

#ifdef __cplusplus
}
#endif

#endif /* BOOT_ENTRY_H */
