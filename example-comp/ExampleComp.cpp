#include "daisy_seed.h"
#include "usb-comp.h"

#include <cmath>
#include <cstdio>

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
constexpr uint32_t kUsbTestPeriodMs   = 1000;
constexpr uint32_t kHidPressMs        = 40;
constexpr float    kSignalIncrement   = (kTwoPi * kTestToneHz) / kSampleRateHz;
constexpr uint32_t kMidiRxQueueSize   = 16;
constexpr uint32_t kMidiRxQueueMask   = kMidiRxQueueSize - 1;

DaisySeed hw;

struct UsbMidiEvent
{
    uint8_t cin;
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
};

float phase = 0.0f;
volatile uint32_t midi_rx_write = 0;
volatile uint32_t midi_rx_read  = 0;
UsbMidiEvent midi_rx_queue[kMidiRxQueueSize] = {};

void OnUsbMidi(uint8_t cin, uint8_t status, uint8_t data1, uint8_t data2)
{
    const uint32_t write = midi_rx_write;
    const uint32_t next  = (write + 1) & kMidiRxQueueMask;
    if(next == midi_rx_read)
        return;

    midi_rx_queue[write] = {cin, status, data1, data2};
    midi_rx_write        = next;
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
    UsbComp::SetMidiReceiveCallback(OnUsbMidi);
    hw.StartAudio(AudioCallback);

    bool led_state = false;
    bool midi_note_on = false;
    uint32_t last_led_ms = System::GetNow();
    uint32_t last_usb_test_ms = last_led_ms;

    while(true)
    {
        UsbComp::Process();

        while(midi_rx_read != midi_rx_write)
        {
            const UsbMidiEvent event = midi_rx_queue[midi_rx_read];
            midi_rx_read = (midi_rx_read + 1) & kMidiRxQueueMask;

            char line[64];
            std::snprintf(line,
                          sizeof(line),
                          "MIDI RX %02X %02X %02X %02X\r\n",
                          event.cin,
                          event.status,
                          event.data1,
                          event.data2);
            UsbComp::SendCdc(line);
        }

        const uint32_t now = System::GetNow();
        if(now - last_led_ms >= kLedPeriodMs)
        {
            last_led_ms = now;
            led_state = !led_state;
            hw.SetLed(led_state);
        }

        if(now - last_usb_test_ms >= kUsbTestPeriodMs)
        {
            last_usb_test_ms = now;

            const uint8_t note = 60;
            UsbComp::SendMidiPacket(midi_note_on ? 0x08 : 0x09,
                                    midi_note_on ? 0x80 : 0x90,
                                    note,
                                    midi_note_on ? 0 : 100);
            midi_note_on = !midi_note_on;

#if USB_COMP_TEST_HID
            const uint8_t key = UsbComp::CharToKeycode('a');
            UsbComp::ClearAllKeys();
            UsbComp::SetHidKeyState(key, true);
            UsbComp::SendHidReport();
            System::Delay(kHidPressMs);
            UsbComp::SetHidKeyState(key, false);
            UsbComp::SendHidReport();
#endif
        }
    }
}
