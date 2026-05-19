#include "daisy_seed.h"
#include "HID.h"

using namespace daisy;

namespace {
DaisySeed hw;
} // namespace

namespace {
#ifndef USB_HID_TEST_MODE
#define USB_HID_TEST_MODE 1
#endif

constexpr uint32_t kChord32_All = 0xFFFFFFFFu; // 32 contiguous keys

void BlinkOnce(int ms)
{
    hw.SetLed(true);
    System::Delay(ms);
    hw.SetLed(false);
}

void RunNkroStressPattern()
{
    // 32-key chord sent as one 32-bit mask over a contiguous key range starting at 'a'
    UsbHid_SetChord32(0x04, kChord32_All);
    BlinkOnce(500);
    System::Delay(1600);
    UsbHid_ClearChord32(0x04, kChord32_All);
    System::Delay(900);

    // Modifier combo: Shift + Ctrl with the same 32-key chord
    UsbHid_KeyOn(0xE1); // Left Shift
    UsbHid_KeyOn(0xE0); // Left Ctrl
    UsbHid_SetChord32(0x04, kChord32_All);
    BlinkOnce(700);
    System::Delay(1800);
    UsbHid_ClearChord32(0x04, kChord32_All);
    UsbHid_KeyOff(0xE0);
    UsbHid_KeyOff(0xE1);
    System::Delay(2000);
}
} // namespace

int main(void)
{
    hw.Init();
    UsbHid_Init();

    System::Delay(1500);
    UsbHid_ClearAllKeys();

    for(;;)
    {
#if USB_HID_TEST_MODE
        RunNkroStressPattern();
#else
        hw.SetLed(true);
        System::Delay(500);
        hw.SetLed(false);
        System::Delay(500);
#endif
    }
}
