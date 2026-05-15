/**
 * @file  boot_state.h
 * @brief Bootloader state machine states and error codes.
 */
#ifndef BOOT_STATE_H
#define BOOT_STATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BOOT_START          = 0,
    BOOT_CHECK_ENTRY    = 1,
    BOOT_WAIT_COMMAND   = 2,
    BOOT_PREPARE_UPDATE = 3,
    BOOT_RECEIVE_FW     = 4,
    BOOT_VERIFY_STAGING = 5,
    BOOT_INSTALL_FW     = 6,
    BOOT_VERIFY_APP     = 7,
    BOOT_READY_TO_BOOT  = 8,
    BOOT_ERROR          = 9,
} boot_state_t;

typedef enum {
    BOOT_ERR_NONE                = 0,
    BOOT_ERR_PRODUCT_MISMATCH   = 1,
    BOOT_ERR_HW_REV_MISMATCH   = 2,
    BOOT_ERR_IMAGE_TOO_LARGE   = 3,
    BOOT_ERR_BLOCK_CRC          = 4,
    BOOT_ERR_IMAGE_CRC          = 5,
    BOOT_ERR_FLASH_ERASE        = 6,
    BOOT_ERR_FLASH_WRITE        = 7,
    BOOT_ERR_APP_VALIDATE       = 8,
    BOOT_ERR_UPDATE_TIMEOUT     = 9,
    BOOT_ERR_BLOCK_INDEX        = 10,
    BOOT_ERR_BAD_PARAMS         = 11,
} boot_error_t;

#ifdef __cplusplus
}
#endif

#endif /* BOOT_STATE_H */
