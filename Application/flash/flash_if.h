/**
 * @file  flash_if.h
 * @brief Internal Flash erase / write / read driver.
 */
#ifndef FLASH_IF_H
#define FLASH_IF_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool flash_if_erase_sectors(uint32_t first_sector, uint32_t count);
bool flash_if_write(uint32_t dest_addr, const uint8_t *src, uint32_t len);
bool flash_if_read(uint32_t src_addr, uint8_t *dest, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* FLASH_IF_H */
