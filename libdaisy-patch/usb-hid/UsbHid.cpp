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

constexpr uint8_t kPhraseKeys[] = {
    0x04, // a
};

void BlinkOnce(int ms)
{
    hw.SetLed(true);
    System::Delay(ms);
    hw.SetLed(false);
}

void TapKey(uint8_t keycode)
{
    UsbHid_KeyOn(keycode);
    BlinkOnce(60);
    System::Delay(120);
    UsbHid_KeyOff(keycode);
    System::Delay(220);
}

void RunBootKeyboardTestPattern()
{
    for(size_t i = 0; i < sizeof(kPhraseKeys); ++i)
        TapKey(kPhraseKeys[i]);

    UsbHid_ClearAllKeys();
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
        RunBootKeyboardTestPattern();
#else
        hw.SetLed(true);
        System::Delay(500);
        hw.SetLed(false);
        System::Delay(500);
#endif
    }
}
