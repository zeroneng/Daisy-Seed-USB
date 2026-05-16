#include "daisy_seed.h"

using namespace daisy;

int main(void)
{
    DaisySeed hw;
    hw.Init();

    bool led_state = false;
    uint32_t blink_count = 0;
    const uint32_t blink_interval_ms = 250;

    while(1)
    {
        led_state = !led_state;
        hw.SetLed(led_state);
        blink_count++;

        if(blink_count == 20)
        {
            hw.SetLed(true);
            System::Delay(2000);
            hw.SetLed(false);
            System::Delay(50);
            System::ResetToBootloader(System::BootloaderMode::STM);
        }

        System::Delay(blink_interval_ms);
    }
}
