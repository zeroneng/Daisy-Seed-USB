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

/* -----------------------------------------------------------------------
 * Audio geometry — must match descriptor in usbd_audio.c
 * --------------------------------------------------------------------- */
#define AUDIO_SAMPLE_RATE   48000U
#define AUDIO_CHANNELS      2U
#define BYTES_PER_SAMPLE    2U
#define SAMPLES_NOM         (AUDIO_SAMPLE_RATE / 1000U)            /* 48 */
#define SAMPLES_MAX         (SAMPLES_NOM + 1U)                     /* 49 */
#define SAMPLES_MIN         (SAMPLES_NOM - 1U)                     /* 47 */
#define BYTES_NOM           (SAMPLES_NOM * AUDIO_CHANNELS * BYTES_PER_SAMPLE)  /* 192 */
#define BYTES_MAX           (SAMPLES_MAX * AUDIO_CHANNELS * BYTES_PER_SAMPLE)  /* 196 */
#define BYTES_MIN           (SAMPLES_MIN * AUDIO_CHANNELS * BYTES_PER_SAMPLE)  /* 188 */

/* -----------------------------------------------------------------------
 * Tuning knobs — adjust these three values only
 * --------------------------------------------------------------------- */
#define FRAME_RING_SIZE     512U    /* ring depth in frames — power of 2, 512=10.67ms */
#define FRAME_TARGET        256U    /* target fill level = FRAME_RING_SIZE / 2        */
#define FRAME_DRIFT_THRESH   64U    /* deadband half-width in frames                  */

/* Derived watermarks — do not edit */
#define FRAME_DRIFT_HIGH    (FRAME_TARGET + FRAME_DRIFT_THRESH)   /* 320 */
#define FRAME_DRIFT_LOW     (FRAME_TARGET - FRAME_DRIFT_THRESH)   /* 192 */

/* -----------------------------------------------------------------------
 * Diagnostics counters
 * --------------------------------------------------------------------- */
volatile uint32_t drift_corrections_up   = 0U;
volatile uint32_t drift_corrections_down = 0U;
volatile uint32_t underrun_events        = 0U;

/* -----------------------------------------------------------------------
 * Frame ring buffer
 *
 * Stored as flat interleaved int16: [L0,R0, L1,R1, ... L511,R511]
 * Total size = 512 * 2 * 2 = 2048 bytes
 *
 * DSY_RAM_D2 = D2 SRAM (non-cacheable) — safe for USB DMA reads.
 * --------------------------------------------------------------------- */
static __attribute__((section(".heap"))) int16_t  frame_ring[FRAME_RING_SIZE * AUDIO_CHANNELS] ;

static volatile uint32_t frame_write = 0U;   /* next write position in frames */
static volatile uint32_t frame_read  = 0U;   /* next read  position in frames */
static          uint8_t  streaming_active = 0U;

/* -----------------------------------------------------------------------
 * Init
 * --------------------------------------------------------------------- */
static  int8_t Audio_Init(uint32_t AudioFreq, uint32_t Volume, uint32_t options)
{
    (void)AudioFreq; (void)Volume; (void)options;
    frame_write            = 0U;
    frame_read             = 0U;
    streaming_active       = 0U;
    drift_corrections_up   = 0U;
    drift_corrections_down = 0U;
    underrun_events        = 0U;
    memset(frame_ring, 0, sizeof(frame_ring));
    return USBD_OK;
}

static int8_t Audio_DeInit    (uint32_t o)                      { (void)o; return USBD_OK; }
static int8_t Audio_AudioCmd  (uint8_t *p,uint32_t s,uint8_t c) { (void)p;(void)s;(void)c; return USBD_OK; }
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
static void copy_frames_from_ring(uint8_t *dst, uint32_t n)
{
    uint32_t pos        = frame_read % FRAME_RING_SIZE;
    uint32_t to_end     = FRAME_RING_SIZE - pos;

    if(n <= to_end)
    {
        /* Contiguous — single memcpy */
        memcpy(dst, &frame_ring[pos * AUDIO_CHANNELS],
               n * AUDIO_CHANNELS * BYTES_PER_SAMPLE);
    }
    else
    {
        /* Wraps around — two memcpys */
        uint32_t first  = to_end;
        uint32_t second = n - first;
        memcpy(dst,
               &frame_ring[pos * AUDIO_CHANNELS],
               first * AUDIO_CHANNELS * BYTES_PER_SAMPLE);
        memcpy(dst + first * AUDIO_CHANNELS * BYTES_PER_SAMPLE,
               &frame_ring[0],
               second * AUDIO_CHANNELS * BYTES_PER_SAMPLE);
    }

    frame_read = (frame_read + n) % FRAME_RING_SIZE;
}

/* -----------------------------------------------------------------------
 * Audio_PeriodicTC — called from DataIn every ~1ms
 *
 * Reads exactly tx_samples frames from the frame ring. Because frame_read
 * advances by tx_samples, the ring level genuinely changes by that amount.
 * This is what makes 47/48/49 corrections actually work.
 * --------------------------------------------------------------------- */
static uint16_t Audio_PeriodicTC(uint8_t *pbuf, uint32_t size, uint8_t cmd)
{
    (void)size;
    if(cmd != AUDIO_IN_TC) return BYTES_NOM;

    uint32_t available = (frame_write - frame_read + FRAME_RING_SIZE) % FRAME_RING_SIZE;

    /* Pre-fill: hold silence until buffer reaches target */
    if(!streaming_active)
    {
        if(available >= (uint32_t)FRAME_TARGET)
            streaming_active = 1U;
        else {
            memset(pbuf, 0, BYTES_NOM);
            return BYTES_NOM;
        }
    }

    /* Underrun: buffer empty — reset and re-prime */
    if(available < SAMPLES_NOM)
    {
        streaming_active = 0U;
        underrun_events++;
        memset(pbuf, 0, BYTES_NOM);
        return BYTES_NOM;
    }

    /* Drift control — genuinely changes drain rate because frame_read
       advances by exactly tx_samples, not by a fixed slot size */
    uint32_t tx_samples = SAMPLES_NOM;

    if(available > FRAME_DRIFT_HIGH)
    {
        tx_samples = SAMPLES_MAX;        /* 49 — drain faster */
        drift_corrections_up++;
    }
    else if(available < FRAME_DRIFT_LOW)
    {
        tx_samples = SAMPLES_MIN;        /* 47 — drain slower */
        drift_corrections_down++;
    }

    /* Clamp to available frames (safety — should never be needed) */
    if(tx_samples > available)
        tx_samples = available;

    copy_frames_from_ring(pbuf, tx_samples);

    return (uint16_t)(tx_samples * AUDIO_CHANNELS * BYTES_PER_SAMPLE);
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
    for(uint32_t i = 0; i < num_frames; i++)
    {
        uint32_t pos = frame_write % FRAME_RING_SIZE;

        /* Check full — leave 1 slot gap between write and read */
        uint32_t next = (pos + 1U) % FRAME_RING_SIZE;
        if(next == (frame_read % FRAME_RING_SIZE)) return;  /* full — drop rest */

        frame_ring[pos * AUDIO_CHANNELS + 0] = samples[i * AUDIO_CHANNELS + 0];
        frame_ring[pos * AUDIO_CHANNELS + 1] = samples[i * AUDIO_CHANNELS + 1];

        frame_write = (frame_write + 1U) % FRAME_RING_SIZE;
    }
}
