/**
 * @file  flash_if.c
 * @brief Internal Flash erase / write / read driver for STM32F407.
 */

#include "flash_if.h"
#include "stm32f4xx_hal.h"
#include <string.h>

bool flash_if_erase_sectors(uint32_t first_sector, uint32_t count)
{
    if (HAL_FLASH_Unlock() != HAL_OK) {
        return false;
    }

    FLASH_EraseInitTypeDef erase = {
        .TypeErase    = FLASH_TYPEERASE_SECTORS,
        .Banks        = FLASH_BANK_1,
        .Sector       = first_sector,
        .NbSectors    = count,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3,
    };
    uint32_t sector_error = 0;

    HAL_StatusTypeDef rc = HAL_FLASHEx_Erase(&erase, &sector_error);
    HAL_FLASH_Lock();
    return rc == HAL_OK;
}

bool flash_if_write(uint32_t dest_addr, const uint8_t *src, uint32_t len)
{
    if (HAL_FLASH_Unlock() != HAL_OK) {
        return false;
    }

    /* Program word-by-word (32-bit). Pad the last partial word if needed. */
    uint32_t full_words = len / 4u;
    uint32_t tail       = len % 4u;
    uint32_t addr       = dest_addr;
    const uint32_t *w   = (const uint32_t *)(const void *)src;

    for (uint32_t i = 0; i < full_words; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, w[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return false;
        }
        addr += 4u;
    }

    if (tail > 0u) {
        uint32_t last = 0xFFFFFFFFu;
        memcpy(&last, src + full_words * 4u, tail);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, last) != HAL_OK) {
            HAL_FLASH_Lock();
            return false;
        }
    }

    HAL_FLASH_Lock();
    return true;
}

bool flash_if_read(uint32_t src_addr, uint8_t *dest, uint32_t len)
{
    memcpy(dest, (const void *)src_addr, len);
    return true;
}
