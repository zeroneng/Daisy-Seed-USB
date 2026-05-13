# usb-midi

## Purpose

This folder contains a USB MIDI sample for Daisy Seed-based development.
It is intended to validate USB MIDI enumeration and MIDI message transport between the Daisy target and a host.

## Main files

- `UsbMidi.cpp` — main application/sample entry point
- `usbd_conf.c` / `usbd_conf.h` — USB device configuration glue
- `usbd_desc.c` / `usbd_desc.h` — device descriptors for the MIDI-capable USB identity
- `Makefile` — build rules

## How it works

At a high level this sample:

1. initializes the USB device stack on the target
2. presents descriptors that identify the device as a MIDI-capable USB function
3. handles MIDI event/message flow through the application and USB stack
4. lets the host communicate with the device as a USB MIDI endpoint

## Notes

This is highly relevant to music-device workflows and may be closer to RHYTHM-style use cases than generic CDC/HID examples.
