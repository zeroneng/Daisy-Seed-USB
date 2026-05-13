#include "daisy_seed.h"
#include <cstring>
#include <cctype>

using namespace daisy;

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_hid.h"
extern USBD_HandleTypeDef hUsbDeviceHS;
}

DaisySeed hw;

static uint8_t hid_report[8] = {0};

static void InitUSBHID(void)
{
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
    USBD_RegisterClass(&hUsbDeviceHS, &USBD_HID);
    USBD_Start(&hUsbDeviceHS);
}

static void SendReport()
{
    USBD_HID_SendReport(&hUsbDeviceHS, hid_report, sizeof(hid_report));
}

static bool StrEqIgnoreCase(const char* a, const char* b)
{
    while(*a && *b)
    {
        if(std::toupper((unsigned char)*a) != std::toupper((unsigned char)*b))
            return false;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static uint8_t ModifierMaskForName(const char* name)
{
    if(StrEqIgnoreCase(name, "CTRL") || StrEqIgnoreCase(name, "LCTRL")) return 0x01;
    if(StrEqIgnoreCase(name, "SHIFT") || StrEqIgnoreCase(name, "LSHIFT")) return 0x02;
    if(StrEqIgnoreCase(name, "ALT") || StrEqIgnoreCase(name, "LALT")) return 0x04;
    if(StrEqIgnoreCase(name, "GUI") || StrEqIgnoreCase(name, "WIN") || StrEqIgnoreCase(name, "CMD")) return 0x08;
    if(StrEqIgnoreCase(name, "RCTRL")) return 0x10;
    if(StrEqIgnoreCase(name, "RSHIFT")) return 0x20;
    if(StrEqIgnoreCase(name, "RALT")) return 0x40;
    if(StrEqIgnoreCase(name, "RGUI") || StrEqIgnoreCase(name, "RCMD")) return 0x80;
    return 0x00;
}

static uint8_t KeycodeForName(const char* name)
{
    if(name[0] && name[1] == '\0')
    {
        char c = name[0];
        if(c >= 'a' && c <= 'z') return 0x04 + (c - 'a');
        if(c >= 'A' && c <= 'Z') return 0x04 + (c - 'A');
        if(c >= '1' && c <= '9') return 0x1E + (c - '1');
        if(c == '0') return 0x27;
        if(c == ' ') return 0x2C;
    }

    if(StrEqIgnoreCase(name, "ENTER")) return 0x28;
    if(StrEqIgnoreCase(name, "ESC") || StrEqIgnoreCase(name, "ESCAPE")) return 0x29;
    if(StrEqIgnoreCase(name, "BACKSPACE")) return 0x2A;
    if(StrEqIgnoreCase(name, "TAB")) return 0x2B;
    if(StrEqIgnoreCase(name, "SPACE")) return 0x2C;
    if(StrEqIgnoreCase(name, "MINUS")) return 0x2D;
    if(StrEqIgnoreCase(name, "EQUAL") || StrEqIgnoreCase(name, "=")) return 0x2E;
    if(StrEqIgnoreCase(name, "LEFT") || StrEqIgnoreCase(name, "LEFTARROW")) return 0x50;
    if(StrEqIgnoreCase(name, "RIGHT") || StrEqIgnoreCase(name, "RIGHTARROW")) return 0x4F;
    if(StrEqIgnoreCase(name, "DOWN") || StrEqIgnoreCase(name, "DOWNARROW")) return 0x51;
    if(StrEqIgnoreCase(name, "UP") || StrEqIgnoreCase(name, "UPARROW")) return 0x52;

    return 0x00;
}

void Press(const char* name)
{
    uint8_t mod = ModifierMaskForName(name);
    if(mod)
    {
        hid_report[0] |= mod;
        SendReport();
        return;
    }

    uint8_t key = KeycodeForName(name);
    if(!key)
        return;

    for(int i = 2; i < 8; i++)
    {
        if(hid_report[i] == key)
        {
            SendReport();
            return;
        }
    }

    for(int i = 2; i < 8; i++)
    {
        if(hid_report[i] == 0x00)
        {
            hid_report[i] = key;
            SendReport();
            return;
        }
    }
}

void Release(const char* name)
{
    uint8_t mod = ModifierMaskForName(name);
    if(mod)
    {
        hid_report[0] &= ~mod;
        SendReport();
        return;
    }

    uint8_t key = KeycodeForName(name);
    if(!key)
        return;

    for(int i = 2; i < 8; i++)
    {
        if(hid_report[i] == key)
        {
            hid_report[i] = 0x00;
        }
    }
    SendReport();
}

int main(void)
{
    hw.Init();
    InitUSBHID();

    System::Delay(2000);

    const char* sequence[] = {"a", "SPACE", "a", "SPACE", "a"};

    bool led = false;
    for(;;)
    {
        for(size_t i = 0; i < 5; ++i)
        {
            Press(sequence[i]);
            System::Delay(40);
            Release(sequence[i]);
            System::Delay(40);
        }

        for(int i = 0; i < 20; ++i)
        {
            led = !led;
            hw.SetLed(led);
            System::Delay(250);
        }
    }
}
