# libdaisy-patch

This folder contains the libDaisy-side patch file needed for the standalone
USB HID work.

Contents:
- `src/hid/usb.cpp`

Purpose:
- makes libDaisy's default full-speed USB IRQ handlers weak
- allows `daisy-usb/usb-hid` or another project to provide strong local FS IRQ
  handlers without duplicate-symbol link errors

Important:
- the actual `usb-hid` project files belong in `daisy-usb/usb-hid/`
- they should not be duplicated here

## Current recommendation

The current preferred libDaisy-side fix is to make these handlers in
`src/hid/usb.cpp` weak:

- `OTG_FS_EP1_OUT_IRQHandler`
- `OTG_FS_EP1_IN_IRQHandler`
- `OTG_FS_IRQHandler`

Leave `hUsbDeviceHS` and `hUsbDeviceFS` owned by libDaisy. Projects with custom
USB stacks can then provide strong local IRQ handlers and declare the USB
device handles as `extern` instead of redefining them.
