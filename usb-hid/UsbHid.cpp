#include "daisy_seed.h"
#include "HID.h"

using namespace daisy;

namespace {
DaisySeed hw;
} // namespace

int main(void)
{
    hw.Init();
    UsbHid_Init();

    // Example usage:
    // System::Delay(1000);
    // UsbHid_KeyOn(0x04);      // press 'a'
    // System::Delay(50);
    // UsbHid_KeyOff(0x04);     // release 'a'
    // System::Delay(50);
    // UsbHid_ClearAllKeys();

    // Deliberately idle after enumeration.
    // This sample is intended as a clean HID bring-up base that can be imported
    // into another project, not as a self-typing demo loop.
    bool led = false;
    for(;;)
    {
        led = !led;
        hw.SetLed(led);
        System::Delay(500);
    }
}
