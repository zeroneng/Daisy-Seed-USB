#include "usbd_audio_if.h"
#include "stm32h7xx_hal.h"
#include <string.h>
#include <stdint.h>
#include <math.h>

#define USB_SAMPLE_RATE     48000.0f
#define AUDIO_CHANNELS      2U
#define BYTES_PER_SAMPLE    2U
#define USB_FRAMES_NOM      48U
#define USB_FRAMES_MAX      48U

#ifndef M_TWOPI
#define M_TWOPI 6.28318530717958647692f
#endif

volatile uint32_t drift_corrections_up   = 0U;
volatile uint32_t drift_corrections_down = 0U;
volatile uint32_t underrun_events        = 0U;
volatile uint32_t usb_packets_sent       = 0U;
volatile uint32_t usb_last_frames        = USB_FRAMES_NOM;
volatile uint32_t usb_last_size_arg      = 0U;
volatile uint32_t usb_last_cmd_arg       = 0U;
volatile uint32_t usb_min_size_arg       = 0xffffffffU;
volatile uint32_t usb_max_size_arg       = 0U;
volatile uint32_t usb_size_probe         = 0U;

static float phase = 0.0f;
static float phase_inc = 0.0f;
static float amplitude = 0.1f;
static uint32_t usb_frame_toggle = 0U;

static int8_t Audio_Init(uint32_t AudioFreq, uint32_t Volume, uint32_t options)
{
    (void)AudioFreq;
    (void)Volume;
    (void)options;
    phase = 0.0f;
    phase_inc = M_TWOPI * 100.0f / USB_SAMPLE_RATE;
    amplitude = 0.1f;
    usb_frame_toggle = 0U;
    usb_packets_sent = 0U;
    usb_last_frames = USB_FRAMES_NOM;
    usb_last_size_arg = 0U;
    usb_last_cmd_arg = 0U;
    usb_min_size_arg = 0xffffffffU;
    usb_max_size_arg = 0U;
    usb_size_probe = 0U;
    drift_corrections_up = 0U;
    drift_corrections_down = 0U;
    underrun_events = 0U;
    return USBD_OK;
}

static int8_t Audio_DeInit(uint32_t o)                      { (void)o; return USBD_OK; }
static int8_t Audio_AudioCmd(uint8_t *p,uint32_t s,uint8_t c) { (void)p;(void)s;(void)c; return USBD_OK; }
static int8_t Audio_VolumeCtl(uint8_t v)                   { (void)v; return USBD_OK; }
static int8_t Audio_MuteCtl(uint8_t c)                     { (void)c; return USBD_OK; }
static int8_t Audio_GetState(void)                         { return USBD_OK; }

static uint16_t Audio_PeriodicTC(uint8_t *pbuf, uint32_t size, uint8_t cmd)
{
    usb_last_size_arg = size;
    usb_size_probe = size;
    usb_last_cmd_arg = cmd;
    if(size < usb_min_size_arg)
        usb_min_size_arg = size;
    if(size > usb_max_size_arg)
        usb_max_size_arg = size;

    if(cmd != AUDIO_IN_TC)
        return (USB_FRAMES_NOM * AUDIO_CHANNELS * BYTES_PER_SAMPLE);

    uint32_t out_frames = (usb_frame_toggle % 10U == 0U) ? USB_FRAMES_MAX : USB_FRAMES_NOM;
    usb_frame_toggle++;
    usb_last_frames = out_frames;

    int16_t *out = (int16_t *)pbuf;
    for(uint32_t i = 0; i < out_frames; i++)
    {
        float s = sinf(phase) * amplitude;
        phase += phase_inc;
        if(phase >= M_TWOPI)
            phase -= M_TWOPI;

        int16_t sample = (int16_t)(s * 32767.0f);
        out[i * AUDIO_CHANNELS + 0] = sample;
        out[i * AUDIO_CHANNELS + 1] = sample;
    }

    usb_packets_sent++;
    return (uint16_t)(out_frames * AUDIO_CHANNELS * BYTES_PER_SAMPLE);
}

USBD_AUDIO_ItfTypeDef USBD_AUDIO_fops = {
    Audio_Init,
    Audio_DeInit,
    Audio_AudioCmd,
    Audio_VolumeCtl,
    Audio_MuteCtl,
    Audio_PeriodicTC,
    Audio_GetState,
};

void AudioIF_PushSamples(int16_t *samples, uint32_t num_frames)
{
    (void)samples;
    (void)num_frames;
}

void AudioIF_PopPlaybackSamples(int16_t *samples, uint32_t num_frames)
{
    for(uint32_t i = 0; i < num_frames; i++)
    {
        samples[i * AUDIO_CHANNELS + 0] = 0;
        samples[i * AUDIO_CHANNELS + 1] = 0;
    }
}
