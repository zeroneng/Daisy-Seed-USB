#pragma once

#include <stdint.h>

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

// Convert a human-readable ASCII character to a HID usage code.
// Returns 0 when no direct mapping is available.
uint8_t UsbHid_CharToKeycode(char c);

