/**
 * @file  app_validate.h
 * @brief Application image validation.
 */
#ifndef APP_VALIDATE_H
#define APP_VALIDATE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Firmware image header.  Must be placed at a known offset inside the
 * application image so that the bootloader can locate it.
 *
 * In the default layout the header lives at APP_FLASH_BASE + 0x200
 * (after the 512-byte vector table).
 */
#define FW_HEADER_OFFSET    0x200u

typedef struct __attribute__((packed)) {
    uint32_t magic;              /* FW_IMAGE_MAGIC                       */
    uint32_t product_id;
    uint16_t hw_revision;
    uint16_t reserved0;
    uint32_t fw_version;
    uint32_t image_size;         /* total image bytes (incl. header)     */
    uint32_t image_crc32;        /* CRC32 of the raw binary image       */
    uint32_t vector_table_offset;/* usually 0                            */
    uint32_t reserved1[2];
} fw_header_t;

/** Quick check: MSP and Reset_Handler point to valid memory ranges. */
bool app_validate_vectors(uint32_t app_base);

/** Full validation: vectors + header + CRC32 of the whole image. */
bool app_validate_full(uint32_t app_base, uint32_t expected_size,
                       uint32_t expected_crc);

/** Read the firmware header from the given base address. */
bool app_read_header(uint32_t app_base, fw_header_t *hdr);

#ifdef __cplusplus
}
#endif

#endif /* APP_VALIDATE_H */
