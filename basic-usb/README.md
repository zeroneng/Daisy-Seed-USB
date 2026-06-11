# basic-usb

Minimal Daisy Seed blink plus audio callback app.

- Initializes the Daisy Seed.
- Toggles the onboard LED every 500 ms.
- Result: one full blink cycle per second.
- Starts a libDaisy audio callback.
- Outputs a 100 Hz sine tone on left and right audio outputs.
- Passes line input straight to the matching output channel.
- Starts the shared `usb-comp` USB stack with CDC, HID, USB audio, and MIDI enabled.
- Sends analog input plus the 100 Hz test tone to USB audio capture.
- Mixes USB audio playback into the analog outputs.
- Echoes incoming USB MIDI note messages back out one semitone higher.
- Sends a short HID keyboard `A` press/release once per full LED cycle.

## Exact CDC Bring-Up Steps

1. Keep `basic-usb` as the application folder and reuse the shared sources from `../usb-comp`.
2. Include the bridge header in `BasicUsb.cpp`:

```cpp
#include "usb-comp.h"
```

3. Initialize the Daisy Seed, set the USB/audio callback rate, then start the shared USB stack before audio starts:

```cpp
hw.Init();
hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
hw.SetAudioBlockSize(48);
UsbComp::Init();
hw.StartAudio(AudioCallback);
```

4. Compile the shared USB core, descriptor, composite builder, CDC interface, and CDC class sources from `../usb-comp` and its vendored ST USB Device tree.
5. Add these include paths before the shared standalone makefile is included:

```make
-I$(USB_COMP_DIR)
-I$(ST_USB_DEVICE_DIR)/Core/Inc
-I$(ST_USB_DEVICE_DIR)/Class/CDC/Inc
-I$(ST_USB_DEVICE_DIR)/Class/CompositeBuilder/Inc
```

6. For CDC-only bring-up, use these feature flags:

```make
-DUSBD_CMPSIT_ACTIVATE_CDC=1
-DUSBD_CMPSIT_ACTIVATE_HID=0
-DUSBD_CMPSIT_ACTIVATE_AUDIO=0
-DUSBD_CMPSIT_ACTIVATE_MIDI=0
-DUSBD_CMPSIT_ACTIVATE_MSC=0
-DUSB_COMP_ENABLE_CDC=1
-DUSB_COMP_ENABLE_HID=0
-DUSB_COMP_ENABLE_AUDIO=0
-DUSB_COMP_ENABLE_MIDI=0
-DUSB_COMP_ENABLE_MSC=0
```

7. Build and flash:

```bash
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make clean
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make -j2
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make program
```

8. Confirm the board enumerates:

```bash
lsusb | grep '0483:5764'
ls -l /dev/serial/by-id | grep 'USB_Composite'
```

9. Confirm CDC data works:

```bash
stty -F /dev/ttyACM1 115200 raw -echo -echoe -echok
timeout 3 cat /dev/ttyACM1 > /tmp/basic-usb-cdc.out &
printf 'x' > /dev/ttyACM1
cat /tmp/basic-usb-cdc.out
```

Expected response:

```text
COMP CDC RX
```

## USB Audio In/Out

USB audio is enabled by adding the shared `usbd_audio.c` and `usbd_audio_if.c`
sources, enabling `USB_COMP_ENABLE_AUDIO`, and using the `UsbComp` audio bridge
inside `AudioCallback()`:

```make
$(USB_COMP_DIR)/usbd_audio.c
$(USB_COMP_DIR)/usbd_audio_if.c
-DUSBD_CMPSIT_ACTIVATE_AUDIO=1
-DUSB_COMP_ENABLE_AUDIO=1
-DUSB_COMP_TEST_AUDIO=1
-DUSB_COMP_AUDIO_START_ON_BOOT=1
```

```cpp
UsbComp::PushCapture(capture_l, capture_r);
UsbComp::PopPlayback(usb_l, usb_r);
OUT_L[i] = capture_l + usb_l;
OUT_R[i] = capture_r + usb_r;
UsbComp::CommitCaptureBlock();
```

The firmware sends analog input plus the 100 Hz test tone to USB capture. USB
playback from the host is mixed into the analog outputs.

Host validation:

```bash
aplay -l | grep -A1 'USB Composite Sample'
arecord -l | grep -A1 'USB Composite Sample'
```

On this Pi the USB audio device appeared as `hw:3,0`. Record one second of the
Daisy capture stream:

```bash
rm -f /tmp/basic-usb-capture.wav
arecord -D hw:3,0 -f S16_LE -c 2 -r 48000 -d 1 /tmp/basic-usb-capture.wav
ls -lh /tmp/basic-usb-capture.wav
```

Expected result:

```text
188K /tmp/basic-usb-capture.wav
```

Open one second of playback from the host to the Daisy:

```bash
speaker-test -D hw:3,0 -c 2 -r 48000 -F S16_LE -t sine -f 440 -l 1
```

A passing test opens the playback stream without ALSA errors. The current
firmware mixes that USB playback stream into the analog outputs.

## USB MIDI In/Out

USB MIDI is enabled by adding the shared `usbd_midi.c` source and enabling the
MIDI composite flags:

```make
$(USB_COMP_DIR)/usbd_midi.c
-DUSBD_CMPSIT_ACTIVATE_MIDI=1
-DUSB_COMP_ENABLE_MIDI=1
-DUSB_COMP_TEST_MIDI=1
```

The shared `usb-comp.h` bridge registers the MIDI interface during
`UsbComp::Init()`. Incoming USB MIDI note-on and note-off packets are echoed
back to the host one semitone higher, which gives a simple in/out validation
without adding a sequencer or UI to `basic-usb`.

Host validation:

```bash
aconnect -l
amidi -l
```

Then open a MIDI capture and send a note packet to the Daisy MIDI raw port. The
expected returned packet for note-on `90 3C 40` is `90 3D 40`.

```bash
rm -f /tmp/basic-usb-midi.bin
timeout 3 amidi -p hw:3,0,0 -r /tmp/basic-usb-midi.bin &
sleep 0.5
amidi -p hw:3,0,0 -S '90 3C 40'
sleep 1
od -An -tx1 /tmp/basic-usb-midi.bin
```

Expected output:

```text
90 3d 40
```

## USB HID Keyboard

USB HID is enabled by adding the shared NKRO keyboard class source, adding the
vendored HID include path, and enabling the HID composite flags:

```make
$(USB_COMP_DIR)/usbd_hid_kbd.c
-I$(USB_COMP_DIR)/vendor/hid/Inc
-DUSBD_CMPSIT_ACTIVATE_HID=1
-DUSB_COMP_ENABLE_HID=1
-DUSB_COMP_TEST_HID=1
-DHID_FS_BINTERVAL=0x01U
```

The shared `usb-comp.h` bridge stores a 33-byte NKRO keyboard report, exposes
`UsbComp::SetHidKeyA(bool pressed)`, and sends reports through
`USBD_HID_SendReport()`. `basic-usb` calls it once per full LED cycle:

```cpp
UsbComp::SetHidKeyA(true);
System::Delay(40);
UsbComp::SetHidKeyA(false);
```

Host validation:

```bash
ls -l /dev/input/by-id | grep 'USB_Composite.*event-kbd'
```

Read the matching `/dev/input/event*` node while the firmware sends the test
tap. A passing test shows `KEY_A` press and release events.

On this Pi the HID keyboard appeared as `/dev/input/event4`, and the test used:

```bash
python3 - <<'PY'
import os, struct, time
path = '/dev/input/event4'
fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
end = time.time() + 4
seen = []
fmt = 'llHHI'
size = struct.calcsize(fmt)
while time.time() < end:
    try:
        data = os.read(fd, size * 32)
    except BlockingIOError:
        time.sleep(0.02)
        continue
    for off in range(0, len(data) - size + 1, size):
        sec, usec, etype, code, value = struct.unpack(fmt, data[off:off + size])
        if etype == 1:
            seen.append((code, value))
os.close(fd)
print('events=' + ','.join(f'{code}:{value}' for code, value in seen[:20]))
print('key_a_press=' + str((30, 1) in seen))
print('key_a_release=' + str((30, 0) in seen))
PY
```

Expected result:

```text
key_a_press=True
key_a_release=True
```
