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
- Registers a USB HID keyboard interface, with the firmware-generated key tap
  test disabled by default.
- Builds as a Daisy bootloader QSPI app with `APP_TYPE = BOOT_QSPI`.

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
```

The shared `usb-comp` bridge derives its `USB_COMP_ENABLE_*` gates from the
matching `USBD_CMPSIT_ACTIVATE_*` values, so application Makefiles only need to
define the `USBD_CMPSIT_ACTIVATE_*` set unless they intentionally need an
override.

7. Build and flash:

```bash
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make clean
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make -j2
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make program-dfu
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

## BasicUsb.cpp Firmware Changes

The `basic-usb` firmware has to call the shared `usb-comp` bridge from the app
code. These are the firmware-side changes that make the enabled USB functions
work.

Include the bridge header:

```cpp
#include "usb-comp.h"
```

Initialize audio at the USB audio rate, use a 48-sample callback block, start
the shared composite USB stack, then start the libDaisy audio callback:

```cpp
hw.Init();
hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
hw.SetAudioBlockSize(48);
UsbComp::Init();
hw.StartAudio(AudioCallback);
```

The main loop calls `UsbComp::Process()` before the LED delay. This gives the
shared bridge a foreground place to flush queued USB work, including the MIDI
echo response queued by the USB OUT callback:

```cpp
while(true)
{
    UsbComp::Process();
    hw.SetLed(led_state);
    ...
}
```

The audio callback is where analog audio, the test tone, USB capture, and USB
playback are connected. For each sample:

1. Create the local 100 Hz test tone.
2. Mix analog input plus the test tone into `capture_l` / `capture_r`.
3. Push that stereo pair into USB audio capture with `UsbComp::PushCapture()`.
4. Pop one stereo frame of host playback with `UsbComp::PopPlayback()`.
5. Mix host playback into the analog outputs.
6. After the block finishes, call `UsbComp::CommitCaptureBlock()` so the USB
   audio interface can see the new capture write pointer.

```cpp
void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        float signal = sinf(phase) * 0.5f;
        phase += kSignalIncrement;
        if(phase > M_TWOPI)
            phase -= M_TWOPI;

        const float capture_l = IN_L[i] + signal;
        const float capture_r = IN_R[i] + signal;

        UsbComp::PushCapture(capture_l, capture_r);

        float usb_l;
        float usb_r;
        UsbComp::PopPlayback(usb_l, usb_r);

        OUT_L[i] = capture_l + usb_l;
        OUT_R[i] = capture_r + usb_r;
    }

    UsbComp::CommitCaptureBlock();
}
```

The optional HID self-test is controlled by `USB_COMP_TEST_HID`. With the
default `basic-usb` build it is set to `0`, so the main loop only blinks the
LED and does not send keyboard reports:

```cpp
#if USB_COMP_TEST_HID
if(led_state)
{
    UsbComp::SetHidKeyA(true);
    System::Delay(40);
    UsbComp::SetHidKeyA(false);
    System::Delay(460);
}
else
{
    System::Delay(500);
}
#else
System::Delay(500);
#endif
```

MIDI in/out does not require a libDaisy audio callback. `basic-usb` enables the
MIDI class in the Makefile. The shared `usb-comp.h` bridge queues note-on and
note-off echo responses from the USB MIDI receive callback, and the main loop
flushes those responses from `UsbComp::Process()`.

## QSPI Boot

`basic-usb` is configured as a Daisy bootloader QSPI application:

```make
APP_TYPE = BOOT_QSPI
```

The standard libDaisy core Makefile maps that app type to:

```make
LDSCRIPT = $(LIBDAISY_DIR)/core/STM32H750IB_qspi.lds
FLASH_ADDRESS = 0x90040000
C_DEFS += -DBOOT_APP
```

Use the bootloader programming path for this build:

```bash
make clean
make -j2
make program-dfu
```

`make program` is intentionally only for non-bootloader internal-flash builds.
With `APP_TYPE = BOOT_QSPI`, use `program-dfu` after the Daisy bootloader is
running.

## USB Audio In/Out

USB audio is enabled by adding the shared `usbd_audio.c` and `usbd_audio_if.c`
sources, enabling the audio composite flag, and using the `UsbComp` audio
bridge inside `AudioCallback()`:

```make
$(USB_COMP_DIR)/usbd_audio.c
$(USB_COMP_DIR)/usbd_audio_if.c
-DUSBD_CMPSIT_ACTIVATE_AUDIO=1
-DUSB_COMP_TEST_AUDIO=1
-DUSB_COMP_AUDIO_START_ON_BOOT=1
```

The USB capture ring defaults to `16384` stereo frames in the shared bridge for
backward compatibility, but `basic-usb` validates the smaller SRAM-friendly
setting:

```make
-DUSB_COMP_AUDIO_CAPTURE_RING_SIZE=4096
-DUSB_COMP_AUDIO_USE_SDRAM=1
```

The ring stores left and right channels as `float`, so `16384` frames uses
128 KB. The `4096` frame setting uses 32 KB and saves 96 KB of SRAM. Keep the
value a power of two because the bridge uses a ring mask for wrapping.
`USB_COMP_AUDIO_USE_SDRAM=1` moves that capture ring from the normal SRAM-backed
`.heap` section to `.sdram_bss`, which keeps the app's 512 KB SRAM budget clear.
When libDaisy provides `DSY_SDRAM_BSS`, the bridge uses that macro and adds
32-byte alignment so the USB capture FIFO can coexist with larger application
sample pools in the same SDRAM section. Only use the SDRAM option after
`hw.Init()` has initialized external SDRAM.

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

Use the card/device number reported by `aplay -l` and `arecord -l`. On one Pi
test run the USB audio device appeared as `hw:3,0`. Record one second of the
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

Open one second of playback from the host to the Daisy, again using the actual
USB audio device number from the host:

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
-DUSB_COMP_TEST_MIDI=1
```

The shared `usb-comp.h` bridge registers the MIDI interface during
`UsbComp::Init()`. Incoming USB MIDI note-on and note-off packets queue an echo
one semitone higher. The main loop sends the queued response from
`UsbComp::Process()`, which gives a simple in/out validation without adding a
sequencer or UI to `basic-usb`.

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
-DUSB_COMP_TEST_HID=0
-DHID_FS_BINTERVAL=0x01U
```

The shared `usb-comp.h` bridge stores a 33-byte NKRO keyboard report, exposes
`UsbComp::SetHidKeyA(bool pressed)`, and sends reports through
`USBD_HID_SendReport()`. The current `basic-usb` default does not call it from
the main loop because `USB_COMP_TEST_HID=0`.

To intentionally re-enable the firmware-generated `A` tap test, set
`USB_COMP_TEST_HID=1` and use the guarded main-loop test block:

```cpp
#if USB_COMP_TEST_HID
UsbComp::SetHidKeyA(true);
System::Delay(40);
UsbComp::SetHidKeyA(false);
#endif
```

Host validation:

```bash
ls -l /dev/input/by-id | grep 'USB_Composite.*event-kbd'
```

With `USB_COMP_TEST_HID=0`, validation should confirm that the keyboard event
node exists and that no firmware-generated `KEY_A` press/release events appear
during the observation window.

Use the `/dev/input/event*` target from the `by-id` symlink:

```bash
python3 - <<'PY'
import os, struct, time
path = os.path.realpath('/dev/input/by-id/usb-Generic_USB_Composite_Sample_3958326E3432-event-kbd')
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
key_a_press=False
key_a_release=False
```
