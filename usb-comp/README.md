# usb-comp

Composite USB support for Daisy Seed projects.

Default profile: CDC ACM serial + NKRO HID keyboard + USB audio + USB MIDI.
MSC is not part of this stack.

## Files

- `usb-comp.h` - header-only app bridge
- `UsbComp.cpp` - standalone composite test app
- `Makefile` - standalone build and feature flags
- `usbd_conf.c` / `usbd_conf.h` - USB PCD glue and FIFO allocation
- `usbd_desc.c` / `usbd_desc.h` - USB descriptors and strings
- `usbd_composite_builder.c` - local ST composite builder
- `usb_comp_cdc_if.c` / `usb_comp_cdc_if.h` - CDC glue
- `usbd_hid_kbd.c` - NKRO HID keyboard class
- `usbd_audio.c` / `usbd_audio_if.c` - USB audio class and bridge
- `usbd_midi.c` / `usbd_midi.h` - USB MIDI class
- `vendor/**` - local ST USB Device and HID headers

## App API

```cpp
#include "usb-comp.h"

UsbComp::Init();
UsbComp::Process();

UsbComp::SendCdc("text\r\n");

UsbComp::SetHidKeyState(keycode, pressed);
UsbComp::SendHidReport();
UsbComp::KeyOn(keycode);
UsbComp::KeyOff(keycode);
UsbComp::ClearAllKeys();

UsbComp::PushCapture(left, right);
UsbComp::PopPlayback(left, right);
UsbComp::CommitCaptureBlock();
```

## Drop-In Makefile

```make
USB_COMP_DIR = ../usb-comp
ST_USB_DEVICE_DIR = $(USB_COMP_DIR)/vendor/stm32_mw_usb_device

C_SOURCES += \
$(USB_COMP_DIR)/usbd_conf.c \
$(USB_COMP_DIR)/usbd_desc.c \
$(USB_COMP_DIR)/usb_comp_cdc_if.c \
$(USB_COMP_DIR)/usbd_audio.c \
$(USB_COMP_DIR)/usbd_audio_if.c \
$(USB_COMP_DIR)/usbd_midi.c \
$(USB_COMP_DIR)/usbd_hid_kbd.c \
$(USB_COMP_DIR)/usbd_composite_builder.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_core.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_ctlreq.c \
$(ST_USB_DEVICE_DIR)/Core/Src/usbd_ioreq.c \
$(ST_USB_DEVICE_DIR)/Class/CDC/Src/usbd_cdc.c

C_INCLUDES += \
-I$(USB_COMP_DIR) \
-I$(USB_COMP_DIR)/vendor/hid/Inc \
-I$(ST_USB_DEVICE_DIR)/Core/Inc \
-I$(ST_USB_DEVICE_DIR)/Class/CDC/Inc \
-I$(ST_USB_DEVICE_DIR)/Class/HID/Inc \
-I$(ST_USB_DEVICE_DIR)/Class/CompositeBuilder/Inc \
-I$(LIBDAISY_DIR)/src/usbd

C_DEFS += \
-DUSE_USBD_COMPOSITE \
-DUSBD_CMPSIT_ACTIVATE_CDC=1 \
-DUSBD_CMPSIT_ACTIVATE_HID=1 \
-DUSBD_CMPSIT_ACTIVATE_AUDIO=1 \
-DUSBD_CMPSIT_ACTIVATE_MIDI=1 \
-DUSB_COMP_TEST_CDC=0 \
-DUSB_COMP_TEST_HID=0 \
-DUSB_COMP_TEST_AUDIO=0 \
-DUSB_COMP_TEST_MIDI=0 \
-DUSB_COMP_AUDIO_CAPTURE_RING_SIZE=256 \
-DUSB_COMP_AUDIO_PLAYBACK_RING_SIZE=512 \
-DHID_FS_BINTERVAL=0x01U
```

Keep `-I$(USB_COMP_DIR)` before libDaisy USB includes so the local
`usbd_conf.h` is used.

## App Startup

```cpp
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
UsbComp::PopPlayback(play_l, play_r);
UsbComp::CommitCaptureBlock();
```

Do not call `hw.StartLog(false)` for the same USB device after `UsbComp::Init()`.

## USB Handle Ownership

The composite stack must own the ST USB device handles. `usbd_conf.c` defines:

```c
USBD_HandleTypeDef hUsbDeviceFS;
USBD_HandleTypeDef hUsbDeviceHS;
```

Do not leave these as unresolved `extern` symbols in an app integration. If
`hUsbDeviceHS` is declared but not defined by this stack, the linker can satisfy
the symbol from `libDaisy/build/libdaisy.a(usb.o)`. That silently pulls in part
of libDaisy's older USB device layer, so a project may appear to avoid USB
logging or legacy USB calls while still linking against the old USB owner.

If `UsbComp::Init()` locks up, or composite USB behaves differently inside an
app than it does in this standalone example, check the map file first:

```bash
grep -E 'hUsbDevice(HS|FS)|hpcd_USB_OTG_HS' build/*.map
```

Expected result: `hUsbDeviceHS`, `hUsbDeviceFS`, and the PCD handle should come
from the local `usbd_conf.o` built from this `usb-comp` directory.

## Defaults

- USB audio: 48 kHz, stereo, 16-bit
- USB audio packet: 48 stereo frames / 192 bytes every 1 ms
- Capture ring: 256 stereo float frames / 2048 bytes
- Playback ring: 512 usable stereo int16 frames / 2048 bytes
- Capture ring storage: SRAM by default
- USB capture uses a persistent read cursor. It does not re-anchor each packet
  to the app write cursor, because that can skip or repeat samples as the USB
  SOF and audio callback clocks drift.
- Set `USB_COMP_AUDIO_CAPTURE_RING_SIZE` larger than the app audio callback
  block size when needed. `256` is the default; `512` is a conservative option
  for heavier apps.
- `USB_COMP_AUDIO_PLAYBACK_RING_SIZE` is separate from the capture callback
  size. It buffers USB host playback jitter; 512 is the smallest value that
  passed the full composite hardware test on this target.
- Capture ring sizes must be powers of two and at least 128 frames
- HID polling interval: 1 ms

## Build And Test

```bash
cd /mnt/clu-nas/developer/daisy-usb/usb-comp
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make clean
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make -j2
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make program
```

Host validation:

```bash
TEST_CDC=1 TEST_HID=1 TEST_AUDIO=1 TEST_MIDI=1 bash ./test_usb_comp.sh
```

Manual checks:

```bash
lsusb | grep '0483:5764'
ls -l /dev/serial/by-id | grep 'USB_Composite'
ls -l /dev/input/by-id | grep 'USB_Composite.*event-kbd'
aplay -l | grep -A1 'USB Composite Sample'
arecord -l | grep -A1 'USB Composite Sample'
amidi -l | grep 'USB Composite Sample'
```
