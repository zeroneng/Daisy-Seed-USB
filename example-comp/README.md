# example-comp

Minimal Daisy Seed app using `../usb-comp`.

It blinks the LED, runs a 48 kHz audio callback, starts composite USB, sends a
100 Hz test tone to analog and USB audio capture, mixes USB playback into analog
output, captures incoming MIDI to CDC serial text, sends a one-second MIDI test
message, and sends a one-second NKRO HID `A` key tap by directly setting key
state and sending HID reports.

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

## CDC, HID, And MIDI Examples

Send formatted lines to the host CDC ACM serial port:

```cpp
UsbComp::CDCSend("example-comp online");
UsbComp::CDCSend("tempo %lu", tempo);
```

Send a single HID keyboard key press and release with NKRO report functions:

```cpp
const uint8_t key = UsbComp::CharToKeycode('a');
UsbComp::ClearAllKeys();
UsbComp::SetHidKeyState(key, true);
UsbComp::SendHidReport();
System::Delay(40);
UsbComp::SetHidKeyState(key, false);
UsbComp::SendHidReport();
```

Build a multi-key HID report, then send it once:

```cpp
UsbComp::SetHidKeyState(UsbComp::CharToKeycode('a'), true);
UsbComp::SetHidKeyState(UsbComp::CharToKeycode('s'), true);
UsbComp::SendHidReport();

UsbComp::ClearAllKeys();
```

Send USB MIDI note packets directly. The first byte is the USB MIDI CIN:

```cpp
UsbComp::SendMidiPacket(0x09, 0x90, 60, 100); // note on, middle C
UsbComp::SendMidiPacket(0x08, 0x80, 60, 0);   // note off, middle C
```

Capture incoming USB MIDI packets with a callback:

```cpp
void OnUsbMidi(uint8_t cin, uint8_t status, uint8_t data1, uint8_t data2)
{
    // Copy into an app queue; do heavier work from the main loop.
}

UsbComp::SetMidiReceiveCallback(OnUsbMidi);
```

In this app the callback copies each USB MIDI packet into a small foreground
queue. The main loop prints each packet to the CDC serial console:

```text
MIDI RX 09 90 3C 40
```

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
USB_COMP_MANUFACTURER ?= Generic
USB_COMP_PRODUCT ?= USB Composite
USB_COMP_CONFIGURATION ?= Composite Config
USB_COMP_INTERFACE ?= Composite Interface
USB_COMP_CDC ?= Composite CDC
USB_COMP_HID ?= Composite HID Keyboard
USB_COMP_AUDIO ?= Composite Audio
USB_COMP_MIDI ?= Composite MIDI

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
'-DUSB_COMP_MANUFACTURER_STRING="$(USB_COMP_MANUFACTURER)"' \
'-DUSB_COMP_PRODUCT_STRING="$(USB_COMP_PRODUCT)"' \
'-DUSB_COMP_CONFIGURATION_STRING="$(USB_COMP_CONFIGURATION)"' \
'-DUSB_COMP_INTERFACE_STRING="$(USB_COMP_INTERFACE)"' \
'-DUSB_COMP_CDC_STRING="$(USB_COMP_CDC)"' \
'-DUSB_COMP_HID_STRING="$(USB_COMP_HID)"' \
'-DUSB_COMP_AUDIO_STRING="$(USB_COMP_AUDIO)"' \
'-DUSB_COMP_MIDI_STRING="$(USB_COMP_MIDI)"' \
-DUSB_COMP_TEST_CDC=1 \
-DUSB_COMP_TEST_HID=1 \
-DUSB_COMP_TEST_AUDIO=1 \
-DUSB_COMP_TEST_MIDI=1 \
-DHID_FS_BINTERVAL=0x01U
```

Override the USB text from the app Makefile:

```make
USB_COMP_MANUFACTURER = ZERONE
USB_COMP_PRODUCT = RHYTHM
USB_COMP_CONFIGURATION = RHYTHM Composite
USB_COMP_INTERFACE = RHYTHM USB
USB_COMP_CDC = RHYTHM CDC
USB_COMP_HID = RHYTHM Keyboard
USB_COMP_AUDIO = RHYTHM Audio
USB_COMP_MIDI = MIDI
```

Some MIDI monitors display `USB_COMP_PRODUCT` plus `USB_COMP_MIDI`, so
`RHYTHM` + `MIDI` appears as `RHYTHM MIDI`.

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
aplay -l | grep -A1 'USB Composite'
arecord -l | grep -A1 'USB Composite'
amidi -l | grep 'USB Composite'
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
