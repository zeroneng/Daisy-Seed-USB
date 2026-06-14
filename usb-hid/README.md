# usb-hid

## Purpose

This folder contains a USB HID sample for Daisy Seed-based development.
HID is the USB class used for devices such as keyboards, mice, and other report-based interfaces.

## Main files

- `UsbHid.cpp` — thin sample application shell
- `HID.cpp` / `HID.h` — reusable HID init and key-report helpers
- `usbd_hid_kbd.c` — HID keyboard/report handling code
- `usbd_conf.c` / `usbd_conf.h` — USB device configuration glue
- `usbd_desc.c` / `usbd_desc.h` — HID-oriented device descriptors
- `Makefile` — build rules

## How it works

At a high level this sample:

1. initializes the target and USB device path
2. configures HID device descriptors and class handling
3. emits HID-style reports to the host
4. allows the host to recognize the target as a HID-class device

## Integration goal

The intended reusable pattern is:

- use **CDC in debug builds** for serial-style logging / development convenience
- use **HID in non-debug builds** for the actual HID/macropad behavior
- do **not** try to run CDC and HID at the same time in the same simple build path
- instead, compile one USB personality or the other based on a project-wide macro

A practical default policy is:

- `DEBUG=1` or `USB_MODE_CDC=1` → build the CDC path
- otherwise → build the HID path

## Recommended macro pattern

In the target project, define a single USB-mode switch in one place.
For example:

```c
#if defined(DEBUG) && DEBUG
#define USE_USB_CDC 1
#else
#define USE_USB_HID 1
#endif
```

Or, if you want explicit control independent of `DEBUG`:

```c
#define USB_MODE_CDC 1
/* or */
#define USB_MODE_HID 1
```

Prefer one authoritative switch rather than scattered `#ifdef DEBUG` checks throughout the codebase.

## What should switch in the other project

If the other project needs to flip between CDC and HID, the switch needs to be applied consistently in these areas:

1. **application init**
   - CDC build should run the CDC USB init path
   - HID build should run the HID USB init path

2. **USB class registration**
   - CDC build registers the CDC class
   - HID build registers the HID class

3. **descriptor source files**
   - CDC build should compile CDC descriptor files
   - HID build should compile HID descriptor files

4. **USB glue files**
   - if the project uses class-specific USB glue, compile only the set that matches the selected mode

5. **application-facing send/output functions**
   - CDC build routes output through serial-style transmit functions
   - HID build routes output through HID report send functions

6. **build system / Makefile**
   - compile only the source files for the active USB mode
   - add only the include paths needed for the active USB mode

## Recommended project structure in the other project

A clean approach is to isolate mode-specific USB code behind a thin wrapper.
For example:

- `usb_mode.h`
- `usb_mode.cpp`
- `usb_cdc_*` files
- `usb_hid_*` files

Then expose a small common API such as:

```c
void UsbMode_Init(void);
void UsbMode_Task(void);
void UsbMode_SendDebugText(const char* text);
void UsbMode_SendHidReport(const uint8_t* data, size_t len);
```

The implementation can internally select CDC or HID with compile-time conditionals.
That keeps the rest of the project from knowing about descriptor files, class registration, or USB stack details.

## Example integration shape

A practical starting point for the other project is to keep the reusable HID logic in its own helper module and keep the application shell thin.

### Reusable HID helper API

The current reusable helper module in this sample is:

- `HID.h`
- `HID.cpp`

Current helper API:

```c
void UsbHid_Init(void);
void UsbHid_SendReport(void);
void UsbHid_ClearAllKeys(void);
bool UsbHid_KeyOn(uint8_t keycode);
bool UsbHid_KeyOff(uint8_t keycode);
```

Recommended usage model:

- import the HID helper module into the target project
- keep USB descriptor/config/class files alongside it
- call `UsbHid_Init()` during startup
- call `UsbHid_KeyOn()` / `UsbHid_KeyOff()` from application logic
- use `UsbHid_ClearAllKeys()` as a safety reset when needed

### Minimal application shell example

A target project shell can stay very small:

```cpp
#include "daisy_seed.h"
#include "HID.h"

using namespace daisy;

namespace {
DaisySeed hw;
}

int main(void)
{
    hw.Init();
    UsbHid_Init();

    // Example usage:
    // System::Delay(1000);
    // UsbHid_KeyOn(0x04);      // press 'a'
    // System::Delay(50);
    // UsbHid_KeyOff(0x04);     // release 'a'
    // System::Delay(50);
    // UsbHid_ClearAllKeys();

    for(;;)
    {
        System::Delay(1000);
    }
}
```

## Integration notes for the current helper-module approach

- Keep the HID logic in `HID.*` rather than embedding it into the main application file.
- Keep `UsbHid.cpp`-style files as thin shells only.
- The helper module owns:
  - USB HID init
  - HID report state buffer
  - raw HID report send path
  - key on/off helpers
  - clear-all helper
- The application shell should own:
  - board init
  - app logic
  - when to call key on/off functions
- `UsbHid_SendReport()` exists for low-level control, but most integrations should prefer `UsbHid_KeyOn()`, `UsbHid_KeyOff()`, and `UsbHid_ClearAllKeys()`.
- If the target project still needs CDC in debug builds, keep the USB-mode selection above this layer and compile either the CDC stack or the HID helper path per build.

## Minimal HID import set

If the target project is switching into HID mode, the current minimum local file set from this sample is:

- `HID.cpp`
- `HID.h`
- `usbd_conf.c`
- `usbd_conf.h`
- `usbd_desc.c`
- `usbd_desc.h`
- `usbd_hid_kbd.c`

`UsbHid.cpp` is now just the example shell and does not need to be imported as part of the reusable HID layer.

Required build addition:

- add HID include path:
  - `$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/HID/Inc`

Expected libDaisy dependency:

- use the existing ST HID support already present in libDaisy
- use the FS USB IRQ handlers provided by libDaisy; this sample no longer
  compiles its old local `usb_irq_override.c` file by default

## Important limitation

This sample assumes a HID-only device personality.
If the target project currently uses libDaisy CDC, do not expect this HID code to sit on top of CDC unchanged.
The project must choose one device class path per build unless you intentionally design a composite device.

## Sample behavior

The `UsbHid.cpp` sample sends a repeating `A` key press/release so the host can
validate real HID input events, not just enumeration.

If you import the reusable HID layer into another project, keep generated key
events behind an explicit test path.

## Current sample settings

- Full-speed HID polling interval is set locally to **1 ms**
- The effective setting is enforced in `usb-hid/usbd_hid_kbd.c` so the endpoint descriptor and runtime interval both use `0x01`
- This keeps the change local to the sample without modifying libDaisy source.

## Notes

This folder is useful for validating report-based USB device behavior rather than serial/audio/storage behavior.

The old `usb_irq_override.c` file is left in the folder as reference, but the
Makefile does not compile it. Building both that file and libDaisy's FS USB IRQ
handlers causes duplicate `OTG_FS_*IRQHandler` definitions.

Small maintenance note: after changes, prefer verifying build, flash, and host HID enumeration as a quick sanity pass.
