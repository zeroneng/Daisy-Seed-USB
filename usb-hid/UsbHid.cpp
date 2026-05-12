#include "daisy_seed.h"

using namespace daisy;

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_hid.h"
extern USBD_HandleTypeDef hUsbDeviceHS;
}

DaisySeed hw;

static void InitUSBHID(void)
{
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
    USBD_RegisterClass(&hUsbDeviceHS, &USBD_HID);
    USBD_Start(&hUsbDeviceHS);
}

int main(void)
{
    hw.Init();
    InitUSBHID();

    uint8_t press_a[8] = {0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t release[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    System::Delay(2000);
    USBD_HID_SendReport(&hUsbDeviceHS, press_a, sizeof(press_a));
    System::Delay(50);
    USBD_HID_SendReport(&hUsbDeviceHS, release, sizeof(release));

    bool led = false;
    for(;;)
    {
        led = !led;
        hw.SetLed(led);
        System::Delay(250);
    }
}
