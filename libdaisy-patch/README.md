# libdaisy-patch

This folder contains a reference libDaisy-side patch file for projects that
need to override libDaisy's default full-speed USB interrupt handlers.

Contents:
- `src/hid/usb.cpp`

Purpose:
- shows how to make libDaisy's default full-speed USB IRQ handlers weak
- allows a project with its own FS USB interrupt handlers to provide strong
  local replacements without duplicate-symbol link errors

Important:
- the actual `usb-hid` project files belong in `daisy-usb/usb-hid/`
- they should not be duplicated here
- the current `daisy-usb/usb-hid` build does not compile its old local
  `usb_irq_override.c`; it uses libDaisy's FS USB IRQ handlers instead

## Current recommendation

Only apply this patch when a target project intentionally provides its own
strong local definitions for these handlers from `src/hid/usb.cpp`:

- `OTG_FS_EP1_OUT_IRQHandler`
- `OTG_FS_EP1_IN_IRQHandler`
- `OTG_FS_IRQHandler`

Leave `hUsbDeviceHS` and `hUsbDeviceFS` owned by libDaisy. Projects with custom
USB stacks can then provide strong local IRQ handlers and declare the USB device
handles as `extern` instead of redefining them.
