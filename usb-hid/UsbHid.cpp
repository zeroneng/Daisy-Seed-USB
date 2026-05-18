#include "daisy_seed.h"

using namespace daisy;

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_hid.h"
extern USBD_HandleTypeDef hUsbDeviceHS;
}

namespace {
DaisySeed hw;

void InitUsbHid()
{
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
    USBD_RegisterClass(&hUsbDeviceHS, &USBD_HID);
    USBD_Start(&hUsbDeviceHS);
}
} // namespace

int main(void)
{
    hw.Init();
    InitUsbHid();

    // Deliberately idle after enumeration.
    // This sample is intended as a clean HID bring-up base that can be imported
    // into another project, not as a self-typing demo loop.
    bool led = false;
    for(;;)
    {
        led = !led;
        hw.SetLed(led);
        System::Delay(500);
    }
}
