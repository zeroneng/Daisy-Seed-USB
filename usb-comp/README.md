# usb-comp

`usb-comp` is a Daisy Seed USB composite-device sample built on the ST USB
Device composite builder and libDaisy-facing hardware APIs.

It is meant to be a practical starting point for projects that need several USB
functions on the Daisy external USB port without moving the whole application to
TinyUSB.

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

2. Copy the required USB source files.

   At minimum, bring these into your project:

   - `usbd_conf.c`
   - `usbd_desc.c` / `usbd_desc.h`
   - `usbd_composite_builder.c`
   - `usb_comp_cdc_if.c` / `usb_comp_cdc_if.h`
   - enabled class files such as `usbd_audio*`, `usbd_midi*`, and `usbd_msc_storage*`
   - `UsbCompMscSdBackend.cpp` only if you need SD-backed MSC

   Also include the local HID keyboard class from `../usb-hid/usbd_hid_kbd.c`
   if you need NKRO keyboard support.

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

7. Decide on a mode model.

   For RHYTHM, the recommended model is:

   - Performance mode: CDC + HID + audio + MIDI, MSC disabled.
   - Storage/update mode: stop or avoid audio DMA, enable MSC, expose SD.
   - Leave storage/update mode only after the host has unmounted/ejected.

8. Validate one layer at a time.

   Start with descriptors and enumeration, then each active interface. Do MSC
   reads/writes by themselves. Do not run storage I/O while scripts are opening
   CDC or otherwise causing USB reset/re-enumeration.

## Known-Good Status

Validated on the Pi/Daisy setup:

- CDC + HID + audio + MIDI with MSC disabled
- MIDI + MSC with audio DMA not running
- SD-backed MSC runtime enable after boot
- MSC-only SD reads with 4-bit libDaisy SD configuration

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
