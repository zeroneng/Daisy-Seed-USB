# example-hid

Minimal Daisy Seed app showing how to use `../usb-hid`.

It initializes Daisy, starts the HID keyboard stack, blinks the LED every
500 ms, and sends a repeating `A` key tap by default for host validation.

## Files

- `ExampleHid.cpp` - tiny app using `HID.h`
- `Makefile` - explicit `../usb-hid` source/include list
- `README.md`

The HID implementation comes from `../usb-hid`; there is no extra `.mk` helper.

## App Steps

```cpp
#include "daisy_seed.h"
#include "HID.h"

DaisySeed hw;

int main()
{
    hw.Init();
    UsbHid_Init();
    UsbHid_ClearAllKeys();

    while(true)
    {
        hw.SetLed(true);
        UsbHid_KeyOn(UsbHid_CharToKeycode('a'));
        System::Delay(40);
        UsbHid_KeyOff(UsbHid_CharToKeycode('a'));

        hw.SetLed(false);
        System::Delay(460);
    }
}
```

For product code, keep generated key events behind `EXAMPLE_HID_TEST_KEYS` or a
real input condition.

## Makefile Steps

```make
USB_HID_DIR = ../usb-hid

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
```

Do not compile `usb_irq_override.c`; libDaisy owns the FS USB IRQ handlers.

## Build And Flash

```bash
cd /mnt/clu-nas/developer/daisy-usb/example-hid
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make clean
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make -j2
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make program
```

## Host Test

```bash
lsusb | grep '0483:5751'
ls -l /dev/input/by-id | grep 'USB_HID_Sample.*event-kbd'
```

Read the event node and confirm `KEY_A` press/release events. Repeats
(`KEY_A 2`) may appear while the key is held.

Quiet build with HID present but no generated key taps:

```bash
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH \
make clean
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH \
make -j2 EXAMPLE_HID_TEST_KEYS=0
```
