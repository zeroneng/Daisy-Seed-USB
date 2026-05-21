#include "daisy_seed.h"
#include "HID.h"

using namespace daisy;

namespace {
DaisySeed hw;
constexpr uint8_t kKeyA = 0x04;

void BlinkMs(int ms)
{
    hw.SetLed(true);
    System::Delay(ms);
    hw.SetLed(false);
}

void SendATapOncePerSecond()
{
    UsbHid_KeyOn(kKeyA);
    BlinkMs(80);
    System::Delay(120);
    UsbHid_KeyOff(kKeyA);
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
        SendATapOncePerSecond();
    }
}
