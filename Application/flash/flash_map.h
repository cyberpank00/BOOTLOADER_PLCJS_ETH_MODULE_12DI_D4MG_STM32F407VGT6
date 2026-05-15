/**
 * @file  flash_map.h
 * @brief Internal Flash memory layout for bootloader + application.
 *
 * STM32F407VGT6 — 1 MB internal Flash, 12 sectors.
 *
 * Sector  Address       Size   Usage
 * ------  ----------    -----  ----------------------
 *  0      0x0800 0000   16 KB  Bootloader
 *  1      0x0800 4000   16 KB  Bootloader
 *  2      0x0800 8000   16 KB  Bootloader
 *  3      0x0800 C000   16 KB  Bootloader
 *  4      0x0801 0000   64 KB  Bootloader
 *  5      0x0802 0000  128 KB  Metadata (update state)
 *  6      0x0804 0000  128 KB  Application
 *  7      0x0806 0000  128 KB  Application
 *  8      0x0808 0000  128 KB  Staging
 *  9      0x080A 0000  128 KB  Staging
 * 10      0x080C 0000  128 KB  App Settings (NVM)
 * 11      0x080E 0000  128 KB  Staging (3rd sector)
 *
 * Application: 256 KB (sectors 6-7)
 * Staging:     384 KB (sectors 8-9, 11)
 *              NOTE: staging sectors are non-contiguous (8,9 + 11).
 *              For simplicity we use only sectors 8-9 (256 KB) as
 *              contiguous staging.  Max firmware image = 256 KB.
 */
#ifndef FLASH_MAP_H
#define FLASH_MAP_H

#include <stdint.h>

/* ---- Bootloader region (sectors 0-4, 128 KB) ---------------------------- */
#define BOOT_FLASH_BASE         0x08000000u
#define BOOT_FLASH_SIZE         (128u * 1024u)
#define BOOT_FLASH_END          (BOOT_FLASH_BASE + BOOT_FLASH_SIZE)

/* ---- Metadata region (sector 5, 128 KB) --------------------------------- */
#define META_FLASH_SECTOR       5u
#define META_FLASH_BASE         0x08020000u
#define META_FLASH_SIZE         (128u * 1024u)

/* ---- Application region (sectors 6-7, 256 KB) --------------------------- */
#define APP_FLASH_BASE          0x08040000u
#define APP_FLASH_SIZE          (256u * 1024u)
#define APP_FLASH_END           (APP_FLASH_BASE + APP_FLASH_SIZE)
#define APP_FIRST_SECTOR        6u
#define APP_LAST_SECTOR         7u

/* ---- Staging region (sectors 8-9, 256 KB) ------------------------------- */
#define STAGING_FLASH_BASE      0x08080000u
#define STAGING_FLASH_SIZE      (256u * 1024u)
#define STAGING_FLASH_END       (STAGING_FLASH_BASE + STAGING_FLASH_SIZE)
#define STAGING_FIRST_SECTOR    8u
#define STAGING_LAST_SECTOR     9u

/* ---- App Settings (sector 10, 128 KB — used by main app) ---------------- */
#define SETTINGS_FLASH_SECTOR   10u
#define SETTINGS_FLASH_BASE     0x080C0000u
#define SETTINGS_FLASH_SIZE     (128u * 1024u)

/* ---- RAM ---------------------------------------------------------------- */
#define RAM_BASE                0x20000000u
#define RAM_SIZE                (128u * 1024u)

/* ---- Firmware image header ---------------------------------------------- */
#define FW_IMAGE_MAGIC          0x504C434Au  /* "PLCJ" */
#define PRODUCT_ID_DEFAULT      0x12D1D4A0u
#define HW_REVISION_DEFAULT     1u
#define BOOTLOADER_VERSION      0x00010000u  /* 1.0.0 */

/* Maximum firmware block size for Modbus transfer (bytes). */
#define FW_MAX_BLOCK_SIZE       240u

#endif /* FLASH_MAP_H */
