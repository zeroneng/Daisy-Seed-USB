# usb-comp

`usb-comp` is a Daisy Seed USB composite-device sample built on the ST USB
Device composite builder and libDaisy-facing hardware APIs.

It is meant to be a practical starting point for projects that need several USB
functions on the Daisy external USB port without moving the whole application to
TinyUSB.

## What This Project Proves

This project is the current reference for the following stack on the Daisy Seed
external USB connector:

- Daisy Seed / STM32H750 external USB connector
- ST USB Device composite stack
- local composite descriptor builder
- CDC ACM serial
- NKRO HID keyboard
- USB audio capture/playback
- USB MIDI
- SD-backed MSC when exposed as a separate storage-mode function

It also proves that the external USB path can be owned explicitly by the
application instead of relying on libDaisy's `hw.StartLog(false)` CDC/logging
startup path.

The important migration lesson is that USB ownership, descriptors, endpoint
numbers, FIFO allocation, and class registration order all need to move
together. Port the known-good shape first, then replace the test behavior with
product behavior.

## Current Capabilities

The sample can compile these USB functions into one composite device:

- CDC ACM serial
- NKRO HID keyboard
- USB audio capture/playback
- USB MIDI
- USB Mass Storage Class (MSC), backed by RAM or Daisy SD

The default build keeps MSC disabled and runs the four active performance
interfaces:

- CDC ACM
- NKRO HID keyboard
- USB audio
- USB MIDI

MSC can be compiled in when needed. With SD-backed MSC, storage starts disabled
by default and firmware can enable or disable it at runtime:

```c
USB_COMP_MSC_Enable();
USB_COMP_MSC_Disable();
USB_COMP_MSC_IsEnabled();
```

Important current limitation: active Daisy audio DMA and live SD-backed MSC are
not yet proven stable at the same time. MIDI + MSC works when audio DMA is not
running. The recommended product shape is performance mode without MSC, and a
separate storage mode that enables MSC only when it is safe to expose the SD
card to the host.

## Main Files

- `UsbComp.cpp` - application entry point and class registration order
- `Makefile` - feature flags, test flags, and build defaults
- `usbd_conf.c` - USB PCD glue and full-speed FIFO allocation
- `usbd_desc.c` / `usbd_desc.h` - device descriptor strings
- `usbd_composite_builder.c` - local ST composite descriptor builder
- `usb_comp_cdc_if.c` / `usb_comp_cdc_if.h` - CDC interface glue
- `usbd_audio.c` / `usbd_audio_if.c` - USB audio class and Daisy audio bridge
- `usbd_midi.c` / `usbd_midi.h` - USB MIDI class implementation
- `usbd_msc_storage.c` / `usbd_msc_storage.h` - MSC storage callbacks and runtime gate
- `UsbCompMscSdBackend.cpp` - libDaisy-facing SD backend for MSC
- `test_usb_comp.sh` - host validation for enabled interfaces

## Build Flags

Feature flags control which USB interfaces appear in the descriptor:

```bash
USB_COMP_ENABLE_CDC=1
USB_COMP_ENABLE_HID=1
USB_COMP_ENABLE_AUDIO=1
USB_COMP_ENABLE_MIDI=1
USB_COMP_ENABLE_MSC=0
```

Test flags control firmware-generated traffic only. They do not change the USB
descriptor:

```bash
USB_COMP_TEST_CDC=1
USB_COMP_TEST_HID=1
USB_COMP_TEST_AUDIO=1
USB_COMP_TEST_MIDI=1
USB_COMP_TEST_MSC=1
```

MSC-specific flags:

```bash
USB_COMP_MSC_USE_SD=0
USB_COMP_MSC_START_ENABLED=0
USB_COMP_MSC_TEST_ENABLE_DELAY_MS=0
```

Audio/MSC interaction flag:

```bash
USB_COMP_AUDIO_START_ON_BOOT=1
```

When `USB_COMP_ENABLE_MSC=1`, the Makefile defaults audio test traffic off and
does not start Daisy audio on boot. This protects storage testing from the known
audio-DMA-versus-SD-MSC conflict.

## Common Builds

Default performance profile:

```bash
make clean
make
make program
```

Equivalent explicit build:

```bash
make clean
make \
  USB_COMP_ENABLE_CDC=1 \
  USB_COMP_ENABLE_HID=1 \
  USB_COMP_ENABLE_AUDIO=1 \
  USB_COMP_ENABLE_MIDI=1 \
  USB_COMP_ENABLE_MSC=0
```

Quiet product-style performance build with no generated test traffic:

```bash
make clean
make \
  USB_COMP_ENABLE_CDC=1 \
  USB_COMP_ENABLE_HID=1 \
  USB_COMP_ENABLE_AUDIO=1 \
  USB_COMP_ENABLE_MIDI=1 \
  USB_COMP_ENABLE_MSC=0 \
  USB_COMP_TEST_CDC=0 \
  USB_COMP_TEST_HID=0 \
  USB_COMP_TEST_AUDIO=0 \
  USB_COMP_TEST_MIDI=0
```

SD-backed MSC storage-mode diagnostic build:

```bash
make clean
make \
  USB_COMP_ENABLE_CDC=1 \
  USB_COMP_ENABLE_HID=1 \
  USB_COMP_ENABLE_AUDIO=1 \
  USB_COMP_ENABLE_MIDI=1 \
  USB_COMP_ENABLE_MSC=1 \
  USB_COMP_MSC_USE_SD=1 \
  USB_COMP_MSC_START_ENABLED=0 \
  USB_COMP_AUDIO_START_ON_BOOT=0
```

MSC-only SD isolation build:

```bash
make clean
make \
  USB_COMP_ENABLE_CDC=0 \
  USB_COMP_ENABLE_HID=0 \
  USB_COMP_ENABLE_AUDIO=0 \
  USB_COMP_ENABLE_MIDI=0 \
  USB_COMP_ENABLE_MSC=1 \
  USB_COMP_MSC_USE_SD=1 \
  USB_COMP_MSC_START_ENABLED=1
```

## Host Validation

Default four-function validation:

```bash
bash ./test_usb_comp.sh
```

Validate the current performance profile with MSC explicitly skipped:

```bash
TEST_CDC=1 TEST_HID=1 TEST_AUDIO=1 TEST_MIDI=1 TEST_MSC=0 \
bash ./test_usb_comp.sh
```

Validate only MIDI and MSC in a storage diagnostic build:

```bash
TEST_CDC=0 TEST_HID=0 TEST_AUDIO=0 TEST_MIDI=1 TEST_MSC=1 \
bash ./test_usb_comp.sh
```

Do not run MSC block reads or writes in parallel with CDC/HID/audio/MIDI tests.
Opening CDC or resetting the composite device while Linux has an active MSC
READ(10) can wedge the host USB path until reboot. Test storage first, then test
the other interfaces, or keep MSC disabled while validating performance mode.

## Minimal Migration Goal

For a first merge into another Daisy application, keep the target simple:

- build cleanly with CDC + HID + audio + MIDI enabled and MSC disabled
- enumerate on the external USB port as one composite device
- create a CDC ACM device on the host
- send/receive one simple CDC command or status message
- produce one known HID key press/release event
- enumerate MIDI and echo or receive one short MIDI message
- enumerate audio capture/playback
- leave product audio, storage mode, and full app routing for later layers

Once that baseline works inside the target project, remove the generated test
traffic and connect each USB class to the real application code.

## Runtime MSC Control

When MSC is compiled in, the USB MSC descriptor can enumerate while the storage
medium reports unavailable. This lets application firmware decide when to expose
the SD card.

Call this when entering a storage/update mode:

```c
if(USB_COMP_MSC_Enable() == 0)
{
    // Host can now see the medium after it rescans the MSC device.
}
```

Call this when leaving storage/update mode:

```c
USB_COMP_MSC_Disable();
```

Do not disable MSC while the host has the disk mounted or is reading/writing.
Treat disable like removing media from a card reader.

`USB_COMP_MSC_START_ENABLED=1` exposes the medium at boot. Leave it at `0` for a
product workflow where RHYTHM controls when the SD card is visible.

`USB_COMP_MSC_TEST_ENABLE_DELAY_MS` is a diagnostic hook. It calls
`USB_COMP_MSC_Enable()` from the main loop after a delay so runtime enable can be
validated without relying on CDC commands.

## Using This In Your Own Project

The cleanest integration path is to copy the USB composition pattern into your
application and then replace the test behaviors with your product behaviors.

1. Start from your existing Daisy application.

   Keep your normal `DaisySeed hw;`, audio setup, main loop, and application
   state. `usb-comp` is not meant to become your whole app; it is a reference for
   how to register and service the composite USB classes.

   Avoid calling `hw.StartLog(false)` for the same external USB device after the
   composite stack owns USB. CDC should be one class inside this composite
   device, not a second USB startup path.

2. Copy the `usb-comp/` folder into your project.

   The folder is intended to be self-contained for the composite USB stack. It
   includes the descriptors, composite builder, CDC/audio/MIDI/MSC class glue,
   NKRO HID class file, and the ST USB Device middleware/vendor headers used by
   the sample.

   At minimum, keep these files in your copied `usb-comp/` folder:

   - `usbd_conf.c`
   - `usbd_desc.c` / `usbd_desc.h`
   - `usbd_composite_builder.c`
   - `usb_comp_cdc_if.c` / `usb_comp_cdc_if.h`
   - `usb-comp.h` if you want the header-only integration bridge
   - `usbd_hid_kbd.c`
   - enabled class files such as `usbd_audio*`, `usbd_midi*`, and `usbd_msc_storage*`
   - `vendor/stm32_mw_usb_device/**`
   - `vendor/hid/Inc/usbd_hid.h`
   - `UsbCompMscSdBackend.cpp` only if you need SD-backed MSC

3. Mirror the Makefile includes and definitions.

   Your project must include the ST USB Device core, class folders, composite
   builder headers, libDaisy headers, and the same `C_DEFS` feature flags used by
   `usb-comp`.

   The important descriptor flags are:

   ```make
   -DUSE_USBD_COMPOSITE
   -DUSBD_CMPSIT_ACTIVATE_CDC=$(USB_COMP_ENABLE_CDC)
   -DUSBD_CMPSIT_ACTIVATE_HID=$(USB_COMP_ENABLE_HID)
   -DUSBD_CMPSIT_ACTIVATE_AUDIO=$(USB_COMP_ENABLE_AUDIO)
   -DUSBD_CMPSIT_ACTIVATE_MIDI=$(USB_COMP_ENABLE_MIDI)
   -DUSBD_CMPSIT_ACTIVATE_MSC=$(USB_COMP_ENABLE_MSC)
   ```

   Example performance-mode source shape:

   ```make
   C_SOURCES += \
     usb/usbd_conf.c \
     usb/usbd_desc.c \
     usb/usbd_composite_builder.c \
     usb/usb_comp_cdc_if.c \
     usb/usbd_audio.c \
     usb/usbd_audio_if.c \
     usb/usbd_midi.c \
     usb/usbd_hid_kbd.c \
     $(ST_USB_DEVICE_DIR)/Core/Src/usbd_core.c \
     $(ST_USB_DEVICE_DIR)/Core/Src/usbd_ctlreq.c \
     $(ST_USB_DEVICE_DIR)/Core/Src/usbd_ioreq.c \
     $(ST_USB_DEVICE_DIR)/Class/CDC/Src/usbd_cdc.c
   ```

   Add `usbd_msc_storage.c` and `UsbCompMscSdBackend.cpp` only when you are ready
   to compile MSC support into the target app.

   Keep your project include path before libDaisy's USB include path:

   ```make
   C_INCLUDES += \
     -Iusb \
     -Iusb/vendor/hid/Inc \
     -I$(ST_USB_DEVICE_DIR)/Core/Inc \
     -I$(ST_USB_DEVICE_DIR)/Class/CDC/Inc \
     -I$(ST_USB_DEVICE_DIR)/Class/HID/Inc \
     -I$(ST_USB_DEVICE_DIR)/Class/MSC/Inc \
     -I$(ST_USB_DEVICE_DIR)/Class/CompositeBuilder/Inc \
     -I$(LIBDAISY_DIR)/src/usbd
   ```

   The `-Iusb` ordering matters because the USB core must see your local
   composite `usbd_conf.h`, not libDaisy's default one-interface copy.

4. Register classes in a known-good order.

   `InitUSBComposite()` shows the registration pattern. Keep the endpoint arrays
   and class IDs together:

   - HID uses IN EP4
   - CDC data uses EP2, CDC notification uses EP6
   - audio uses its class endpoint macros
   - MIDI uses EP3 IN/OUT
   - MSC uses EP5 IN and EP4 OUT in composite mode

   Do not casually move endpoint numbers. EP6 IN is acceptable for CDC
   notification, but it did not behave as a reliable real data path in this
   full-speed ST composite setup.

   Minimal startup shape:

   ```cpp
   hw.Init();
   InitUSBComposite();

   while(1)
   {
       RunCdcService();
       RunHidService();
       RunMidiService();
       RunStorageModeService();
       System::Delay(1);
   }
   ```

   The exact function names in your app can change. The ownership model should
   not: initialize the Daisy hardware, initialize one composite USB device, then
   service the enabled USB classes from the main loop or their callbacks.

5. Keep the FIFO allocation matched to the endpoint layout.

   `usbd_conf.c` is just as important as the descriptors. Full-speed USB has a
   320-word FIFO budget. If you enable MSC, the code gives MSC IN a large FIFO
   and shrinks other IN FIFOs enough to fit. If you disable MSC, audio gets more
   IN FIFO space.

6. Replace tests with real application behavior.

   - Replace `RunCdcTest()` with your command/status protocol.
   - Replace `RunHidTest()` with your actual NKRO key state.
   - Replace MIDI echo behavior in `MIDI_Receive_HS()` with your MIDI parser.
   - Replace the generated audio test tone with RHYTHM audio data.
   - Keep MSC behind explicit storage-mode entry/exit calls.

   Example CDC command hook:

   ```cpp
   if(command == "storage on")
   {
       StopAudioForStorageMode();
       USB_COMP_MSC_Enable();
   }
   else if(command == "storage off")
   {
       USB_COMP_MSC_Disable();
       StartAudioForPerformanceMode();
   }
   ```

   Only use a command like this after the product has a clear UI/host workflow
   for ejecting the disk. Do not disable MSC while the host has it mounted.

7. Decide on a mode model.

   For RHYTHM, the recommended model is:

   - Performance mode: CDC + HID + audio + MIDI, MSC disabled.
   - Storage/update mode: stop or avoid audio DMA, enable MSC, expose SD.
   - Leave storage/update mode only after the host has unmounted/ejected.

8. Validate one layer at a time.

   Start with descriptors and enumeration, then each active interface. Do MSC
   reads/writes by themselves. Do not run storage I/O while scripts are opening
   CDC or otherwise causing USB reset/re-enumeration.

## RHYTHM Example Profiles

Performance mode, no generated test traffic:

```bash
make clean
make \
  USB_COMP_ENABLE_CDC=1 \
  USB_COMP_ENABLE_HID=1 \
  USB_COMP_ENABLE_AUDIO=1 \
  USB_COMP_ENABLE_MIDI=1 \
  USB_COMP_ENABLE_MSC=0 \
  USB_COMP_TEST_CDC=0 \
  USB_COMP_TEST_HID=0 \
  USB_COMP_TEST_AUDIO=0 \
  USB_COMP_TEST_MIDI=0
```

Storage-capable build, with MSC compiled in but not exposed at boot:

```bash
make clean
make \
  USB_COMP_ENABLE_CDC=1 \
  USB_COMP_ENABLE_HID=1 \
  USB_COMP_ENABLE_AUDIO=1 \
  USB_COMP_ENABLE_MIDI=1 \
  USB_COMP_ENABLE_MSC=1 \
  USB_COMP_MSC_USE_SD=1 \
  USB_COMP_MSC_START_ENABLED=0 \
  USB_COMP_AUDIO_START_ON_BOOT=0 \
  USB_COMP_TEST_CDC=0 \
  USB_COMP_TEST_HID=0 \
  USB_COMP_TEST_AUDIO=0 \
  USB_COMP_TEST_MIDI=0 \
  USB_COMP_TEST_MSC=0
```

First host validation after migrating:

```bash
TEST_CDC=1 TEST_HID=1 TEST_AUDIO=1 TEST_MIDI=1 TEST_MSC=0 \
bash ./test_usb_comp.sh
```

Storage validation should be separate:

```bash
TEST_CDC=0 TEST_HID=0 TEST_AUDIO=0 TEST_MIDI=0 TEST_MSC=1 \
bash ./test_usb_comp.sh
```

## Symptoms This Integration Avoids

Use this project as the reference when the target app shows any of these
symptoms:

- `hw.StartLog(false)` prevents the app from booting or enumerating correctly
- CDC works in `usb-cdc`, but not in the product app
- descriptors look right, but CDC/HID/MIDI traffic is silent
- Linux audio capture fails at `SET_INTERFACE`
- adding one class breaks another class that previously worked
- MSC reads fail while audio DMA is running
- the host USB path wedges after storage I/O and re-enumeration happen together

Most of those failures come from one of four causes: split USB ownership,
incorrect include order, endpoint/FIFO mismatch, or testing storage while
another interface is resetting the device.

## What Not To Do

- Do not mix this composite path with `hw.StartLog(false)` on the same USB
  device.
- Do not copy descriptors without copying the matching endpoint arrays, FIFO
  sizes, and class registration order.
- Do not move endpoint numbers casually to make a descriptor "look cleaner."
- Do not use EP6 IN for real data traffic in this full-speed ST composite setup;
  keep it for CDC notification.
- Do not let libDaisy's default `usbd_conf.h` shadow the local project
  `usbd_conf.h`; keep the project include path first.
- Do not enable generated test traffic in the product build.
- Do not run MSC reads/writes in parallel with CDC/HID/audio/MIDI tests.
- Do not expose MSC while RHYTHM is actively using the SD card for performance
  work.
- Do not bypass libDaisy-facing SD APIs in the `usb-comp` app/backend path.

## Validation Status

Historical validation on the Pi/Daisy setup has included:

- CDC + HID + audio + MIDI with MSC disabled
- MIDI + MSC with audio DMA not running
- SD-backed MSC runtime enable after boot
- MSC-only SD reads with 4-bit libDaisy SD configuration

After project cleanup on 2026-06-13, `usb-comp` and `basic-usb` still build
from the kept project-local vendor folders:

- `usb-comp/vendor`
- `usb-hid/vendor`

Not yet claimed:

- Active USB audio streaming and live SD-backed MSC at the same time

## Notes

The SD backend intentionally uses libDaisy-facing SD APIs only:

- `SdmmcHandler`
- `BSP_SD_Init`
- `BSP_SD_GetCardInfo`
- `BSP_SD_GetCardState`
- `BSP_SD_ReadBlocks`
- `BSP_SD_WriteBlocks`

Do not bypass libDaisy with direct STM32 SD peripheral access in the
`usb-comp` app/backend path unless that project rule is explicitly changed.
