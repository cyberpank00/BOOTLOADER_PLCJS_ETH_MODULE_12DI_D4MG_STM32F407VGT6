# Required changes for the main application linker script

The main application (`PLCJS_ETH_MODULE_12DI_D4MG_STM32F407VGT6`) linker script
(`STM32F407XX_FLASH.ld`) must be updated to coexist with the bootloader.

## Flash memory layout

| Sector | Address      | Size    | Usage            |
|--------|------------- |---------|------------------|
| 0-4    | 0x08000000   | 128 KB  | Bootloader       |
| 5      | 0x08020000   | 128 KB  | Metadata         |
| 6-7    | 0x08040000   | 256 KB  | **Application**  |
| 8-9    | 0x08080000   | 256 KB  | Staging          |
| 10     | 0x080C0000   | 128 KB  | App Settings     |
| 11     | 0x080E0000   | 128 KB  | (reserved)       |

## Changes needed in `STM32F407XX_FLASH.ld`

```ld
/* Old: */
FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1024K

/* New: */
FLASH (rx) : ORIGIN = 0x08040000, LENGTH = 256K
```

## Changes needed in `system_stm32f4xx.c`

The `VECT_TAB_OFFSET` must be updated:

```c
/* Old: */
#define VECT_TAB_OFFSET  0x00

/* New: */
#define VECT_TAB_OFFSET  0x40000
```

## Application header

The main application binary must include a firmware header at offset `0x200`
(after the vector table) for the bootloader to validate it.
See `Application/validate/app_validate.h` in the bootloader for the
`fw_header_t` structure definition.

## Settings sector change

The Settings module currently uses **Sector 11** (`0x080E0000`).
In the new layout it moves to **Sector 10** (`0x080C0000`).
Update `SETTINGS_FLASH_SECTOR` and `SETTINGS_FLASH_BASE` in `settings.h`.
