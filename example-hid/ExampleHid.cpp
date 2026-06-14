#include "daisy_seed.h"
#include "HID.h"

using namespace daisy;

#ifndef EXAMPLE_HID_TEST_KEYS
#define EXAMPLE_HID_TEST_KEYS 1
#endif

namespace {
DaisySeed hw;
constexpr char kTestChar = 'a';

void SendTestKeyTap()
{
    const uint8_t keycode = UsbHid_CharToKeycode(kTestChar);
    if(keycode == 0x00)
        return;

    UsbHid_KeyOn(keycode);
    System::Delay(40);
    UsbHid_KeyOff(keycode);
}
} // namespace

int main(void)
{
    hw.Init();
    UsbHid_Init();

    System::Delay(1500);
    UsbHid_ClearAllKeys();

    bool led_state = false;
    while(true)
    {
        led_state = !led_state;
        hw.SetLed(led_state);

#if EXAMPLE_HID_TEST_KEYS
        if(led_state)
            SendTestKeyTap();
#endif

        System::Delay(500);
    }
}
