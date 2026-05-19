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

/* Old boot-style 6-key rollover implementation kept for reference.
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
*/

bool UsbHid_SetKeyState(uint8_t keycode, bool pressed)
{
    if(keycode > 0xE7)
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

void UsbHid_PressKeys(const uint8_t *keys, size_t count)
{
    for(size_t i = 0; i < count; ++i)
        UsbHid_SetKeyState(keys[i], true);
}

void UsbHid_ReleaseKeys(const uint8_t *keys, size_t count)
{
    for(size_t i = 0; i < count; ++i)
        UsbHid_SetKeyState(keys[i], false);
}

void UsbHid_SetChord32(uint8_t start_keycode, uint32_t chord_bits)
{
    for(uint8_t bit = 0; bit < 32; ++bit)
    {
        if((chord_bits & (1UL << bit)) == 0)
            continue;
        const uint8_t keycode = static_cast<uint8_t>(start_keycode + bit);
        if(keycode > 0xDF)
            continue;
        const uint8_t byte_index = static_cast<uint8_t>(keycode >> 3);
        const uint8_t bit_mask   = static_cast<uint8_t>(1u << (keycode & 0x07u));
        if(byte_index < kBitmapBytes)
            current_report[kBitmapOffset + byte_index] |= bit_mask;
    }
    UsbHid_SendReport();
}

void UsbHid_ClearChord32(uint8_t start_keycode, uint32_t chord_bits)
{
    for(uint8_t bit = 0; bit < 32; ++bit)
    {
        if((chord_bits & (1UL << bit)) == 0)
            continue;
        const uint8_t keycode = static_cast<uint8_t>(start_keycode + bit);
        if(keycode > 0xDF)
            continue;
        const uint8_t byte_index = static_cast<uint8_t>(keycode >> 3);
        const uint8_t bit_mask   = static_cast<uint8_t>(1u << (keycode & 0x07u));
        if(byte_index < kBitmapBytes)
            current_report[kBitmapOffset + byte_index] &= static_cast<uint8_t>(~bit_mask);
    }
    UsbHid_SendReport();
}
