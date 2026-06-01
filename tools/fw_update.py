#!/usr/bin/env python3
"""
fw_update.py — Bootloader OTA client for PLCJS ETH MODULE (STM32F407VGT6)
Communicates via Modbus TCP (port 502) using the bootloader register protocol.

Usage:
    python fw_update.py status            # Read and print bootloader state
    python fw_update.py update <file.bin> # Upload firmware binary
    python fw_update.py abort             # Abort current update session
    python fw_update.py reboot            # Software reset

Requirements:
    pip install pymodbus>=3.0
"""

import argparse
import math
import struct
import sys
import time
import zlib

try:
    from pymodbus.client import ModbusTcpClient
except ImportError:
    print("ERROR: pymodbus not installed. Run:  pip install pymodbus")
    sys.exit(1)

# ---------------------------------------------------------------------------
# Target configuration
# ---------------------------------------------------------------------------
TARGET_IP   = "192.168.142.99"
TARGET_PORT = 502
UNIT_ID     = 1

PRODUCT_ID_DEFAULT  = 0x12D1D4A0
HW_REVISION_DEFAULT = 1

FW_MAX_BLOCK_SIZE   = 240          # Must match FW_MAX_BLOCK_SIZE in flash_map.h
STAGING_FLASH_SIZE  = 256 * 1024   # 256 KB

# ---------------------------------------------------------------------------
# Input Register map (FC04, base 0x0000) — read-only
# ---------------------------------------------------------------------------
IR_MAGIC_HI         = 0x0000   # 2 regs  — 0xB00710AD
IR_MAGIC_LO         = 0x0001
IR_VERSION_HI       = 0x0002   # 2 regs
IR_VERSION_LO       = 0x0003
IR_BOOT_STATE       = 0x0004   # 1 reg
IR_APP_VALID        = 0x0005   # 1 reg
IR_APP_VER_HI       = 0x0006   # 2 regs
IR_APP_VER_LO       = 0x0007
IR_PRODUCT_ID_HI    = 0x0008   # 2 regs
IR_PRODUCT_ID_LO    = 0x0009
IR_HW_REV           = 0x000A   # 1 reg
IR_LAST_ERROR       = 0x000B   # 1 reg
IR_BLOCK_COUNT_HI   = 0x000C   # 2 regs
IR_BLOCK_COUNT_LO   = 0x000D
IR_RECV_BLOCKS_HI   = 0x000E   # 2 regs
IR_RECV_BLOCKS_LO   = 0x000F
IR_IMAGE_SIZE_HI    = 0x0010   # 2 regs
IR_IMAGE_SIZE_LO    = 0x0011
IR_IMAGE_CRC_HI     = 0x0012   # 2 regs
IR_IMAGE_CRC_LO     = 0x0013
IR_CMD_STATUS       = 0x0014   # 1 reg  — 0=idle 1=busy 2=ok 3=err
IR_STAGING_VALID    = 0x0015   # 1 reg
IR_COUNT            = 0x0016   # total registers to read

# ---------------------------------------------------------------------------
# Holding Register map (FC03/FC16, base 0x0000) — read/write
# ---------------------------------------------------------------------------
HR_CMD          = 0x0000
HR_PARAM_BASE   = 0x0010   # image_size(2), crc32(2), fw_ver(2), prod_id(2),
                            # hw_rev(1), block_size(1), block_count(2)
HR_BLOCK_BASE   = 0x0100   # block_idx(2), data_len(1), data(up to 120 regs)

# Commands (written to HR_CMD)
CMD_BEGIN_UPDATE    = 1
CMD_FINALIZE_UPDATE = 2
CMD_INSTALL_UPDATE  = 3
CMD_ABORT_UPDATE    = 4
CMD_REBOOT          = 5

CMD_STATUS_IDLE  = 0
CMD_STATUS_BUSY  = 1
CMD_STATUS_OK    = 2
CMD_STATUS_ERROR = 3

# boot_state_t values
BOOT_STATE_NAMES = {
    0: "BOOT_START",
    1: "BOOT_CHECK_ENTRY",
    2: "BOOT_PREPARE_UPDATE",
    3: "BOOT_WAIT_COMMAND",
    4: "BOOT_RECEIVE_FW",
    5: "BOOT_VERIFY_STAGING",
    6: "BOOT_INSTALL_FW",
    7: "BOOT_VERIFY_APP",
    8: "BOOT_READY_TO_BOOT",
    9: "BOOT_ERROR",
}

ERROR_NAMES = {
    0: "NONE",
    1: "FLASH_ERASE",
    2: "FLASH_WRITE",
    3: "IMAGE_CRC",
    4: "IMAGE_TOO_LARGE",
    5: "BAD_PARAMS",
    6: "BLOCK_INDEX",
    7: "APP_VALIDATE",
    8: "PRODUCT_MISMATCH",
    9: "HW_REV_MISMATCH",
}

CMD_STATUS_NAMES = {0: "IDLE", 1: "BUSY", 2: "OK", 3: "ERROR"}

# ---------------------------------------------------------------------------
# CRC32 (IEEE 802.3 / zlib — matches crc32_calc() in the bootloader)
# ---------------------------------------------------------------------------
def crc32(data: bytes) -> int:
    return zlib.crc32(data) & 0xFFFFFFFF

# ---------------------------------------------------------------------------
# Modbus helpers
# ---------------------------------------------------------------------------
def u32_from_regs(regs, base: int) -> int:
    return (regs[base] << 16) | regs[base + 1]

def connect(ip: str = TARGET_IP, port: int = TARGET_PORT) -> ModbusTcpClient:
    c = ModbusTcpClient(ip, port=port, timeout=5)
    if not c.connect():
        print(f"ERROR: cannot connect to {ip}:{port}")
        sys.exit(1)
    return c

def read_input_regs(c: ModbusTcpClient) -> list:
    rr = c.read_input_registers(0, count=IR_COUNT, device_id=UNIT_ID)
    if rr.isError():
        print(f"ERROR reading input registers: {rr}")
        sys.exit(1)
    return rr.registers

def write_cmd(c: ModbusTcpClient, cmd: int):
    rr = c.write_register(HR_CMD, cmd, device_id=UNIT_ID)
    if rr.isError():
        print(f"ERROR writing command {cmd}: {rr}")
        sys.exit(1)

def write_regs(c: ModbusTcpClient, addr: int, values: list):
    rr = c.write_registers(addr, values, device_id=UNIT_ID)
    if rr.isError():
        print(f"ERROR writing registers at 0x{addr:04X}: {rr}")
        sys.exit(1)

def wait_for_status(c: ModbusTcpClient, expected: int, timeout_s: float = 30.0) -> bool:
    """Poll IR_CMD_STATUS until it matches expected or ERROR, or timeout."""
    t0 = time.time()
    while time.time() - t0 < timeout_s:
        regs = read_input_regs(c)
        st = regs[IR_CMD_STATUS]
        if st == expected:
            return True
        if st == CMD_STATUS_ERROR:
            err = regs[IR_LAST_ERROR]
            print(f"  Bootloader error: {ERROR_NAMES.get(err, str(err))} ({err})")
            return False
        time.sleep(0.05)
    print(f"  Timeout waiting for status {CMD_STATUS_NAMES.get(expected, str(expected))}")
    return False

# ---------------------------------------------------------------------------
# Pack firmware bytes into Modbus 16-bit register list.
# Modbus TCP sends registers big-endian (high byte first).
# nanoMODBUS receives them and regs_to_bytes() extracts high→byte[0], low→byte[1].
# So we pack straightforwardly: reg[i] = (byte[2i] << 8) | byte[2i+1].
# ---------------------------------------------------------------------------
def bytes_to_regs(data: bytes) -> list:
    if len(data) % 2:
        data = data + b'\xff'
    n = len(data) // 2
    return list(struct.unpack_from(f'>{n}H', data))

# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------
def cmd_status(args):
    c = connect(args.ip, args.port)
    regs = read_input_regs(c)
    magic = u32_from_regs(regs, IR_MAGIC_HI)
    if magic != 0xB00710AD:
        print(f"WARNING: unexpected magic 0x{magic:08X} (expected 0xB00710AD)")
    else:
        print(f"Magic:          0x{magic:08X}  OK")

    ver = u32_from_regs(regs, IR_VERSION_HI)
    print(f"BL version:     {ver >> 16}.{(ver >> 8) & 0xFF}.{ver & 0xFF}")

    state = regs[IR_BOOT_STATE]
    print(f"Boot state:     {BOOT_STATE_NAMES.get(state, str(state))} ({state})")
    print(f"App valid:      {regs[IR_APP_VALID]}")
    app_ver = u32_from_regs(regs, IR_APP_VER_HI)
    print(f"App version:    {app_ver >> 16}.{(app_ver >> 8) & 0xFF}.{app_ver & 0xFF}")
    prod_id = u32_from_regs(regs, IR_PRODUCT_ID_HI)
    print(f"Product ID:     0x{prod_id:08X}")
    print(f"HW revision:    {regs[IR_HW_REV]}")
    err = regs[IR_LAST_ERROR]
    print(f"Last error:     {ERROR_NAMES.get(err, str(err))} ({err})")
    block_count = u32_from_regs(regs, IR_BLOCK_COUNT_HI)
    recv_blocks = u32_from_regs(regs, IR_RECV_BLOCKS_HI)
    if block_count:
        print(f"Blocks:         {recv_blocks}/{block_count} received")
    image_size = u32_from_regs(regs, IR_IMAGE_SIZE_HI)
    image_crc  = u32_from_regs(regs, IR_IMAGE_CRC_HI)
    if image_size:
        print(f"Image size:     {image_size} bytes")
        print(f"Image CRC32:    0x{image_crc:08X}")
    st = regs[IR_CMD_STATUS]
    print(f"Cmd status:     {CMD_STATUS_NAMES.get(st, str(st))} ({st})")
    print(f"Staging valid:  {regs[IR_STAGING_VALID]}")
    c.close()


def cmd_abort(args):
    c = connect(args.ip, args.port)
    print("Sending CMD_ABORT_UPDATE...")
    write_cmd(c, CMD_ABORT_UPDATE)
    if wait_for_status(c, CMD_STATUS_OK, 5.0):
        print("Aborted OK")
    c.close()


def cmd_reboot(args):
    c = connect(args.ip, args.port)
    print("Sending CMD_REBOOT...")
    write_cmd(c, CMD_REBOOT)
    print("Device is rebooting.")
    c.close()


def cmd_update(args):
    # ---- Load binary -------------------------------------------------------
    try:
        with open(args.firmware, 'rb') as f:
            fw_data = f.read()
    except OSError as e:
        print(f"ERROR: cannot open {args.firmware}: {e}")
        sys.exit(1)

    image_size = len(fw_data)
    if image_size == 0 or image_size > STAGING_FLASH_SIZE:
        print(f"ERROR: image size {image_size} bytes is out of range (1..{STAGING_FLASH_SIZE})")
        sys.exit(1)

    image_crc = crc32(fw_data)
    block_size = FW_MAX_BLOCK_SIZE
    block_count = math.ceil(image_size / block_size)
    fw_version = args.version

    print(f"Firmware:    {args.firmware}")
    print(f"Size:        {image_size} bytes")
    print(f"CRC32:       0x{image_crc:08X}")
    print(f"Block size:  {block_size} bytes")
    print(f"Blocks:      {block_count}")
    print(f"FW version:  0x{fw_version:08X}")
    print()

    c = connect(args.ip, args.port)

    # ---- Verify bootloader is reachable ------------------------------------
    regs = read_input_regs(c)
    magic = u32_from_regs(regs, IR_MAGIC_HI)
    if magic != 0xB00710AD:
        print(f"ERROR: unexpected magic 0x{magic:08X} — is bootloader running?")
        sys.exit(1)
    state = regs[IR_BOOT_STATE]
    if state not in (3, 4, 5, 9):   # WAIT_COMMAND, RECEIVE_FW, VERIFY_STAGING, ERROR
        print(f"ERROR: unexpected boot state {BOOT_STATE_NAMES.get(state, str(state))}")
        sys.exit(1)

    # ---- BEGIN_UPDATE ------------------------------------------------------
    print("Step 1/4  BEGIN_UPDATE (erasing staging…)")
    params = [0] * 0x10  # HR[0x0000..0x000F] — zero first
    # HR[0x0010]: image_size (2 regs, hi/lo)
    params += [
        (image_size >> 16) & 0xFFFF,    # 0x0010
        image_size         & 0xFFFF,    # 0x0011
        (image_crc  >> 16) & 0xFFFF,    # 0x0012
        image_crc          & 0xFFFF,    # 0x0013
        (fw_version >> 16) & 0xFFFF,    # 0x0014
        fw_version         & 0xFFFF,    # 0x0015
        (PRODUCT_ID_DEFAULT >> 16) & 0xFFFF,  # 0x0016
        PRODUCT_ID_DEFAULT         & 0xFFFF,  # 0x0017
        HW_REVISION_DEFAULT,            # 0x0018
        block_size,                     # 0x0019
        (block_count >> 16) & 0xFFFF,   # 0x001A
        block_count         & 0xFFFF,   # 0x001B
    ]
    # Write params (starting at HR[0x0010] to avoid touching command reg)
    write_regs(c, 0x0010, params[0x10:])
    # Trigger BEGIN_UPDATE
    write_cmd(c, CMD_BEGIN_UPDATE)
    # Staging erase takes ~1s per sector × 2 sectors ≈ 2-4s
    if not wait_for_status(c, CMD_STATUS_OK, 20.0):
        print("BEGIN_UPDATE failed")
        sys.exit(1)
    print("  OK")

    # ---- WRITE_BLOCK -------------------------------------------------------
    print(f"Step 2/4  Sending {block_count} blocks…")
    for blk_idx in range(block_count):
        offset = blk_idx * block_size
        chunk  = fw_data[offset:offset + block_size]
        data_len = len(chunk)

        # Build register payload: [block_idx_hi, block_idx_lo, data_len, data...]
        n_data_regs = math.ceil(data_len / 2)
        payload = [
            (blk_idx >> 16) & 0xFFFF,   # reg 0 — block_idx high
            blk_idx         & 0xFFFF,   # reg 1 — block_idx low
            data_len,                   # reg 2 — data length in bytes
        ] + bytes_to_regs(chunk)        # regs 3..n — firmware data

        # FC16 write to HR[0x0100], qty = 3 + n_data_regs
        # This triggers exec_write_block() on the device
        write_regs(c, HR_BLOCK_BASE, payload)
        if not wait_for_status(c, CMD_STATUS_OK, 5.0):
            print(f"  Block {blk_idx} write failed")
            sys.exit(1)

        pct = int((blk_idx + 1) * 100 / block_count)
        print(f"  {blk_idx + 1}/{block_count}  [{pct:3d}%]", end='\r', flush=True)

    print(f"  {block_count}/{block_count}  [100%]  done          ")

    # ---- FINALIZE (CRC check of staging) -----------------------------------
    print("Step 3/4  FINALIZE (CRC32 check of staging area…)")
    write_cmd(c, CMD_FINALIZE_UPDATE)
    if not wait_for_status(c, CMD_STATUS_OK, 10.0):
        print("FINALIZE failed — CRC mismatch or blocks missing")
        sys.exit(1)
    print("  Staging CRC OK")

    # ---- INSTALL -----------------------------------------------------------
    print("Step 4/4  INSTALL (erasing app, copying, verifying…)")
    write_cmd(c, CMD_INSTALL_UPDATE)
    # Installation: erase 2 sectors (~1s each) + copy 256 KB + verify
    if not wait_for_status(c, CMD_STATUS_OK, 30.0):
        print("INSTALL failed")
        sys.exit(1)
    print("  Install OK")
    print()
    print("Firmware update complete. Device will reboot to application.")
    c.close()


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description="PLCJS Bootloader OTA client (Modbus TCP)")
    parser.add_argument('--ip',   default=TARGET_IP,   help=f"Target IP (default {TARGET_IP})")
    parser.add_argument('--port', default=TARGET_PORT, type=int,
                        help=f"Modbus TCP port (default {TARGET_PORT})")

    sub = parser.add_subparsers(dest='command', required=True)

    sub.add_parser('status',  help='Read bootloader status registers')
    sub.add_parser('abort',   help='Abort current update session')
    sub.add_parser('reboot',  help='Software reset the device')

    p_upd = sub.add_parser('update', help='Upload firmware binary')
    p_upd.add_argument('firmware', help='Path to firmware .bin file')
    p_upd.add_argument('--version', type=lambda x: int(x, 0),
                       default=0x00010000,
                       help='Firmware version as 0xMMmmpp (default 0x00010000)')

    args = parser.parse_args()

    dispatch = {
        'status': cmd_status,
        'abort':  cmd_abort,
        'reboot': cmd_reboot,
        'update': cmd_update,
    }
    dispatch[args.command](args)


if __name__ == '__main__':
    main()
