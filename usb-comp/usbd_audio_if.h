#ifndef USBD_AUDIO_IF_H
#define USBD_AUDIO_IF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_audio.h"

extern USBD_AUDIO_ItfTypeDef USBD_AUDIO_fops;

/**
 * @brief Compatibility hook for older packet-ring code.
 *
 * Current usb-comp capture uses the AudioFifo_* functions supplied by
 * usb-comp.h or UsbComp.cpp, so this function is intentionally a no-op.
 */
void AudioIF_PushSamples(int16_t *samples, uint32_t num_frames);

/**
 * @brief Pop stereo int16 playback samples received from the USB host.
 */
void AudioIF_PopPlaybackSamples(int16_t *samples, uint32_t num_frames);

#ifdef __cplusplus
}
#endif

#endif /* USBD_AUDIO_IF_H */
