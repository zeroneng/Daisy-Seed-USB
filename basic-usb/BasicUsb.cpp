#include "daisy_seed.h"
#include <cmath>

using namespace daisy;

DaisySeed hw;

const float kSignalIncrement = (M_TWOPI * 100.0f) * (1.0f / 48000.0f);

float phase = 0.0f;

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        float signal = sinf(phase) * 0.5f;
        phase += kSignalIncrement;
        if(phase > M_TWOPI)
            phase -= M_TWOPI;

        OUT_L[i] = IN_L[i] + signal;
        OUT_R[i] = IN_R[i] + signal;
    }
}

int main()
{
    hw.Init();
    hw.StartAudio(AudioCallback);

    bool led_state = false;

    while(true)
    {
        hw.SetLed(led_state);
        led_state = !led_state;
        System::Delay(500);
    }
}
