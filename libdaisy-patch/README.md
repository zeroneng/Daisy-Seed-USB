# libdaisy-patch

This folder now contains only the libDaisy-side patch file needed for the USB HID work.

Contents:
- `src/hid/usb.cpp`

Purpose:
- adds the `LIBDAISY_DISABLE_FS_USB_IRQ_HANDLERS` guard around the default FS USB IRQ handlers in libDaisy
- allows the `daisy-usb/usb-hid` project to provide its own FS IRQ handlers without symbol collision

Important:
- the actual `usb-hid` project files belong in `daisy-usb/usb-hid/`
- they should not be duplicated here

## Current recommendation

The current preferred libDaisy-side fix is to make the default FS USB IRQ handlers in `src/hid/usb.cpp` weak, while leaving `hUsbDeviceHS` and `hUsbDeviceFS` owned by libDaisy. Projects with custom HID USB stacks can then provide strong local IRQ handlers and declare the USB device handles as `extern` instead of redefining them.
