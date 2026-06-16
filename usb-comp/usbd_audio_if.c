/**
 * @file usbd_audio_if.c
 * @brief UAC1 stereo audio bridge for the composite USB stack.
 *
 * USB capture reads from the app-owned AudioFifo_* float ring defined by
 * usb-comp.h or UsbComp.cpp. USB playback writes incoming host samples into
 * playback_ring, which the app drains with AudioIF_PopPlaybackSamples().
 */

#include "usbd_audio_if.h"
#include "stm32h7xx_hal.h"
#include <string.h>
#include <stdint.h>
#include "audio_fifo_shared.h"

#define AUDIO_CHANNELS      2U
#define BYTES_PER_SAMPLE    2U
#define USB_FRAMES_NOM      48U
#define USB_BYTES_NOM       (USB_FRAMES_NOM * AUDIO_CHANNELS * BYTES_PER_SAMPLE)
#define CAPTURE_DELAY_FRAMES (USB_FRAMES_NOM * 2U)

#if !defined(USB_COMP_AUDIO_PLAYBACK_RING_SIZE) \
    || ((USB_COMP_AUDIO_PLAYBACK_RING_SIZE + 0U) == 0U)
#undef USB_COMP_AUDIO_PLAYBACK_RING_SIZE
#define USB_COMP_AUDIO_PLAYBACK_RING_SIZE 512U
#endif

#if (USB_COMP_AUDIO_PLAYBACK_RING_SIZE < 64U) || ((USB_COMP_AUDIO_PLAYBACK_RING_SIZE & (USB_COMP_AUDIO_PLAYBACK_RING_SIZE - 1U)) != 0U)
#error "USB_COMP_AUDIO_PLAYBACK_RING_SIZE must be a power of two >= 64"
#endif

#define PLAYBACK_RING_FRAMES USB_COMP_AUDIO_PLAYBACK_RING_SIZE
#define PLAYBACK_RING_MASK   (PLAYBACK_RING_FRAMES - 1U)

static __attribute__((section(".heap"))) int16_t playback_ring[PLAYBACK_RING_FRAMES * AUDIO_CHANNELS];
static volatile uint32_t playback_wr = 0U;
static volatile uint32_t playback_rd = 0U;
static uint32_t capture_rd = 0U;
static uint8_t capture_rd_valid = 0U;

static float ClampAudioFloat(float sample)
{
    if(sample > 1.0f)
        return 1.0f;
    if(sample < -1.0f)
        return -1.0f;
    return sample;
}

static uint32_t CaptureStartDelay(uint32_t ring_size)
{
    uint32_t delay = CAPTURE_DELAY_FRAMES;
    const uint32_t max_delay = ring_size - USB_FRAMES_NOM;

    if(delay > max_delay)
        delay = max_delay;

    return delay;
}

static void CaptureResync(uint32_t write_pos, uint32_t ring_size)
{
    const uint32_t delay = CaptureStartDelay(ring_size);
    capture_rd = (write_pos > delay) ? (write_pos - delay) : 0U;
    capture_rd_valid = 1U;
}

/* -----------------------------------------------------------------------
 * Init
 * --------------------------------------------------------------------- */
static  int8_t Audio_Init(uint32_t AudioFreq, uint32_t Volume, uint32_t options)
{
    (void)AudioFreq; (void)Volume; (void)options;
    memset(playback_ring, 0, sizeof(playback_ring));
    playback_wr = 0U;
    playback_rd = 0U;
    capture_rd = 0U;
    capture_rd_valid = 0U;
    return USBD_OK;
}

static int8_t Audio_DeInit    (uint32_t o)                      { (void)o; return USBD_OK; }
static int8_t Audio_AudioCmd  (uint8_t *p,uint32_t s,uint8_t c)
{
    if(c != AUDIO_OUT_TC || p == NULL)
        return USBD_OK;
    uint32_t frames = s / (AUDIO_CHANNELS * BYTES_PER_SAMPLE);
    int16_t *src = (int16_t *)p;
    for(uint32_t i = 0; i < frames; i++)
    {
        if((playback_wr - playback_rd) >= PLAYBACK_RING_FRAMES)
            break;

        uint32_t write_idx = playback_wr & PLAYBACK_RING_MASK;
        playback_ring[write_idx * AUDIO_CHANNELS + 0] = src[i * AUDIO_CHANNELS + 0];
        playback_ring[write_idx * AUDIO_CHANNELS + 1] = src[i * AUDIO_CHANNELS + 1];
        playback_wr++;
    }
    return USBD_OK;
}
static int8_t Audio_VolumeCtl (uint8_t v)                       { (void)v; return USBD_OK; }
static int8_t Audio_MuteCtl   (uint8_t c)                       { (void)c; return USBD_OK; }
static int8_t Audio_GetState  (void)                            { return USBD_OK; }

static uint16_t Audio_PeriodicTC(uint8_t *pbuf, uint32_t size, uint8_t cmd)
{
    (void)size;

    if(cmd != AUDIO_IN_TC)
        return USB_BYTES_NOM;

    uint32_t out_frames = USB_FRAMES_NOM;
    uint32_t ring_size = AudioFifo_GetRingSize();
    uint32_t write_pos = AudioFifo_GetWritePtrLast();
    int16_t *out = (int16_t *)pbuf;

    if(ring_size <= USB_FRAMES_NOM)
    {
        memset(pbuf, 0, USB_BYTES_NOM);
        return USB_BYTES_NOM;
    }

    if(!capture_rd_valid)
        CaptureResync(write_pos, ring_size);

    uint32_t available = write_pos - capture_rd;
    if(available > (ring_size - USB_FRAMES_NOM))
    {
        CaptureResync(write_pos, ring_size);
        available = write_pos - capture_rd;
    }

    for(uint32_t i = 0; i < out_frames; i++)
    {
        if(available == 0U)
        {
            out[i * AUDIO_CHANNELS + 0] = 0;
            out[i * AUDIO_CHANNELS + 1] = 0;
            continue;
        }

        float l = ClampAudioFloat(AudioFifo_GetLeft(capture_rd));
        float r = ClampAudioFloat(AudioFifo_GetRight(capture_rd));

        int16_t sl = (int16_t)(l * 32767.0f);
        int16_t sr = (int16_t)(r * 32767.0f);
        out[i * AUDIO_CHANNELS + 0] = sl;
        out[i * AUDIO_CHANNELS + 1] = sr;

        capture_rd++;
        available--;
    }

    return (uint16_t)(out_frames * AUDIO_CHANNELS * BYTES_PER_SAMPLE);
}

/* -----------------------------------------------------------------------
 * USBD_AUDIO_fops
 * --------------------------------------------------------------------- */
USBD_AUDIO_ItfTypeDef USBD_AUDIO_fops = {
    Audio_Init, Audio_DeInit, Audio_AudioCmd,
    Audio_VolumeCtl, Audio_MuteCtl,
    Audio_PeriodicTC, Audio_GetState,
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
        if(playback_rd != playback_wr)
        {
            uint32_t read_idx = playback_rd & PLAYBACK_RING_MASK;
            samples[i * AUDIO_CHANNELS + 0] = playback_ring[read_idx * AUDIO_CHANNELS + 0];
            samples[i * AUDIO_CHANNELS + 1] = playback_ring[read_idx * AUDIO_CHANNELS + 1];
            playback_rd++;
        }
        else
        {
            samples[i * AUDIO_CHANNELS + 0] = 0;
            samples[i * AUDIO_CHANNELS + 1] = 0;
        }
    }
}
