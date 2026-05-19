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

// HID usage codes for:
// asdfghjkl;'zxcvbnm,./
constexpr uint8_t kPhraseKeys[] = {
    0x04, // a
    0x16, // s
    0x07, // d
    0x09, // f
    0x0A, // g
    0x0B, // h
    0x0D, // j
    0x0E, // k
    0x0F, // l
    0x33, // ;
    0x34, // '
    0x1D, // z
    0x1B, // x
    0x06, // c
    0x19, // v
    0x05, // b
    0x11, // n
    0x10, // m
    0x36, // ,
    0x37, // .
    0x38  // /
};

void BlinkOnce(int ms)
{
    hw.SetLed(true);
    System::Delay(ms);
    hw.SetLed(false);
}

void RunNkroStressPattern()
{
    // Press the whole phrase as one combined report.
    for(size_t i = 0; i < sizeof(kPhraseKeys); ++i)
        UsbHid_SetKeyState(kPhraseKeys[i], true);
    UsbHid_SendReport();

    BlinkOnce(500);
    System::Delay(1600);

    // Release the whole phrase as one combined report.
    for(size_t i = 0; i < sizeof(kPhraseKeys); ++i)
        UsbHid_SetKeyState(kPhraseKeys[i], false);
    UsbHid_SendReport();

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
