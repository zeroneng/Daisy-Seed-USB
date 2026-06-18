# usb-hid

Standalone USB HID keyboard support for Daisy Seed projects.

Use this when the device only needs to be a USB keyboard. Use `usb-comp` when
the same USB device also needs CDC, audio, MIDI, or MSC.

## Files

- `HID.h` / `HID.cpp` - public HID helper API and NKRO report state
- `usbd_hid_kbd.c` - HID class descriptor and report transport
- `usbd_conf.c` / `usbd_conf.h` - USB PCD glue
- `usbd_desc.c` / `usbd_desc.h` - USB device/string descriptors
- `vendor/hid/Inc/usbd_hid.h` - local HID class header
- `UsbHid.cpp` - standalone test app
- `Makefile` - standalone build

Do not add `usb_irq_override.c`. Current libDaisy owns the FS USB IRQ handlers.

## App API

```cpp
#include "HID.h"

UsbHid_Init();
UsbHid_KeyOn(keycode);
UsbHid_KeyOff(keycode);
UsbHid_ClearAllKeys();
```

For a scanned key matrix:

```cpp
UsbHid_SetKeyState(keycode, pressed);
UsbHid_SendReport();
```

Call `UsbHid_SendReport()` once after updating all keys in a scan pass.

## Drop-In Makefile

```make
TARGET = MyApp

LIBDAISY_DIR = ../../libDaisy
USB_HID_DIR = ../usb-hid

CPP_SOURCES = MyApp.cpp

C_SOURCES += \
$(USB_HID_DIR)/usbd_conf.c \
$(USB_HID_DIR)/usbd_desc.c \
$(USB_HID_DIR)/usbd_hid_kbd.c

CPP_SOURCES += \
$(USB_HID_DIR)/HID.cpp

C_INCLUDES += \
-I$(USB_HID_DIR) \
-I$(USB_HID_DIR)/vendor/hid/Inc

C_DEFS += \
-DHID_FS_BINTERVAL=0x01U

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
```

## App Startup

```cpp
DaisySeed hw;

int main()
{
    hw.Init();
    UsbHid_Init();
    UsbHid_ClearAllKeys();

    while(true)
    {
        UsbHid_KeyOn(UsbHid_CharToKeycode('a'));
        System::Delay(40);
        UsbHid_KeyOff(UsbHid_CharToKeycode('a'));
        System::Delay(460);
    }
}
```

## Build And Test

```bash
cd /mnt/clu-nas/developer/Daisy-Seed-USB/usb-hid
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make clean
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make -j2
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make program
```

Expected host checks:

```bash
lsusb | grep '0483:5751'
ls -l /dev/input/by-id | grep 'USB_HID_Sample.*event-kbd'
```

The standalone app sends repeated `A` key taps. Validate real input events from
the `/dev/input/by-id/*USB_HID_Sample*event-kbd` node.
