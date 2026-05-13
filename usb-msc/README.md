# usb-msc

## Purpose

This folder contains a USB Mass Storage Class (MSC) sample for Daisy Seed-based development.
It is intended to let the target appear as a storage device to a host computer.

## Main files

- `UsbMscMain.cpp` — main application/sample entry point
- `UsbMscSdBackend.cpp` — storage backend logic, likely tying USB MSC behavior to SD/media access
- `usbd_msc_storage.c` / `usbd_msc_storage.h` — MSC storage interface glue
- `usbd_conf.c` / `usbd_conf.h` — USB device configuration glue
- `usbd_desc.c` / `usbd_desc.h` — device descriptors
- `Makefile` — build rules

## How it works

At a high level this sample:

1. initializes the USB device stack
2. presents the device as a USB storage-class target
3. maps storage read/write requests into the backend implementation
4. exposes storage media to the host through USB MSC protocols

## Notes

This sample is useful for SD/media exposure experiments and host-visible storage workflows.
