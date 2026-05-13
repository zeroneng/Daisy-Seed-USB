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

## Notes

This folder is useful for validating report-based USB device behavior rather than serial/audio/storage behavior.
