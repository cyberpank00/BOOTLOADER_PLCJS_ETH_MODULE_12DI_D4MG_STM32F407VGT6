/**
 * @file  crc32.c
 * @brief Software CRC32 (IEEE 802.3, polynomial 0xEDB88320).
 */

#include "crc32.h"

uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t len)
{
    crc = ~crc;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint32_t k = 0; k < 8; k++) {
            crc = (crc >> 1) ^ (0xEDB88320u & (-(int32_t)(crc & 1u)));
        }
    }
    return ~crc;
}

uint32_t crc32_calc(const uint8_t *data, uint32_t len)
{
    return crc32_update(0u, data, len);
}
