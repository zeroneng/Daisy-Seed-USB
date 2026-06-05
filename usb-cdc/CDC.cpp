#include "CDC.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "../../libDaisy/src/usbd/usbd_cdc_if.h"
extern USBD_HandleTypeDef hUsbDeviceHS;
}

void UsbCdc_Init(void)
{
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
    USBD_RegisterClass(&hUsbDeviceHS, &USBD_CDC);
    USBD_CDC_RegisterInterface(&hUsbDeviceHS, &USBD_Interface_fops_HS);
    USBD_Start(&hUsbDeviceHS);
}

int UsbCdc_Transmit(const uint8_t* data, uint16_t len)
{
    if(!data || len == 0)
        return 0;
    return (int)CDC_Transmit_HS((uint8_t*)data, len);
}

int UsbCdc_TransmitString(const char* s)
{
    if(!s)
        return 0;
    return UsbCdc_Transmit((const uint8_t*)s, (uint16_t)strlen(s));
}

void UsbCdc_PrintLine(const char* fmt, ...)
{
    if(!fmt)
        return;

    char buffer[256];

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buffer, sizeof(buffer) - 2, fmt, args);
    va_end(args);

    if(n <= 0)
        return;

    size_t len = (size_t)n;
    if(len > sizeof(buffer) - 3)
        len = sizeof(buffer) - 3;

    buffer[len++] = '\r';
    buffer[len++] = '\n';
    buffer[len]   = '\0';

    CDC_Transmit_HS((uint8_t*)buffer, (uint16_t)len);
}
