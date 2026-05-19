#pragma once

#include <stdint.h>
#include <stddef.h>

// Initialize the USB HID device stack.
void UsbHid_Init(void);

// Send the current full keyboard report if it changed since the last send.
// Typical usage: update all key states first, then call this once per scan pass.
void UsbHid_SendReport(void);

// Clear all modifier and key bits, then send the all-released report if changed.
void UsbHid_ClearAllKeys(void);

// Set or clear one key bit in the in-memory report without sending immediately.
// This is the preferred function for matrix scanning: call it for each key, then
// call UsbHid_SendReport() once after the full scan is complete.
bool UsbHid_SetKeyState(uint8_t keycode, bool pressed);

// Convenience wrapper: mark one key pressed and immediately send the report.
// Use for simple/manual tests. For matrix scanning, prefer UsbHid_SetKeyState().
bool UsbHid_KeyOn(uint8_t keycode);

// Convenience wrapper: mark one key released and immediately send the report.
// Use for simple/manual tests. For matrix scanning, prefer UsbHid_SetKeyState().
bool UsbHid_KeyOff(uint8_t keycode);

// Batch helper: set multiple keys pressed in memory, then send one report.
// Useful for grouped test patterns.
void UsbHid_PressKeys(const uint8_t *keys, size_t count);

// Batch helper: clear multiple keys in memory, then send one report.
// Useful for grouped test patterns.
void UsbHid_ReleaseKeys(const uint8_t *keys, size_t count);

// Apply a 32-bit chord window starting at start_keycode, then send one report.
// Bit 0 maps to start_keycode, bit 1 to start_keycode + 1, and so on.
void UsbHid_SetChord32(uint8_t start_keycode, uint32_t chord_bits);

// Clear a 32-bit chord window starting at start_keycode, then send one report.
void UsbHid_ClearChord32(uint8_t start_keycode, uint32_t chord_bits);
