#include "HID.h"

#include <cstring>

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_hid.h"
extern USBD_HandleTypeDef hUsbDeviceHS;
}

namespace {
constexpr int kReportBytes = 33;
constexpr int kBitmapOffset = 1;
constexpr int kBitmapBytes = 32;
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

bool UsbHid_SetKeyState(uint8_t keycode, bool pressed)
{
    if(keycode == 0x00 || keycode > 0xE7)
        return false;

    if(keycode >= 0xE0 && keycode <= 0xE7)
    {
        const uint8_t bit_mask = static_cast<uint8_t>(1u << (keycode - 0xE0));
        if(pressed)
            current_report[0] |= bit_mask;
        else
            current_report[0] &= static_cast<uint8_t>(~bit_mask);
        return true;
    }

    const uint8_t byte_index = static_cast<uint8_t>(keycode >> 3);
    const uint8_t bit_mask   = static_cast<uint8_t>(1u << (keycode & 0x07u));
    if(byte_index >= kBitmapBytes)
        return false;

    if(pressed)
        current_report[kBitmapOffset + byte_index] |= bit_mask;
    else
        current_report[kBitmapOffset + byte_index] &= static_cast<uint8_t>(~bit_mask);
    return true;
}

bool UsbHid_KeyOn(uint8_t keycode)
{
    const bool ok = UsbHid_SetKeyState(keycode, true);
    if(ok)
        UsbHid_SendReport();
    return ok;
}

bool UsbHid_KeyOff(uint8_t keycode)
{
    const bool ok = UsbHid_SetKeyState(keycode, false);
    if(ok)
        UsbHid_SendReport();
    return ok;
}

uint8_t UsbHid_CharToKeycode(char c)
{
    if(c >= 'a' && c <= 'z')
        return static_cast<uint8_t>(0x04 + (c - 'a'));

    if(c >= 'A' && c <= 'Z')
        return static_cast<uint8_t>(0x04 + (c - 'A'));

    if(c >= '1' && c <= '9')
        return static_cast<uint8_t>(0x1E + (c - '1'));

    switch(c)
    {
        case '0': return 0x27;
        case '\n': return 0x28;
        case '\r': return 0x28;
        case 0x1B: return 0x29;
        case '\b': return 0x2A;
        case '\t': return 0x2B;
        case ' ': return 0x2C;
        case '-': return 0x2D;
        case '=': return 0x2E;
        case '[': return 0x2F;
        case ']': return 0x30;
        case '\\': return 0x31;
        case ';': return 0x33;
        case '\'': return 0x34;
        case '`': return 0x35;
        case ',': return 0x36;
        case '.': return 0x37;
        case '/': return 0x38;
        default: return 0x00;
    }
}

