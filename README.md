# daisy-usb

USB experiments for Daisy Seed, focused on external USB bring-up.

## Current status

Confirmed working on external USB-C wiring:
- `examples/seed/UsbAudioForum` — USB audio example adapted from Daisy forum code for the external USB path
- `examples/seed/TinyUsbCdcExternal` — TinyUSB CDC over libDaisy external USB init

Additional experimental work included:
- `examples/seed/TinyUsbHidExternal`
- `examples/seed/TinyUsbCdcHidExternal`

## Notes

- External USB path on this Daisy setup uses the libDaisy external USB peripheral path.
- The USB audio example is based on forum code using ST USB middleware, adapted for the external USB connector.
- TinyUSB CDC was proven working using libDaisy for hardware bring-up and TinyUSB for the class/device stack.

## Hardware context

- Daisy Seed with external USB-C wiring
- ST-Link flashing used during development
