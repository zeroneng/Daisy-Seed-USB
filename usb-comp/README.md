# usb-comp

## Purpose

This folder contains a composite USB device experiment.
A composite device combines more than one USB function into a single USB device configuration.

## Main files

- `UsbComp.cpp` — main application entry point for the composite-device sample
- `usb_comp_cdc_if.c` / `usb_comp_cdc_if.h` — CDC-specific interface glue used by the composite device
- `usbd_conf.c` / `usbd_conf.h` — USB low-level/device configuration
- `usbd_desc.c` / `usbd_desc.h` — composite-device descriptors
- `Makefile` — build rules

## How it works

At a high level this sample:

1. initializes the USB device stack
2. defines descriptors that expose multiple functions in one device
3. connects class/interface handlers such as CDC into the combined device behavior
4. presents the host with a composite USB identity instead of a single-function one

## Notes

This folder is useful when moving beyond single-class samples like CDC, HID, MIDI, or MSC.
