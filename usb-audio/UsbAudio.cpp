#define DSY_RAM_D2 __attribute__((section(".heap")))
#include "daisy_seed.h"
#include "usbd_audio_if.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;
using namespace std;

extern "C" {
    #include "usbd_core.h"
    #include "usbd_desc.h"
    #include "usbd_audio.h"
    extern USBD_HandleTypeDef hUsbDeviceHS;
}

DaisySeed hw;
Oscillator testTone;

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size)
{
    int16_t usb_buf[size * 2];

    for(size_t i = 0; i < size; i++)
    {
        float temp = testTone.Process();
        float l = in[0][i];
        float r = temp;

        if(l > 1.0f) l = 1.0f;
        if(l < -1.0f) l = -1.0f;
        if(r > 1.0f) r = 1.0f;
        if(r < -1.0f) r = -1.0f;

        usb_buf[i * 2 + 0] = (int16_t)(l * 32767.0f);
        usb_buf[i * 2 + 1] = (int16_t)(r * 32767.0f);

        out[0][i] = l;
        out[1][i] = r;
    }

    AudioIF_PushSamples(usb_buf, (uint32_t)size);
}

static void InitUSBAudio(void)
{
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
    USBD_RegisterClass(&hUsbDeviceHS, &USBD_AUDIO);
    USBD_AUDIO_RegisterInterface(&hUsbDeviceHS, &USBD_AUDIO_fops);
    USBD_Start(&hUsbDeviceHS);
}

int main(void)
{
    hw.Init();
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.SetAudioBlockSize(48);

    testTone.Init(48000.0f);
    testTone.SetWaveform(Oscillator::WAVE_SIN);
    testTone.SetFreq(440.0f);
    testTone.SetAmp(0.1f);

    InitUSBAudio();
    hw.StartAudio(AudioCallback);

    for(;;) {}
}
