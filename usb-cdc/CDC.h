#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void UsbCdc_Init(void);
int  UsbCdc_Transmit(const uint8_t* data, uint16_t len);
int  UsbCdc_TransmitString(const char* s);
void UsbCdc_PrintLine(const char* fmt, ...);

#ifdef __cplusplus
}
#endif
