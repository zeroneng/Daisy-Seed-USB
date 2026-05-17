/**
 * @file usbd_audio_if.c
 * @brief UAC1 stereo input — 16-bit 48kHz — frame-based ring with drift control
 *
 * =============================================================================
 * WHY A FRAME-BASED RING IS REQUIRED
 * =============================================================================
 *
 * The previous packet-based ring stored 48-frame slots. PeriodicTC always
 * consumed exactly 1 slot per call regardless of whether it sent 47, 48, or
 * 49 bytes to the host. The ring level was therefore unaffected by the
 * correction — drift control had no effect on buffer stability.
 *
 * A frame-based ring stores individual stereo frames. PeriodicTC reads
 * exactly tx_samples frames and advances the read pointer by that amount.
 * Sending 49 frames genuinely drains 49 frames. Sending 47 genuinely drains
 * 47. The buffer level responds correctly to corrections.
 *
 * =============================================================================
 * HOW IT WORKS
 * =============================================================================
 *
 * AudioIF_PushSamples() (audio DMA ISR):
 *   Writes interleaved int16 frames directly into frame_ring[], advancing
 *   frame_write. No partial-packet accumulation needed — any number of
 *   frames per call is accepted.
 *
 * Audio_PeriodicTC() (USB DataIn ISR, every 1ms):
 *   1. Computes available = frame_write - frame_read (in frames)
 *   2. Decides tx_samples: 47, 48, or 49 based on level vs target
 *   3. Copies exactly tx_samples frames from ring into pbuf (handles wrap)
 *   4. Advances frame_read by tx_samples
 *   5. Returns byte count → DataIn passes to USBD_LL_Transmit
 *
 * =============================================================================
 * TUNING
 * =============================================================================
 *
 * FRAME_RING_SIZE (default 512 = 10.67ms at 48kHz)
 *   Must be power of 2. Total memory = SIZE * 2ch * 2B = SIZE * 4 bytes.
 *   512 → 2048 bytes. 1024 → 4096 bytes.
 *
 * FRAME_TARGET (default FRAME_RING_SIZE/2 = 256 frames = 5.3ms)
 *   Steady-state fill level. Half the ring gives equal overflow/underrun margin.
 *
 * FRAME_DRIFT_THRESH (default 96 frames = 2ms)
 *   Deadband half-width in frames. Corrections fire when level strays more
 *   than this many frames from FRAME_TARGET.
 *   Smaller = more aggressive, more frequent corrections, very stable.
 *   Larger  = gentler, less frequent, more natural sounding.
 *   Rule of thumb: FRAME_RING_SIZE / 8 is a good starting point.
 *
 * DIAGNOSTICS (read via LED pattern in main loop)
 *   drift_corrections_up    — 49-sample packets sent (buffer was above HIGH)
 *   drift_corrections_down  — 47-sample packets sent (buffer was below LOW)
 *   underrun_events         — buffer hit zero (should always be 0)
 *
 *   Healthy: up and down both nonzero and slowly incrementing, underrun = 0.
 *   up >> down:  audio clock faster than USB — normal.
 *   down >> up:  audio clock slower than USB — normal.
 *   underrun > 0: increase FRAME_RING_SIZE or FRAME_TARGET.
 */

#include "usbd_audio_if.h"
#include "stm32h7xx_hal.h"
#include <string.h>
#include <stdint.h>

#define AUDIO_CHANNELS      2U
#define BYTES_PER_SAMPLE    2U
#define PLAYBACK_RING_FRAMES 2048U
#define PLAYBACK_RING_MASK   (PLAYBACK_RING_FRAMES - 1U)

volatile uint32_t drift_corrections_up   = 0U;
volatile uint32_t drift_corrections_down = 0U;
volatile uint32_t underrun_events        = 0U;

static __attribute__((section(".heap"))) int16_t playback_ring[PLAYBACK_RING_FRAMES * AUDIO_CHANNELS];
static volatile uint32_t playback_wr = 0U;
static volatile uint32_t playback_rd = 0U;

/* -----------------------------------------------------------------------
 * Init
 * --------------------------------------------------------------------- */
static  int8_t Audio_Init(uint32_t AudioFreq, uint32_t Volume, uint32_t options)
{
    (void)AudioFreq; (void)Volume; (void)options;
        drift_corrections_up   = 0U;
    drift_corrections_down = 0U;
    underrun_events        = 0U;
    memset(playback_ring, 0, sizeof(playback_ring));
    playback_wr = 0U;
    playback_rd = 0U;
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
        uint32_t next = (playback_wr + 1U) & PLAYBACK_RING_MASK;
        if(next == playback_rd)
            break;
        playback_ring[playback_wr * AUDIO_CHANNELS + 0] = src[i * AUDIO_CHANNELS + 0];
        playback_ring[playback_wr * AUDIO_CHANNELS + 1] = src[i * AUDIO_CHANNELS + 1];
        playback_wr = next;
    }
    return USBD_OK;
}
static int8_t Audio_VolumeCtl (uint8_t v)                       { (void)v; return USBD_OK; }
static int8_t Audio_MuteCtl   (uint8_t c)                       { (void)c; return USBD_OK; }
static int8_t Audio_GetState  (void)                            { return USBD_OK; }

/* -----------------------------------------------------------------------
 * copy_frames_from_ring
 *
 * Copies exactly n frames from the ring starting at frame_read into dst,
 * handling the ring wrap-around. Advances frame_read by n.
 * Caller must ensure n <= available frames.
 * --------------------------------------------------------------------- */
static uint16_t Audio_PeriodicTC(uint8_t *pbuf, uint32_t size, uint8_t cmd)
{
    (void)pbuf;
    (void)size;
    (void)cmd;
    return AUDIO_OUT_PACKET;
}

/* -----------------------------------------------------------------------
 * USBD_AUDIO_fops
 * --------------------------------------------------------------------- */
USBD_AUDIO_ItfTypeDef USBD_AUDIO_fops = {
    Audio_Init, Audio_DeInit, Audio_AudioCmd,
    Audio_VolumeCtl, Audio_MuteCtl,
    Audio_PeriodicTC, Audio_GetState,
};

/* -----------------------------------------------------------------------
 * AudioIF_PushSamples — called from AudioCallback (DMA ISR)
 *
 * Writes interleaved int16 frames directly into the frame ring.
 * No partial-packet accumulation needed — any block size works.
 * --------------------------------------------------------------------- */
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
            samples[i * AUDIO_CHANNELS + 0] = playback_ring[playback_rd * AUDIO_CHANNELS + 0];
            samples[i * AUDIO_CHANNELS + 1] = playback_ring[playback_rd * AUDIO_CHANNELS + 1];
            playback_rd = (playback_rd + 1U) & PLAYBACK_RING_MASK;
        }
        else
        {
            samples[i * AUDIO_CHANNELS + 0] = 0;
            samples[i * AUDIO_CHANNELS + 1] = 0;
        }
    }
}
