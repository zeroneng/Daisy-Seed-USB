# usb-cdc

## Purpose

This folder contains a USB CDC (virtual serial) sample for Daisy Seed-based development.
CDC is often one of the simplest USB device classes to validate because it is easy to test enumeration and basic data flow from a host computer.

## Main files

- `UsbCdc.cpp` — main application/sample entry point
- `usbd_conf.c` / `usbd_conf.h` — USB device hardware/config glue
- `usbd_desc.c` / `usbd_desc.h` — USB descriptors for the device
- `Makefile` — build rules for the sample

## How it works

At a high level this sample:

1. initializes the target and USB device stack
2. describes itself to the host through the descriptor files
3. uses the CDC class behavior to expose a serial-style USB interface
4. allows basic host/device communication once enumerated

## Notes

This is typically a good minimal bring-up step when validating a new USB hardware path.
