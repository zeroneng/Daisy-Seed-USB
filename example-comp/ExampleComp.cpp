#include "daisy_seed.h"
#include "usb-comp.h"

#include <cmath>

using namespace daisy;

#ifndef USB_COMP_TEST_HID
#define USB_COMP_TEST_HID 0
#endif

namespace
{
#ifndef M_TWOPI
constexpr float kTwoPi = 6.28318530717958647692f;
#else
constexpr float kTwoPi = M_TWOPI;
#endif

constexpr float    kSampleRateHz      = 48000.0f;
constexpr float    kTestToneHz        = 100.0f;
constexpr float    kTestToneGain      = 0.5f;
constexpr size_t   kAudioBlockSize    = 48;
constexpr uint32_t kLedPeriodMs       = 500;
constexpr uint32_t kHidPressMs        = 40;
constexpr uint32_t kHidReleaseMs      = kLedPeriodMs - kHidPressMs;
constexpr float    kSignalIncrement   = (kTwoPi * kTestToneHz) / kSampleRateHz;

DaisySeed hw;

float phase = 0.0f;

void RunHidSelfTest(bool led_state)
{
#if USB_COMP_TEST_HID
    if(!led_state)
    {
        System::Delay(kLedPeriodMs);
        return;
    }

    UsbComp::SetHidKeyA(true);
    System::Delay(kHidPressMs);
    UsbComp::SetHidKeyA(false);
    System::Delay(kHidReleaseMs);
#else
    (void)led_state;
    System::Delay(kLedPeriodMs);
#endif
}
} // namespace

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        const float signal = sinf(phase) * kTestToneGain;
        phase += kSignalIncrement;
        if(phase > kTwoPi)
            phase -= kTwoPi;

        const float capture_l = IN_L[i] + signal;
        const float capture_r = IN_R[i] + signal;

        UsbComp::PushCapture(capture_l, capture_r);

        float usb_l = 0.0f;
        float usb_r = 0.0f;
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
    hw.SetAudioBlockSize(kAudioBlockSize);
    UsbComp::Init();
    hw.StartAudio(AudioCallback);

    bool led_state = false;

    while(true)
    {
        UsbComp::Process();
        hw.SetLed(led_state);
        RunHidSelfTest(led_state);
        led_state = !led_state;
    }
}
