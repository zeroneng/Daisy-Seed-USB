# usb-cdc - Porting Notes

This project is a standalone external USB CDC example for Daisy Seed /
STM32H750. Historical validation on the Pi/Daisy setup showed it can:

- build successfully on the Pi host
- flash successfully to the Daisy target
- enumerate as a CDC ACM serial device on the **external USB port**
- transmit test strings like `LED: ON` / `LED: OFF`

Re-run build, flash, and host enumeration checks after libDaisy or USB middleware
changes before treating it as a current known-good baseline.

This README explains how to port the working CDC path into another application.

---

## What This Project Proves

The `usb-cdc` example proves that the following stack can work on this hardware/path:

- external USB connector
- STM32H750 / Daisy Seed target
- USB CDC device class
- direct ST USB device stack usage
- explicit `USBD_Init()` / `USBD_RegisterClass()` / `USBD_CDC_RegisterInterface()` / `USBD_Start()` flow

It also proves that the external USB CDC path works independently of a libDaisy `hw.StartLog(false)` logging path.

---

## Key Difference From libDaisy StartLog

A common libDaisy app-level CDC/logging path tries to enable CDC with:

```cpp
hw.StartLog(false);
```

That uses libDaisy's default CDC/logging path.

The `usb-cdc` example does **not** use `hw.StartLog(false)`.
Instead, it manually brings up USB CDC with:

```cpp
USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
USBD_RegisterClass(&hUsbDeviceHS, &USBD_CDC);
USBD_CDC_RegisterInterface(&hUsbDeviceHS, &USBD_Interface_fops_HS);
USBD_Start(&hUsbDeviceHS);
```

That is the most important architectural difference.

---

## Files That Matter

These files are the CDC reference set:

- `UsbCdc.cpp`
- `CDC.cpp` / `CDC.h`
- `usbd_conf.c`
- `usbd_desc.c`
- `Makefile`

These are the files to study/port.

### `UsbCdc.cpp`
Contains:
- `DaisySeed hw;` plus `hw.Init()` for libDaisy compatibility
- `UsbCdc_Init()` startup
- `UsbCdc_TransmitString()` test send path
- no dependency on `hw.StartLog(false)`

### `CDC.cpp` / `CDC.h`
Contains:
- direct USB CDC initialization helper
- small transmit helpers for strings and formatted lines

### `usbd_conf.c`
Contains:
- PCD / USB low-level glue
- GPIO setup for USB peripheral
- IRQ setup
- FIFO sizing
- `USBD_LL_*` bridge functions

### `usbd_desc.c`
Contains:
- device descriptors
- product/manufacturer strings
- descriptor tables (`FS_Desc`, `HS_Desc`)

### `Makefile`
Contains:
- exact project-side source list needed for CDC
- no HID-only files
- explicit inclusion of `usbd_cdc.c`

---

## Porting Strategy

There are two possible approaches.

### Option A - Recommended
**Do not use `hw.StartLog(false)` for external CDC if the direct CDC path is the known-good reference.**
Instead, port the same explicit CDC init model used here.

That means the target application should have a dedicated CDC init path like:

```cpp
USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
USBD_RegisterClass(&hUsbDeviceHS, &USBD_CDC);
USBD_CDC_RegisterInterface(&hUsbDeviceHS, &USBD_Interface_fops_HS);
USBD_Start(&hUsbDeviceHS);
```

This avoids relying on the libDaisy logging abstraction for the external USB path.

### Option B - Not Recommended Initially
Try to fix `hw.StartLog(false)` / libDaisy CDC fallback.

This may be worth doing later, but it is less direct because:
- it depends on libDaisy internals
- weak-symbol / USB ownership changes already caused regressions
- it makes debugging harder

In practice, Option A is the faster route back to working CDC.

---

## Recommended Port Steps

### 1. Add a dedicated CDC module
Create a new folder in the target application, for example:

- `usb-cdc/`

Add files modeled after this project:
- `usb-cdc/usbd_conf.c`
- `usb-cdc/usbd_desc.c`
- `usb-cdc/CDC.cpp` / `usb-cdc/CDC.h`

Do **not** mix these into the HID folder.

---

### 2. Update the application Makefile for CDC mode
When `USE_USB_CDC=1`, add CDC-specific sources, similar to:

```make
C_SOURCES += \
 usb-cdc/usbd_conf.c \
 usb-cdc/usbd_desc.c \
 $(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c
```

```make
CPP_SOURCES += \
 usb-cdc/CDC.cpp
```

Also add any needed includes for CDC mode.

Important: do **not** include HID-only sources in CDC mode.

---

### 3. In CDC mode, stop using `hw.StartLog(false)`
Replace the `hw.StartLog(false)` CDC path in the target application:

```cpp
#if USE_USB_CDC
hw.StartLog(false);
#elif USE_USB_HID
UsbHid_Init();
System::Delay(1500);
UsbHid_ClearAllKeys();
#endif
```

with a direct CDC init path.

Something like:

```cpp
#if USE_USB_CDC
UsbCdc_Init();
#elif USE_USB_HID
UsbHid_Init();
System::Delay(1500);
UsbHid_ClearAllKeys();
#endif
```

Where `UsbCdc_Init()` wraps the working init sequence from this example.

---

### 4. Verify which handle/descriptor path is correct
This example uses:

```cpp
extern USBD_HandleTypeDef hUsbDeviceHS;
USBD_Init(&hUsbDeviceHS, &HS_Desc, DEVICE_HS);
```

Even though the external port enumerates as full-speed, this path is what worked in this project.

Do **not** assume the target application should switch to `FS_Desc` or `hUsbDeviceFS` without testing.
The known-good reference is the behavior in this project.

Port it exactly first.

---

### 5. Keep CDC transmit simple at first
Before integrating full logging, add a simple known-good send path like:

```cpp
UsbCdc_TransmitString("hello\r\n");
```

Use it for a minimal heartbeat/test string first.

Do not immediately wire the whole application logging system into CDC until basic enumeration + transmit are proven.

---

### 6. Test in layers
Test in this order:

1. CDC enumerates on external USB port
2. Host creates `/dev/ttyACM*`
3. The application sends a repeating test string
4. Only after that, integrate real application messages/logging

Do not jump straight from "USB init compiles" to "full app logging should work."

---

## Weak Symbol / libDaisy Notes

A prior regression showed that changing certain USB-related symbols to `weak` can break the default CDC startup path used by `hw.StartLog(false)`.

Practical implication:
- if the target application uses the explicit CDC path from this project, it is less dependent on that fragile fallback path
- do not assume the libDaisy logging USB ownership model is safe after local USB changes

If the target application keeps HID and CDC side-by-side:
- HID should keep owning only HID-specific pieces
- CDC should use its own explicit setup
- avoid ambiguous ownership through generic weak-symbol tricks when possible

---

## Symptoms This Port Should Fix

This port is specifically meant to address cases where:
- the app boots fine until `hw.StartLog(false)` is enabled
- HID works but CDC fallback does not
- external USB CDC does not enumerate correctly in the app
- the known-good `usb-cdc` test works, but the app CDC path does not

---

## What Not To Do

- Do not keep using `hw.StartLog(false)` and assume it will behave like this project
- Do not mix HID descriptors or HID IRQ/config files into CDC mode
- Do not change FS/HS handle selection speculatively before first porting the known-good path exactly
- Do not try to solve this first with more weak-symbol tricks

---

## Minimal Port Goal

The first success criterion is:
- build in CDC mode
- boot successfully
- enumerate as a CDC ACM device on the external USB port
- send a simple repeating text message

Once that works, expand from there.

---

## Suggested Next Porting Work

1. Create `usb-cdc/` folder in the target application
2. Port `usbd_conf.c` and `usbd_desc.c`
3. Add a tiny `UsbCdc_Init()` helper based on `UsbCdc.cpp`
4. Replace `hw.StartLog(false)` in CDC mode
5. Add a tiny periodic `CDC_Transmit_HS()` test message
6. Verify host sees `/dev/ttyACM*`
7. Then integrate real CDC output into the app

---

## Reference Behavior Observed

On the Pi host, this project successfully enumerated as:
- product string: `USB CDC Sample`
- Linux device: `/dev/ttyACM1`

And it successfully transmitted:
- `LED: ON`
- `LED: OFF`

That is the known-good baseline to reproduce inside the target application.
