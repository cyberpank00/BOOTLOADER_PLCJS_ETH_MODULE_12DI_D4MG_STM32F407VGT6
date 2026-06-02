/**
 * @file  modbus_boot_server.h
 * @brief Bare-metal Modbus TCP server using LwIP raw API + nanoMODBUS.
 */
#ifndef MODBUS_BOOT_SERVER_H
#define MODBUS_BOOT_SERVER_H

#include <stdbool.h>
#include <stdint.h>
#include "metadata.h"

#ifdef __cplusplus
extern "C" {
#endif

void modbus_boot_server_init(metadata_t *meta, uint16_t port);
void modbus_boot_server_poll(void);

#ifdef __cplusplus
}
#endif

#endif /* MODBUS_BOOT_SERVER_H */
