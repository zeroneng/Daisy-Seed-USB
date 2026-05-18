#include "daisy_seed.h"
#include <cstring>

using namespace daisy;

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_hid.h"
extern USBD_HandleTypeDef hUsbDeviceHS;
}

namespace {
DaisySeed hw;
uint8_t   hid_report[8] = {0};

void InitUsbHid()
{
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
    USBD_RegisterClass(&hUsbDeviceHS, &USBD_HID);
    USBD_Start(&hUsbDeviceHS);
}

void SendHidReport()
{
    USBD_HID_SendReport(&hUsbDeviceHS, hid_report, sizeof(hid_report));
}

[[maybe_unused]] void ClearAllKeys()
{
    std::memset(hid_report, 0, sizeof(hid_report));
    SendHidReport();
}

[[maybe_unused]] bool KeyOn(uint8_t keycode)
{
    if(keycode == 0x00)
        return false;

    for(int i = 2; i < 8; ++i)
    {
        if(hid_report[i] == keycode)
        {
            SendHidReport();
            return true;
        }
    }

    for(int i = 2; i < 8; ++i)
    {
        if(hid_report[i] == 0x00)
        {
            hid_report[i] = keycode;
            SendHidReport();
            return true;
        }
    }

    return false;
}

[[maybe_unused]] bool KeyOff(uint8_t keycode)
{
    if(keycode == 0x00)
        return false;

    bool changed = false;
    for(int i = 2; i < 8; ++i)
    {
        if(hid_report[i] == keycode)
        {
            hid_report[i] = 0x00;
            changed       = true;
        }
    }

    if(changed)
        SendHidReport();

    return changed;
}
} // namespace

int main(void)
{
    hw.Init();
    InitUsbHid();

    // Example usage:
    // System::Delay(1000);
    // KeyOn(0x04);   // press 'a'
    // System::Delay(50);
    // KeyOff(0x04);  // release 'a'
    // System::Delay(50);
    // ClearAllKeys();

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
