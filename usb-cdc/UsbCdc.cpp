#include "daisy_seed.h"
#include <cstring>

using namespace daisy;

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "../libDaisy/src/usbd/usbd_cdc_if.h"
extern USBD_HandleTypeDef hUsbDeviceHS;
}

DaisySeed hw;

static void InitUSBCDC(void)
{
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
    USBD_RegisterClass(&hUsbDeviceHS, &USBD_CDC);
    USBD_CDC_RegisterInterface(&hUsbDeviceHS, &USBD_Interface_fops_HS);
    USBD_Start(&hUsbDeviceHS);
}

static void SendString(const char* s)
{
    if(!s)
        return;
    CDC_Transmit_HS((uint8_t*)s, (uint16_t)strlen(s));
}

int main(void)
{
    hw.Init();
    InitUSBCDC();

    bool led = false;
    for(;;)
    {
        led = !led;
        hw.SetLed(led);
        SendString(led ? "LED: ON\r\n" : "LED: OFF\r\n");
        System::Delay(250);
    }
}
