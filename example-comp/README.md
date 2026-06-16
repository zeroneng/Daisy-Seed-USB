# example-comp

Minimal Daisy Seed app using `../usb-comp`.

It blinks the LED, runs a 48 kHz audio callback, starts composite USB, sends a
100 Hz test tone to analog and USB audio capture, mixes USB playback into analog
output, echoes MIDI notes one semitone higher, and sends a repeating HID `A`
key tap by default.

USB profile:

- CDC ACM serial
- NKRO HID keyboard
- USB audio capture/playback
- USB MIDI

## Files

- `ExampleComp.cpp` - small app using `usb-comp.h`
- `Makefile` - explicit `../usb-comp` source/include list
- `README.md`

## App Steps

```cpp
#include "daisy_seed.h"
#include "usb-comp.h"

DaisySeed hw;

int main()
{
    hw.Init();
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
    hw.SetAudioBlockSize(48);

    UsbComp::Init();
    hw.StartAudio(AudioCallback);

    while(true)
    {
        UsbComp::Process();
        System::Delay(1);
    }
}
```

In the audio callback:

```cpp
UsbComp::PushCapture(capture_l, capture_r);
UsbComp::PopPlayback(usb_l, usb_r);
OUT_L[i] = capture_l + usb_l;
OUT_R[i] = capture_r + usb_r;
UsbComp::CommitCaptureBlock();
```

`UsbComp::Process()` must run in the foreground loop so queued MIDI responses
and other deferred USB work can flush outside USB callbacks.

## Integration Check

`../usb-comp/usbd_conf.c` must be part of the app build. It owns the ST USB
device handles:

```c
USBD_HandleTypeDef hUsbDeviceFS;
USBD_HandleTypeDef hUsbDeviceHS;
```

This matters because `hUsbDeviceHS` is also present in libDaisy's older USB
device layer. If an app only declares the handle as `extern` and does not link
the `usb-comp` definition, the linker can satisfy the symbol from
`libDaisy/build/libdaisy.a(usb.o)`. That silently pulls in the old USB owner and
can make composite init lock up or behave differently from this example.

When porting this example into another app, check the final map file:

```bash
grep -E 'hUsbDevice(HS|FS)|hpcd_USB_OTG_HS' build/*.map
```

Expected result: the USB device and PCD handles should come from the local
`usbd_conf.o` built from `../usb-comp`, not from `libdaisy.a(usb.o)`.

## Makefile Steps

```make
USB_COMP_DIR = ../usb-comp
ST_USB_DEVICE_DIR = $(USB_COMP_DIR)/vendor/stm32_mw_usb_device
USBD_CMPSIT_ACTIVATE_CDC ?= 1
USBD_CMPSIT_ACTIVATE_HID ?= 1
USBD_CMPSIT_ACTIVATE_AUDIO ?= 1
USBD_CMPSIT_ACTIVATE_MIDI ?= 1

C_SOURCES += \
$(USB_COMP_DIR)/usbd_conf.c \
$(USB_COMP_DIR)/usbd_desc.c \
$(USB_COMP_DIR)/usbd_composite_builder.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_core.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_ctlreq.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_ioreq.c

ifeq ($(USBD_CMPSIT_ACTIVATE_CDC),1)
C_SOURCES += \
$(USB_COMP_DIR)/usb_comp_cdc_if.c \
$(ST_USB_DEVICE_DIR)/Class/CDC/Src/usbd_cdc.c
endif

ifeq ($(USBD_CMPSIT_ACTIVATE_HID),1)
C_SOURCES += \
$(USB_COMP_DIR)/usbd_hid_kbd.c
endif

ifeq ($(USBD_CMPSIT_ACTIVATE_AUDIO),1)
C_SOURCES += \
$(USB_COMP_DIR)/usbd_audio.c \
$(USB_COMP_DIR)/usbd_audio_if.c
endif

ifeq ($(USBD_CMPSIT_ACTIVATE_MIDI),1)
C_SOURCES += \
$(USB_COMP_DIR)/usbd_midi.c
endif

C_INCLUDES += \
-I$(USB_COMP_DIR) \
-I$(USB_COMP_DIR)/vendor/hid/Inc \
-I$(ST_USB_DEVICE_DIR)/Core/Inc \
-I$(ST_USB_DEVICE_DIR)/Class/CDC/Inc \
-I$(ST_USB_DEVICE_DIR)/Class/CompositeBuilder/Inc

C_DEFS += \
-DUSE_USBD_COMPOSITE \
-DUSBD_CMPSIT_ACTIVATE_CDC=$(USBD_CMPSIT_ACTIVATE_CDC) \
-DUSBD_CMPSIT_ACTIVATE_HID=$(USBD_CMPSIT_ACTIVATE_HID) \
-DUSBD_CMPSIT_ACTIVATE_AUDIO=$(USBD_CMPSIT_ACTIVATE_AUDIO) \
-DUSBD_CMPSIT_ACTIVATE_MIDI=$(USBD_CMPSIT_ACTIVATE_MIDI) \
-DUSB_COMP_TEST_CDC=1 \
-DUSB_COMP_TEST_HID=1 \
-DUSB_COMP_TEST_AUDIO=1 \
-DUSB_COMP_TEST_MIDI=1 \
-DHID_FS_BINTERVAL=0x01U
```

Omit `USB_COMP_AUDIO_CAPTURE_RING_SIZE` and `USB_COMP_AUDIO_PLAYBACK_RING_SIZE`
for the default `512` frame capture ring and `512` frame playback ring. Only
define one when overriding it.

## Audio Defaults

- Audio rate: 48 kHz
- Audio block size: 48 samples
- USB audio packet: 48 stereo frames / 192 bytes every 1 ms
- Capture ring: 512 stereo float frames / 4096 bytes in SRAM
- Playback ring: 512 usable stereo int16 frames / 2048 bytes in SRAM
- Capture ring sizes must be powers of two and at least 128 frames
- Playback ring sizes must be powers of two and at least 64 frames

## Build And Flash

```bash
cd /mnt/clu-nas/developer/daisy-usb/example-comp
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make clean
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make -j2
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make program
```

## Host Test

```bash
lsusb | grep '0483:5764'
ls -l /dev/serial/by-id | grep 'USB_Composite'
ls -l /dev/input/by-id | grep 'USB_Composite.*event-kbd'
aplay -l | grep -A1 'USB Composite Sample'
arecord -l | grep -A1 'USB Composite Sample'
amidi -l | grep 'USB Composite Sample'
```

Expected behavior:

- CDC responds with `COMP CDC RX`
- HID produces `KEY_A` press/release events when `USB_COMP_TEST_HID=1`
- MIDI `90 3C 40` echoes as `90 3D 40`
- USB audio capture records 48 kHz stereo data
- USB audio playback opens without ALSA errors

To keep interfaces present but stop generated test traffic, remove the
`USB_COMP_TEST_*` defines or set them to `0`. Undefined test flags default to
off.
