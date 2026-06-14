# example-hid

Simple Daisy Seed blink app that reuses the standalone `usb-hid` helper stack.

## What It Does

- Initializes the Daisy Seed.
- Starts the USB HID keyboard device stack from `../usb-hid`.
- Blinks the onboard LED every 500 ms.
- Sends a repeating `A` key tap by default so the host can verify real HID
  input events.

## Project Shape

`example-hid` is intentionally small. The app-owned files are:

- `ExampleHid.cpp`
- `Makefile`
- `README.md`

The HID implementation is reused from `../usb-hid`:

- `HID.cpp`
- `HID.h`
- `usbd_conf.c`
- `usbd_desc.c`
- `usbd_hid_kbd.c`
- `vendor/hid/Inc/usbd_hid.h`

The Makefile does not compile `../usb-hid/usb_irq_override.c`; libDaisy owns the
FS USB IRQ handlers in the current tree.

## Build And Flash

Use the Daisy-compatible ARM toolchain on this Pi:

```bash
cd /mnt/clu-nas/developer/daisy-usb/example-hid
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make clean
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make -j2
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make program
```

Expected flash result:

```text
** Verified OK **
```

## Host Validation

Confirm USB enumeration:

```bash
lsusb | grep '0483:5751'
```

Expected device:

```text
STMicroelectronics USB HID Sample
```

Find the keyboard event node:

```bash
ls -l /dev/input/by-id | grep 'USB_HID_Sample.*event-kbd'
```

Read a few HID key events:

```bash
python3 - <<'PY'
import glob, os, select, struct, time

path = os.path.realpath(glob.glob('/dev/input/by-id/*USB_HID_Sample*event-kbd')[0])
print(f'HID event: {path}')

fmt = 'llHHI'
size = struct.calcsize(fmt)
fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
seen = []
try:
    end = time.time() + 6
    while time.time() < end and len(seen) < 4:
        readable, _, _ = select.select([fd], [], [], 0.5)
        if not readable:
            continue
        data = os.read(fd, size * 32)
        for offset in range(0, len(data) - size + 1, size):
            _, _, etype, code, value = struct.unpack(fmt, data[offset:offset + size])
            if etype == 1 and code == 30:
                seen.append(value)
                print(f'KEY_A {value}')
                if len(seen) >= 4:
                    break
finally:
    os.close(fd)

if 1 not in seen or 0 not in seen:
    raise SystemExit(f'Missing KEY_A press/release pair: {seen}')
PY
```

Expected output includes `KEY_A 1` and `KEY_A 0`. Some hosts may also report
`KEY_A 2` repeats while the key is held.

## Quiet Mode

To keep the HID keyboard interface present but stop firmware-generated key taps,
set the test flag to `0`:

```bash
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH \
make clean
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH \
make -j2 EXAMPLE_HID_TEST_KEYS=0
```

For product code, prefer keeping generated HID events behind a dedicated test
flag or input condition.
