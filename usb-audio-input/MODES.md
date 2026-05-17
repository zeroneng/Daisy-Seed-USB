# usb-audio proven modes

## 1) Input-only mode (`USB IN` to host)

What it is:
- The Daisy appears to the host as a USB audio **input/capture** device only.
- The host sees it in capture lists (`arecord -l` on Pi, audio input on macOS when accepted).

Signal path:
- `UsbAudio.cpp` audio callback generates a local sine wave.
- That generated stereo signal is written into an interleaved `usb_buf`.
- `AudioIF_PushSamples()` pushes those frames into the USB capture ring.
- `Audio_PeriodicTC()` drains the ring and hands packets to `USBD_AUDIO_DataIn()`.
- `USBD_AUDIO_DataIn()` transmits packets on `AUDIO_IN_EP` (`0x81`) to the host.

What the user experiences:
- The host sees the Daisy as an audio **source**.
- To observe the sine, the user selects the Daisy as an **input** device and monitors/records that input.
- This does **not** make the Daisy play sound locally unless the callback also writes to codec outputs.

Known-good host behavior:
- Pi: `arecord -l` shows `card 3: HS [JamMate Audio In HS], device 0: USB Audio`
- No playback endpoint is exposed in this mode.

## 2) Output-only mode (`USB OUT` from host)

What it is:
- The Daisy appears to the host as a USB audio **output/playback** device only.
- The host sees it in playback lists (`aplay -l` on Pi, audio output device on macOS).

Signal path:
- Host sends stereo PCM packets to the USB OUT endpoint.
- `USBD_AUDIO_DataOut()` receives USB packets.
- `Audio_AudioCmd()` stores those samples into a playback ring.
- `AudioIF_PopPlaybackSamples()` reads playback frames from that ring.
- `UsbAudio.cpp` audio callback writes those frames to the codec outputs.

What the user experiences:
- The host sees the Daisy as an audio **destination**.
- To hear sound, the host must select the Daisy as the **output** device and play audio into it.
- The Daisy codec output then plays what the host sends.

Known-good host behavior:
- Pi: `aplay -l` shows `card 3: HS [JamMate Audio In HS], device 0: USB Audio`
- No capture endpoint is exposed in this mode.
- Verified playback test: Pi opened `hw:3,0` and streamed a generated 440 Hz stereo sine successfully.

## Core takeaway

RCON's expectation was correct for the measurement goal:
- If the goal is **"I want the host to see the generated sine as an input signal"**, then the sine belongs in the **USB input/capture** path.
- If the goal is **"I want the Daisy to play what the host sends"**, then the host audio belongs in the **USB output/playback** path.

## Full-duplex merge target

The merged mode should do both at once:
- USB **input** path sends the locally generated sine wave to the host input meter/recorder.
- USB **output** path accepts host playback audio and routes it to the codec outputs.

That means the two paths should remain logically independent:
- Capture path: callback-generated sine -> USB IN ring -> host
- Playback path: host USB OUT -> playback ring -> codec outputs
