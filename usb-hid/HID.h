#pragma once

#include <stdint.h>

void UsbHid_Init(void);
void UsbHid_SendReport(void);
void UsbHid_ClearAllKeys(void);
bool UsbHid_KeyOn(uint8_t keycode);
bool UsbHid_KeyOff(uint8_t keycode);
