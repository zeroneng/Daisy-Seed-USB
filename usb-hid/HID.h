#pragma once

#include <stdint.h>
#include <stddef.h>

void UsbHid_Init(void);
void UsbHid_SendReport(void);
void UsbHid_ClearAllKeys(void);
bool UsbHid_SetKeyState(uint8_t keycode, bool pressed);
bool UsbHid_KeyOn(uint8_t keycode);
bool UsbHid_KeyOff(uint8_t keycode);
void UsbHid_PressKeys(const uint8_t *keys, size_t count);
void UsbHid_ReleaseKeys(const uint8_t *keys, size_t count);


void UsbHid_SetChord32(uint8_t start_keycode, uint32_t chord_bits);
void UsbHid_ClearChord32(uint8_t start_keycode, uint32_t chord_bits);
