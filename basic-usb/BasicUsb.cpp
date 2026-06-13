#include "daisy_seed.h"
#include "usb-comp.h"
#include <cmath>

using namespace daisy;

#ifndef USB_COMP_TEST_HID
#define USB_COMP_TEST_HID 0
#endif

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

        const float capture_l = IN_L[i] + signal;
        const float capture_r = IN_R[i] + signal;

        UsbComp::PushCapture(capture_l, capture_r);

        float usb_l;
        float usb_r;
        UsbComp::PopPlayback(usb_l, usb_r);

        OUT_L[i] = capture_l + usb_l;
        OUT_R[i] = capture_r + usb_r;
    }

    UsbComp::CommitCaptureBlock();
}

int main()
{
    hw.Init();
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.SetAudioBlockSize(48);
    UsbComp::Init();
    hw.StartAudio(AudioCallback);

    bool led_state = false;

    while(true)
    {
        UsbComp::Process();
        hw.SetLed(led_state);
#if USB_COMP_TEST_HID
        if(led_state)
        {
            UsbComp::SetHidKeyA(true);
            System::Delay(40);
            UsbComp::SetHidKeyA(false);
            System::Delay(460);
        }
        else
        {
            System::Delay(500);
        }
#else
        System::Delay(500);
#endif
        led_state = !led_state;
    }
}
