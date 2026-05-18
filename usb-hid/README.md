# usb-hid

## Purpose

This folder contains a USB HID sample for Daisy Seed-based development.
HID is the USB class used for devices such as keyboards, mice, and other report-based interfaces.

## Main files

- `UsbHid.cpp` — main application/sample entry point
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

## Example wrapper skeleton

A practical starting point for the other project could look like this.

### `usb_mode.h`

```c
#pragma once

#include <stddef.h>
#include <stdint.h>

void UsbMode_Init(void);
void UsbMode_Task(void);

int UsbMode_IsCdc(void);
int UsbMode_IsHid(void);

void UsbMode_SendDebugText(const char* text);
void UsbMode_SendHidReport(const uint8_t* data, size_t len);
```

### `usb_mode.cpp`

```cpp
#include "usb_mode.h"

#if defined(DEBUG) && DEBUG
#define USE_USB_CDC 1
#else
#define USE_USB_HID 1
#endif

#if USE_USB_CDC
extern "C" {
// include the project's CDC-facing headers here
// example:
// #include "usb_cdc_app.h"
}
#endif

#if USE_USB_HID
extern "C" {
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_hid.h"
extern USBD_HandleTypeDef hUsbDeviceHS;
}
#endif

void UsbMode_Init(void)
{
#if USE_USB_CDC
    // call the project's CDC init path here
    // example:
    // UsbCdc_Init();
#elif USE_USB_HID
    USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
    USBD_RegisterClass(&hUsbDeviceHS, &USBD_HID);
    USBD_Start(&hUsbDeviceHS);
#endif
}

void UsbMode_Task(void)
{
    // optional periodic USB servicing hook if the project needs one
}

int UsbMode_IsCdc(void)
{
#if USE_USB_CDC
    return 1;
#else
    return 0;
#endif
}

int UsbMode_IsHid(void)
{
#if USE_USB_HID
    return 1;
#else
    return 0;
#endif
}

void UsbMode_SendDebugText(const char* text)
{
#if USE_USB_CDC
    // route debug text through the CDC transmit path
    // example:
    // UsbCdc_Write(text);
#else
    (void)text;
#endif
}

void UsbMode_SendHidReport(const uint8_t* data, size_t len)
{
#if USE_USB_HID
    USBD_HID_SendReport(&hUsbDeviceHS, (uint8_t*)data, (uint16_t)len);
#else
    (void)data;
    (void)len;
#endif
}
```

## Integration notes for the wrapper

- Only one of `USE_USB_CDC` or `USE_USB_HID` should be active in a given build.
- Keep the USB mode selection in this wrapper rather than repeating conditionals throughout the application.
- Let the rest of the project call `UsbMode_*()` functions instead of USB-class-specific functions directly.
- If the HID build needs helper functions like `Press()` / `Release()`, either:
  - keep them inside a HID-specific source file and call them from the wrapper, or
  - expose a small HID-only helper API that stays behind the same compile-time switch.
- If the CDC build needs serial logging very early at boot, keep that boot-time init path aligned with the same macro policy.

## Minimal HID import set

If the target project is switching into HID mode, the current minimum local file set from this sample is:

- `UsbHid.cpp`
- `usbd_conf.c`
- `usbd_conf.h`
- `usbd_desc.c`
- `usbd_desc.h`
- `usbd_hid_kbd.c`

Required build addition:

- add HID include path:
  - `$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/HID/Inc`

Expected libDaisy dependency:

- use the existing ST HID support already present in libDaisy
- no libDaisy source patch should be required for this path

## Important limitation

This sample assumes a HID-only device personality.
If the target project currently uses libDaisy CDC, do not expect this HID code to sit on top of CDC unchanged.
The project must choose one device class path per build unless you intentionally design a composite device.

## Sample behavior

The `UsbHid.cpp` sample is intentionally quiet after enumeration.
It only blinks the onboard LED to show liveness and does not automatically type or emit test keystrokes.
That keeps the sample cleaner as an import/reference base for another project.

If you want active HID behavior, add it explicitly in the target project or behind a dedicated test path.

## Current sample settings

- Full-speed HID polling interval is set locally to **1 ms**
- The effective setting is enforced in `usb-hid/usbd_hid_kbd.c` so the endpoint descriptor and runtime interval both use `0x01`
- This keeps the change local to the sample without modifying libDaisy source.

## Notes

This folder is useful for validating report-based USB device behavior rather than serial/audio/storage behavior.
