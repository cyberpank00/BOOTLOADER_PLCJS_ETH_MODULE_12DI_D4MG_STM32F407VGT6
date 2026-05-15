/**
 * @file  fw_update_proto.h
 * @brief Firmware update protocol over Modbus TCP register interface.
 *
 * === Input Registers (FC 04, read-only, base 0x0000) ===
 *
 *  Addr   Regs  Description
 *  0x0000  2    Bootloader magic  (0xB00710AD)
 *  0x0002  2    Bootloader version
 *  0x0004  1    Boot state        (boot_state_t)
 *  0x0005  1    App valid         (0/1)
 *  0x0006  2    App FW version
 *  0x0008  2    Product ID
 *  0x000A  1    HW revision
 *  0x000B  1    Last error code   (boot_error_t)
 *  0x000C  2    Total block count
 *  0x000E  2    Received blocks
 *  0x0010  2    Image size
 *  0x0012  2    Image CRC32
 *  0x0014  1    Command status    (0=idle,1=busy,2=ok,3=err)
 *  0x0015  1    Staging valid     (0/1)
 *
 * === Holding Registers (FC 03/16, base 0x0000) ===
 *
 *  Addr   Regs  Description
 *  0x0000  1    Command register
 *  0x0010  2    Param: image_size
 *  0x0012  2    Param: image_crc32
 *  0x0014  2    Param: fw_version
 *  0x0016  2    Param: product_id
 *  0x0018  1    Param: hw_revision
 *  0x0019  1    Param: block_size
 *  0x001A  2    Param: block_count
 *
 *  0x0100  2    Write-block: block index
 *  0x0102  1    Write-block: data length (bytes)
 *  0x0103 ..    Write-block: data (up to 120 regs = 240 bytes)
 *
 * === Commands (written to HR 0x0000) ===
 *
 *  1  CMD_BEGIN_UPDATE    — prepare for new image (uses params 0x0010-0x001B)
 *  2  CMD_FINALIZE_UPDATE — verify staging completeness + CRC
 *  3  CMD_INSTALL_UPDATE  — copy staging → application
 *  4  CMD_ABORT_UPDATE    — cancel current session
 *  5  CMD_REBOOT          — software reset
 */
#ifndef FW_UPDATE_PROTO_H
#define FW_UPDATE_PROTO_H

#include <stdint.h>
#include "metadata.h"
#include "nanomodbus.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BOOT_MODBUS_MAGIC       0xB00710ADu

#define CMD_NONE                0u
#define CMD_BEGIN_UPDATE        1u
#define CMD_FINALIZE_UPDATE     2u
#define CMD_INSTALL_UPDATE      3u
#define CMD_ABORT_UPDATE        4u
#define CMD_REBOOT              5u

#define CMD_STATUS_IDLE         0u
#define CMD_STATUS_BUSY         1u
#define CMD_STATUS_OK           2u
#define CMD_STATUS_ERROR        3u

void     fw_proto_init(metadata_t *meta);
void     fw_proto_poll(metadata_t *meta);
bool     fw_proto_reboot_requested(void);
bool     fw_proto_install_requested(void);
void     fw_proto_install_done(void);
void     fw_proto_install_error(void);

nmbs_error fw_proto_read_input_regs(uint16_t addr, uint16_t qty,
                                    uint16_t *out);
nmbs_error fw_proto_read_holding_regs(uint16_t addr, uint16_t qty,
                                      uint16_t *out);
nmbs_error fw_proto_write_holding_regs(uint16_t addr, uint16_t qty,
                                       const uint16_t *in);

#ifdef __cplusplus
}
#endif

#endif /* FW_UPDATE_PROTO_H */
