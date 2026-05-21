#include "daisy_seed.h"
#include "HID.h"

using namespace daisy;

namespace {
DaisySeed hw;
constexpr uint8_t kKeyA = 0x04;

void BlinkOnce(int ms)
{
    hw.SetLed(true);
    System::Delay(ms);
    hw.SetLed(false);
}

void SendATest()
{
    UsbHid_SetKeyState(kKeyA, true);
    UsbHid_SendReport();
    BlinkOnce(80);
    System::Delay(200);
    UsbHid_SetKeyState(kKeyA, false);
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
