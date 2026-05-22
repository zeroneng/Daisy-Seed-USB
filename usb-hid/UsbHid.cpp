#include "daisy_seed.h"
#include "HID.h"

using namespace daisy;

namespace {
DaisySeed hw;
constexpr char kTestChar = 'a';

void BlinkOnce(int ms)
{
    hw.SetLed(true);
    System::Delay(ms);
    hw.SetLed(false);
}

void SendATest()
{
    const uint8_t keycode = UsbHid_CharToKeycode(kTestChar);
    if(keycode == 0x00)
        return;

    UsbHid_SetKeyState(keycode, true);
    UsbHid_SendReport();
    BlinkOnce(80);
    System::Delay(200);
    UsbHid_SetKeyState(keycode, false);
    UsbHid_SendReport();
    System::Delay(800);
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
        SendATest();
    }
}
