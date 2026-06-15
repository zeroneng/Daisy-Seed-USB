#include "daisy_seed.h"
#include "CDC.h"

using namespace daisy;

DaisySeed hw;

int main(void)
{
    hw.Init();
    UsbCdc_Init();

    bool led = false;
    for(;;)
    {
        led = !led;
        hw.SetLed(led);
        UsbCdc_TransmitString(led ? "LED: ON\r\n" : "LED: OFF\r\n");
        System::Delay(250);
    }
}
