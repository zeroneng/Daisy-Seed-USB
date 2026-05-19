#include "usb_hid_helpers.h"

#include <cstring>

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_hid.h"
extern USBD_HandleTypeDef hUsbDeviceHS;
}

namespace {
uint8_t hid_report[8] = {0};
}

void UsbHid_Init(void)
{
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
    USBD_RegisterClass(&hUsbDeviceHS, &USBD_HID);
    USBD_Start(&hUsbDeviceHS);
}

void UsbHid_SendReport(void)
{
    USBD_HID_SendReport(&hUsbDeviceHS, hid_report, sizeof(hid_report));
}

void UsbHid_ClearAllKeys(void)
{
    std::memset(hid_report, 0, sizeof(hid_report));
    UsbHid_SendReport();
}

bool UsbHid_KeyOn(uint8_t keycode)
{
    if(keycode == 0x00)
        return false;

    for(int i = 2; i < 8; ++i)
    {
        if(hid_report[i] == keycode)
        {
            UsbHid_SendReport();
            return true;
        }
    }

    for(int i = 2; i < 8; ++i)
    {
        if(hid_report[i] == 0x00)
        {
            hid_report[i] = keycode;
            UsbHid_SendReport();
            return true;
        }
    }

    return false;
}

bool UsbHid_KeyOff(uint8_t keycode)
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
        UsbHid_SendReport();

    return changed;
}
