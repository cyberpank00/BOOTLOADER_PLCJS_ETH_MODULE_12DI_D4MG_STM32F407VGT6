# FW Utility Programmer Specification

## Summary

Create a graphical firmware programming utility for the STM32F407 Ethernet module.
The tool must reuse the existing Modbus TCP OTA workflow implemented in
`tools/fw_update.py` and provide an operator-friendly UI without requiring a
console.

## Goals

- Read bootloader status from the module
- Select a `.bin` firmware image
- Upload firmware over Modbus TCP
- Show progress and errors clearly
- Support abort and reboot actions

## Non-goals

- Changing the bootloader protocol
- Replacing the current OTA format
- Adding a new firmware transport

## Functional Requirements

### Connection

- Connect to a target by IP address and TCP port
- Use the same Modbus TCP register map and command sequence as `fw_update.py`
- Allow the operator to retry a failed connection

### Status View

- Show:
  - boot state
  - last error
  - app valid flag
  - app version
  - product ID
  - hardware revision
  - received blocks / total blocks
  - image size
  - image CRC32
  - command status
  - staging valid flag

### Firmware Upload

- Load a local `.bin` file
- Compute and display image size and CRC32
- Send `BEGIN_UPDATE`
- Transfer blocks with progress feedback
- Send `FINALIZE_UPDATE`
- Send `INSTALL_UPDATE`
- Report success or failure in human-readable form

### Control Actions

- `Status`
- `Abort`
- `Reboot`
- `Select firmware`
- `Flash`

## UI Requirements

- Single-window desktop application
- Input fields for target IP and port
- File picker for firmware image
- Progress bar for block transfer
- Log panel for actions and errors
- Read-only status panel for bootloader state

## Error Handling

- Show network errors clearly
- Translate bootloader error codes to readable text
- Keep the UI responsive during upload
- Preserve logs for the current session

## Acceptance Criteria

- The operator can read bootloader status without a console
- The operator can flash a valid firmware image successfully
- The tool shows progress while uploading
- The tool reports invalid CRC and interrupted-transfer failures correctly
- The tool can abort an active update session
- The tool can reboot the target after a successful or interrupted update

## Reference

The GUI must be based on the behavior of:

- `tools/fw_update.py`
- the current bootloader register map
- the tested app ↔ bootloader warm-reset workflow
