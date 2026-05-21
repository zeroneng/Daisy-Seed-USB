#include "HID.h"

#include <cstring>

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_hid.h"
extern USBD_HandleTypeDef hUsbDeviceHS;
}

namespace {
constexpr int kReportBytes = 8;
uint8_t current_report[kReportBytes]   = {0};
uint8_t last_sent_report[kReportBytes] = {0};

bool SendIfChanged()
{
    if(std::memcmp(current_report, last_sent_report, sizeof(current_report)) == 0)
        return false;

    USBD_HID_SendReport(&hUsbDeviceHS, current_report, sizeof(current_report));
    std::memcpy(last_sent_report, current_report, sizeof(current_report));
    return true;
}

int FindKeySlot(uint8_t keycode)
{
    for(int i = 2; i < 8; ++i)
    {
        if(current_report[i] == keycode)
            return i;
    }
    return -1;
}

int FindEmptySlot()
{
    for(int i = 2; i < 8; ++i)
    {
        if(current_report[i] == 0x00)
            return i;
    }
    return -1;
}
}

void UsbHid_Init(void)
{
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
    USBD_RegisterClass(&hUsbDeviceHS, &USBD_HID);
    USBD_Start(&hUsbDeviceHS);
}

void UsbHid_SendReport(void)
{
    SendIfChanged();
}

void UsbHid_ClearAllKeys(void)
{
    std::memset(current_report, 0, sizeof(current_report));
    UsbHid_SendReport();
}

bool UsbHid_KeyOn(uint8_t keycode)
{
    if(keycode == 0x00)
        return false;

    if(keycode >= 0xE0 && keycode <= 0xE7)
    {
        current_report[0] |= static_cast<uint8_t>(1u << (keycode - 0xE0));
        UsbHid_SendReport();
        return true;
    }

    if(FindKeySlot(keycode) >= 0)
    {
        UsbHid_SendReport();
        return true;
    }

    const int slot = FindEmptySlot();
    if(slot < 0)
        return false;

    current_report[slot] = keycode;
    UsbHid_SendReport();
    return true;
}

bool UsbHid_KeyOff(uint8_t keycode)
{
    if(keycode == 0x00)
        return false;

    bool changed = false;
    if(keycode >= 0xE0 && keycode <= 0xE7)
    {
        const uint8_t mask = static_cast<uint8_t>(1u << (keycode - 0xE0));
        const uint8_t prev = current_report[0];
        current_report[0] &= static_cast<uint8_t>(~mask);
        changed = (prev != current_report[0]);
    }
    else
    {
        const int slot = FindKeySlot(keycode);
        if(slot >= 0)
        {
            current_report[slot] = 0x00;
            changed = true;
        }
    }

    if(changed)
        UsbHid_SendReport();
    return changed;
}
