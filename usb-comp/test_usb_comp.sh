#!/usr/bin/env bash
set -euo pipefail

TEST_CDC="${TEST_CDC:-1}"
TEST_HID="${TEST_HID:-1}"
CDC_TIMEOUT="${CDC_TIMEOUT:-5}"
HID_TIMEOUT="${HID_TIMEOUT:-6}"
PRODUCT_PATTERN="${PRODUCT_PATTERN:-USB Composite Sample}"

find_cdc_port() {
  local link target
  for link in /dev/serial/by-id/*; do
    [ -e "$link" ] || continue
    case "$link" in
      *STLINK*) continue ;;
    esac
    target="$(readlink -f "$link")"
    if [[ "$target" == /dev/ttyACM* ]]; then
      printf '%s\n' "$target"
      return 0
    fi
  done

  for target in /dev/ttyACM*; do
    [ -e "$target" ] || continue
    printf '%s\n' "$target"
    return 0
  done

  return 1
}

find_hid_event() {
  local event name
  for event in /dev/input/event*; do
    [ -e "$event" ] || continue
    name="$(cat "/sys/class/input/${event##*/}/device/name" 2>/dev/null || true)"
    case "$name" in
      *Composite*|*HID*|*Keyboard*)
        printf '%s\n' "$event"
        return 0
        ;;
    esac
  done

  return 1
}

echo "USB devices matching '${PRODUCT_PATTERN}':"
lsusb | grep -F "$PRODUCT_PATTERN"

if [[ "$TEST_CDC" == "1" ]]; then
  cdc_port="$(find_cdc_port)"
  echo "CDC port: ${cdc_port}"
  cdc_output="$(CDC_PORT="$cdc_port" CDC_TIMEOUT="$CDC_TIMEOUT" python3 - <<'PY' || true
import os
import select
import termios
import time

port = os.environ["CDC_PORT"]
timeout_s = float(os.environ["CDC_TIMEOUT"])
fd = os.open(port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
try:
    attrs = termios.tcgetattr(fd)
    attrs[0] = 0
    attrs[1] = 0
    attrs[2] = attrs[2] | termios.CLOCAL | termios.CREAD
    attrs[3] = 0
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    os.write(fd, b"?")
    end = time.time() + timeout_s
    out = bytearray()
    while time.time() < end and len(out) < 256:
        readable, _, _ = select.select([fd], [], [], 0.2)
        if not readable:
            continue
        try:
            data = os.read(fd, 256)
        except BlockingIOError:
            continue
        if data:
            out.extend(data)
            if b"\n" in out:
                break
    print(out.decode("utf-8", "replace"), end="")
finally:
    os.close(fd)
PY
)"
  printf '%s\n' "$cdc_output"
  grep -Eq 'COMP CDC|COMP CDC NKRO ready' <<<"$cdc_output"
fi

if [[ "$TEST_HID" == "1" ]]; then
  hid_event="$(find_hid_event)"
  echo "HID event: ${hid_event}"
  hid_output="$(HID_EVENT="$hid_event" HID_TIMEOUT="$HID_TIMEOUT" python3 - <<'PY' || true
import os
import select
import struct
import time

event = os.environ["HID_EVENT"]
timeout_s = float(os.environ["HID_TIMEOUT"])
fmt = "llHHI"
size = struct.calcsize(fmt)
fd = os.open(event, os.O_RDONLY | os.O_NONBLOCK)
seen = []
try:
    end = time.time() + timeout_s
    while time.time() < end and len(seen) < 2:
        readable, _, _ = select.select([fd], [], [], 0.5)
        if not readable:
            continue
        data = os.read(fd, size * 16)
        for offset in range(0, len(data) - size + 1, size):
            _, _, etype, code, value = struct.unpack(fmt, data[offset:offset + size])
            if etype == 1 and code == 30:
                seen.append(value)
                print(f"KEY_A {value}")
                if len(seen) >= 2:
                    break
finally:
    os.close(fd)
PY
)"
  printf '%s\n' "$hid_output"
  grep -F 'KEY_A' <<<"$hid_output"
fi

echo "usb-comp host validation passed"
