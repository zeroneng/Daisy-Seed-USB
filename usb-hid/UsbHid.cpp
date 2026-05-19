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

constexpr uint8_t kChord1[] = {0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B}; // a..h
constexpr uint8_t kChord2[] = {0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B}; // q..x
constexpr uint8_t kChord3[] = {0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25}; // 1..8

void PressKeys(const uint8_t *keys, size_t count)
{
    for(size_t i = 0; i < count; ++i)
        UsbHid_KeyOn(keys[i]);
}

void ReleaseKeys(const uint8_t *keys, size_t count)
{
    for(size_t i = 0; i < count; ++i)
        UsbHid_KeyOff(keys[i]);
}

void BlinkOnce(int ms)
{
    hw.SetLed(true);
    System::Delay(ms);
    hw.SetLed(false);
}

void RunNkroStressPattern()
{
    // Chord 1: 8 simultaneous alpha keys (exceeds old 6KRO limit)
    PressKeys(kChord1, sizeof(kChord1));
    BlinkOnce(250);
    System::Delay(1000);
    ReleaseKeys(kChord1, sizeof(kChord1));
    System::Delay(500);

    // Chord 2: another 8-key alpha block
    PressKeys(kChord2, sizeof(kChord2));
    BlinkOnce(250);
    System::Delay(1000);
    ReleaseKeys(kChord2, sizeof(kChord2));
    System::Delay(500);

    // Chord 3: 8 simultaneous number-row keys
    PressKeys(kChord3, sizeof(kChord3));
    BlinkOnce(250);
    System::Delay(1000);
    ReleaseKeys(kChord3, sizeof(kChord3));
    System::Delay(500);

    // Modifier combo: Shift + Ctrl with 8 regular keys at once
    UsbHid_KeyOn(0xE1); // Left Shift
    UsbHid_KeyOn(0xE0); // Left Ctrl
    PressKeys(kChord1, sizeof(kChord1));
    BlinkOnce(500);
    System::Delay(1200);
    ReleaseKeys(kChord1, sizeof(kChord1));
    UsbHid_KeyOff(0xE0);
    UsbHid_KeyOff(0xE1);
    System::Delay(1500);
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
