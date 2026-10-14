# esp32_live_feed

Live camera view over the USB cable (no WiFi) for focusing the ESP32-S3 + OV3660 boards.
The board streams JPEG frames over the TTL USB-C port, and `viewer.py` shows them on your
computer with a **focus score** (sharpness of the centre box) so you can twist the lens until
the number peaks.

## One-time setup

1. **Python deps** (needs Python 3): `pip install -r requirements.txt`
2. **Flash the board** with `usb_live_feed/usb_live_feed.ino` from the Arduino IDE:
   - Board **ESP32S3 Dev Module**
   - **PSRAM: OPI PSRAM**
   - **USB CDC On Boot: Disabled**
   - Plug into the port labelled **TTL** (not OTG).

## Use

1. **Close the Arduino Serial Monitor** (only one program can use the port).
2. Run `python viewer.py` (auto-picks the port), or name it:
   `python viewer.py COM5` (Windows) / `python viewer.py /dev/ttyUSB0` (Mac, Linux).
3. Click the video window, then use the keys:

| Key | Action |
|---|---|
| `1` to `7` | Picture size: 320x240, 640x480, 800x600, 1024x768, 1280x1024, 1600x1200, 2048x1536 |
| `r` | Reset the "peak" focus score |
| `q` or Esc | Quit |

## Focusing tips

- Mount the camera at its real working distance from the cylinder first, with the normal lighting.
- Gently twist the lens barrel (grip it with a rubber band) while watching the `focus` number.
  Higher is sharper. Keep going until the number stops improving, then back off slightly if it dropped.
- Compare scores only at the same picture size (the peak resets when you change size).
- Use size `2` (640x480) while turning for a smooth picture, then check at `5` or `7` for the final result.

## Troubleshooting

| Problem | Fix |
|---|---|
| "Waiting for frames..." | Tap RST on the board. Make sure `BAUD` in the sketch equals `--baud` in `viewer.py` (default 921600). Close the Serial Monitor. |
| "Camera init failed" text in the Serial Monitor | Set Tools > PSRAM to OPI PSRAM, re-flash, and check the cable. |
| Choppy picture | Use a smaller size (`1` or `2`), or raise `BAUD` to 2000000 in the sketch and run `python viewer.py --baud 2000000` (needs a USB-serial chip that supports it). |
| No serial port found | Use the TTL port, a data-capable USB cable, and install the CH340/CP210x driver if your OS needs it. |
