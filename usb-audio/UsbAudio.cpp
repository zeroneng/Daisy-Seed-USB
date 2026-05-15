#define DSY_RAM_D2 __attribute__((section(".heap")))
#include <math.h>
#include "daisy_seed.h"
#include "usbd_audio_if.h"

using namespace daisy;

extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_audio.h"
extern USBD_HandleTypeDef hUsbDeviceHS;
}

#ifndef M_TWOPI
#define M_TWOPI 6.28318530717958647692f
#endif

DaisySeed hw;

static constexpr uint32_t FIFO_RING_SIZE = 8192u;
static constexpr uint32_t FIFO_RING_MASK = FIFO_RING_SIZE - 1u;
static float fifo_l[FIFO_RING_SIZE] DSY_RAM_D2;
static float fifo_r[FIFO_RING_SIZE] DSY_RAM_D2;
static uint32_t fifoWritePtr = 0u;

static float phase = 0.0f;
static float phase_inc = 0.0f;
static float amplitude = 0.1f;

void AudioCallback(AudioHandle::InputBuffer in,
                   AudioHandle::OutputBuffer out,
                   size_t size)
{
    (void)in;
    for(size_t i = 0; i < size; i++)
    {
        float s = sinf(phase) * amplitude;
        phase += phase_inc;
        if(phase >= M_TWOPI)
            phase -= M_TWOPI;

        fifo_l[fifoWritePtr] = s;
        fifo_r[fifoWritePtr] = s;
        fifoWritePtr = (fifoWritePtr + 1u) & FIFO_RING_MASK;

        out[0][i] = s;
        out[1][i] = s;
    }
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

    phase = 0.0f;
    phase_inc = M_TWOPI * 100.0f / 48000.0f;
    amplitude = 0.1f;
    fifoWritePtr = 0u;
    for(uint32_t i = 0; i < FIFO_RING_SIZE; i++)
    {
        fifo_l[i] = 0.0f;
        fifo_r[i] = 0.0f;
    }

    InitUSBAudio();
    hw.StartAudio(AudioCallback);

    for(;;) {}
}
