# libdaisy-patch

This folder contains the finalized file-overlay patch kit for the working Daisy USB HID keyboard setup.

## Verified outcome

This patch set was validated on the Raspberry Pi host against:
- libDaisy at `/mnt/clu-nas/developer/libDaisy`
- project at `/mnt/clu-nas/developer/daisy-usb/usb-hid`

Verified results:
- build succeeds
- flash succeeds
- USB enumeration succeeds
- no recurring `input irq status -75 received` spam in the stable final build
- actual host-side HID input was observed
- host saw a real `KEY_A` event from the device

Current validated mode:
- boot keyboard mode
- standard 8-byte boot keyboard report
- one-key slow test pattern (`A` press/release)

## Why the libDaisy change was needed

`libDaisy/src/hid/usb.cpp` defines the full-speed USB IRQ handlers:
- `OTG_FS_EP1_OUT_IRQHandler`
- `OTG_FS_EP1_IN_IRQHandler`
- `OTG_FS_IRQHandler`

The `usb-hid` sample also needs to own those handlers because it provides its own USB device glue and HID class path.
Without a guard, both libDaisy and the sample try to export the same IRQ symbols.

## The libDaisy change

Overlay this file into your libDaisy tree before rebuilding libDaisy:
- `src/hid/usb.cpp`

What changed in that file:
- added a compile-time guard around the FS USB IRQ handlers:
  - `LIBDAISY_DISABLE_FS_USB_IRQ_HANDLERS`
- when that macro is defined, libDaisy does not emit the default FS USB IRQ handlers
- the sample can then safely provide its own local FS IRQ handlers

## The project-side files to overlay

Copy these files into the `usb-hid` project tree:
- `usb-hid/Makefile`
- `usb-hid/usb_irq_override.c`
- `usb-hid/usbd_conf.c`
- `usb-hid/usbd_hid_kbd.c`
- `usb-hid/HID.cpp`
- `usb-hid/UsbHid.cpp`
- `usb-hid/vendor/hid/Inc/usbd_hid.h`

## Important behavior changes in the final working version

### 1. Boot keyboard mode
The final verified build uses a boot keyboard descriptor and boot keyboard interface settings:
- subclass: boot (`0x01`)
- protocol: keyboard (`0x01`)
- 8-byte interrupt IN endpoint/report path

### 2. HID state-path fix
The runtime `-75` host error stopped only after tightening the HID class state handling.
The important final behavior is in `usb-hid/usbd_hid_kbd.c`:
- `USBD_HID_DataIn()` only returns the HID class state to `IDLE` for the actual HID IN endpoint
- `USBD_HID_SendReport()` restores the class state to `IDLE` if `USBD_LL_Transmit()` fails

This was the key stability fix.

### 3. Idle and traffic validation
The final workflow verified:
- idle enumeration first
- then a minimal one-key test
- then actual host-side HID input observation

## Recommended install flow on another machine

1. Back up your current `libDaisy` tree and `usb-hid` project.
2. Copy `src/hid/usb.cpp` from this patch kit into your libDaisy source tree.
3. Rebuild libDaisy.
4. Copy the `usb-hid/` files from this patch kit into your project.
5. Build the project.
6. Flash the target.
7. Verify:
   - USB enumeration as a keyboard
   - no fresh `input irq status -75 received` spam
   - actual host-side key events appear

## Testing rule

Do not stop at enumeration.
Always verify actual host-side HID input events when possible.
