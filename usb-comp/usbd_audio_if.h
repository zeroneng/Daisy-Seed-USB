#ifndef USBD_AUDIO_IF_H
#define USBD_AUDIO_IF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_audio.h"

extern USBD_AUDIO_ItfTypeDef USBD_AUDIO_fops;

/**
 * @brief Push one audio block from the Daisy audio callback into the USB ring buffer.
 * @param samples    Interleaved L/R int16 samples: [L0,R0,L1,R1,...]
 * @param num_frames Number of stereo frames (= DSP block size, e.g. 48)
 * @note  ISR-safe. Call every AudioCallback.
 */
void AudioIF_PushSamples(int16_t *samples, uint32_t num_frames);
void AudioIF_PopPlaybackSamples(int16_t *samples, uint32_t num_frames);

/**
 * Diagnostic counters — read these in your main loop to observe drift control.
 *
 * drift_corrections_up   — times a 49-sample packet was sent (buffer high)
 * drift_corrections_down — times a 47-sample packet was sent (buffer low)
 * underrun_events        — times buffer hit zero (should always be 0)
 *
 * Healthy state: up and down both nonzero, underrun = 0.
 */
extern volatile uint32_t drift_corrections_up;
extern volatile uint32_t drift_corrections_down;
extern volatile uint32_t underrun_events;

#ifdef __cplusplus
}
#endif

#endif /* USBD_AUDIO_IF_H */
