/**
 * @file  crc32.h
 * @brief Software CRC32 (IEEE 802.3) for firmware integrity checks.
 */
#ifndef CRC32_H
#define CRC32_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint32_t crc32_calc(const uint8_t *data, uint32_t len);
uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* CRC32_H */
