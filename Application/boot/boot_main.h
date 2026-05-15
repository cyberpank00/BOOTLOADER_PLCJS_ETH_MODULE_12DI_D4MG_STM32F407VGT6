/**
 * @file  boot_main.h
 * @brief Bootloader main state machine and loop.
 */
#ifndef BOOT_MAIN_H
#define BOOT_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Bootloader entry point.  Called from main() after HAL and GPIO init.
 * This function never returns — it either loops servicing the network
 * or jumps to the application.
 */
void boot_run(void) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif /* BOOT_MAIN_H */
