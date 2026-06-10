# basic-usb

Minimal Daisy Seed blink plus audio callback app.

- Initializes the Daisy Seed.
- Toggles the onboard LED every 500 ms.
- Result: one full blink cycle per second.
- Starts a libDaisy audio callback.
- Outputs a 100 Hz sine tone on left and right audio outputs.
- Passes line input straight to the matching output channel.

Build and flash:

```bash
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make clean
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make -j2
PATH=/home/pi/Developer/gcc-arm-none-eabi-10-2020-q4-major/bin:$PATH make program
```
