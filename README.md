# daisy-usb

USB experiments for the Daisy Seed.

This repository is part of a larger effort to explore and enable USB features
on the Daisy Seed external USB port.

## Current Projects

- `basic-usb` - internal-flash app that reuses the shared `usb-comp` stack.
- `usb-comp` - composite CDC, HID, audio, MIDI, and optional MSC reference.
- `usb-cdc` - standalone CDC ACM reference.
- `usb-hid` - standalone HID keyboard/reference path.
- `libdaisy-patch` - reference patch copy for optional libDaisy USB IRQ handler changes.

The removed standalone audio, MIDI, DFU, MSC, and top-level vendor experiments
are not part of the active tree anymore. Recoverable copies were moved to the
NAS trash during cleanup.

Most of this code was written or modified with the help of AI agents. Use at
your own risk, test carefully, and verify behavior before using it in production
hardware.

## License

MIT License

Copyright (c) 2026

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the “Software”), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
