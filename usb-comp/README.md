# usb-comp

## Purpose

This folder contains a small composite USB device experiment.
A composite device combines more than one USB function into a single USB device configuration.

Current default: CDC ACM serial + NKRO HID keyboard.

## Main files

- `UsbComp.cpp` — main application entry point for the composite-device sample
- `usb_comp_cdc_if.c` / `usb_comp_cdc_if.h` — CDC-specific interface glue used by the composite device
- `usbd_composite_builder.c` — local composite descriptor builder patched for keyboard/NKRO HID descriptors
- `usbd_conf.c` / `usbd_conf.h` — USB low-level/device configuration, including TX FIFOs for HID EP1, CDC data EP2, and CDC command EP3
- `usbd_desc.c` / `usbd_desc.h` — composite-device descriptors
- `test_usb_comp.sh` — host-side CDC/HID validation helper
- `Makefile` — build rules

## How it works

At a high level this sample:

1. initializes the USB device stack
2. registers enabled USB classes into the composite builder
3. exposes CDC ACM serial when `USB_COMP_ENABLE_CDC=1`
4. exposes a 33-byte NKRO keyboard HID report when `USB_COMP_ENABLE_HID=1`
5. optionally emits CDC and HID test traffic when the test flags are enabled

## Build Flags

Defaults:

```bash
make
```

Equivalent explicit flags:

```bash
make USB_COMP_ENABLE_CDC=1 USB_COMP_ENABLE_HID=1 USB_COMP_TEST_CDC=1 USB_COMP_TEST_HID=1
```

Useful variants:

```bash
make USB_COMP_ENABLE_CDC=1 USB_COMP_ENABLE_HID=0
make USB_COMP_ENABLE_CDC=0 USB_COMP_ENABLE_HID=1
make USB_COMP_TEST_CDC=0 USB_COMP_TEST_HID=0
```

Feature flags control USB descriptors/classes. Test flags only control firmware-generated test traffic.

## Host Validation

After flashing the default build:

```bash
bash ./test_usb_comp.sh
```

CDC-only validation:

```bash
TEST_HID=0 bash ./test_usb_comp.sh
```

HID-only validation:

```bash
TEST_CDC=0 bash ./test_usb_comp.sh
```

The validation helper uses only bash and Python standard-library calls. It opens
the CDC ACM port read/write, sends one wake byte, expects a `COMP CDC...`
response, then reads Linux input events directly and expects `KEY_A` activity
from the NKRO HID report.

## Notes

This folder is useful when moving beyond single-class samples like CDC, HID, MIDI, or MSC.
