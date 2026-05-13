# usb-audio

## Purpose

This folder contains a USB Audio experiment/sample for Daisy Seed-based development.
It is intended to explore how a Daisy/libDaisy-based target can enumerate and behave as a USB audio device.

## Main files

- `UsbAudio.cpp` — main application entry point for the sample
- `usbd_conf.c` / `usbd_conf.h` — STM32 USB device low-level configuration glue
- `usbd_desc.c` / `usbd_desc.h` — USB descriptors
- `usbd_audio.c` / `usbd_audio.h` — USB audio class support code
- `usbd_audio_if.c` / `usbd_audio_if.h` — audio interface glue between app logic and USB class handling
- `Makefile` — build rules for this sample

## How it works

At a high level this sample:

1. initializes the Daisy target and USB peripheral path
2. configures USB device middleware through `usbd_conf.*`
3. exposes itself to the host as a USB audio-capable device using the class and descriptor code
4. routes audio/interface behavior through the audio interface glue files

## Notes

This folder is for focused USB audio experimentation rather than a general-purpose libDaisy example tree.
