/**
 * @file  metadata.c
 * @brief Persistent metadata in Flash sector 4.
 */

#include "metadata.h"
#include "flash_map.h"
#include "flash_if.h"
#include "crc32.h"
#include <string.h>

static uint32_t meta_crc(const metadata_t *m)
{
    const uint32_t len = (uint32_t)((const uint8_t *)&m->crc32 - (const uint8_t *)m);
    return crc32_calc((const uint8_t *)m, len);
}

bool metadata_load(metadata_t *out)
{
    const metadata_t *nv = (const metadata_t *)META_FLASH_BASE;

    if (nv->magic != META_MAGIC ||
        nv->struct_version != META_STRUCT_VERSION ||
        nv->crc32 != meta_crc(nv)) {
        metadata_reset(out);
        return false;
    }
    memcpy(out, nv, sizeof(*out));
    return true;
}

bool metadata_save(const metadata_t *m)
{
    metadata_t tmp;
    memcpy(&tmp, m, sizeof(tmp));
    tmp.magic          = META_MAGIC;
    tmp.struct_version = META_STRUCT_VERSION;
    tmp.crc32          = meta_crc(&tmp);

    if (!flash_if_erase_sectors(META_FLASH_SECTOR, 1u)) {
        return false;
    }
    return flash_if_write(META_FLASH_BASE, (const uint8_t *)&tmp, sizeof(tmp));
}

void metadata_reset(metadata_t *m)
{
    memset(m, 0, sizeof(*m));
    m->magic          = META_MAGIC;
    m->struct_version = META_STRUCT_VERSION;
}

void metadata_bitmap_set(metadata_t *m, uint32_t block_index)
{
    if (block_index < META_MAX_BLOCKS) {
        m->block_bitmap[block_index / 8u] |= (1u << (block_index % 8u));
    }
}

bool metadata_bitmap_get(const metadata_t *m, uint32_t block_index)
{
    if (block_index >= META_MAX_BLOCKS) {
        return false;
    }
    return (m->block_bitmap[block_index / 8u] & (1u << (block_index % 8u))) != 0u;
}

bool metadata_all_blocks_received(const metadata_t *m)
{
    if (m->block_count == 0u) {
        return false;
    }
    return m->received_block_count >= m->block_count;
}
