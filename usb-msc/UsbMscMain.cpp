#include "daisy_seed.h"

using namespace daisy;

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_msc.h"
#include "usbd_msc_storage.h"
extern USBD_HandleTypeDef hUsbDeviceHS;
}

DaisySeed hw;

static void InitUSBMSC(void)
{
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
    USBD_RegisterClass(&hUsbDeviceHS, &USBD_MSC);
    USBD_MSC_RegisterStorage(&hUsbDeviceHS, &USBD_MSC_Template_fops);
    USBD_Start(&hUsbDeviceHS);
}

int main(void)
{
    hw.Init();
    InitUSBMSC();

    bool led = false;
    for(;;)
    {
        led = !led;
        hw.SetLed(led);
        System::Delay(250);
    }
}
