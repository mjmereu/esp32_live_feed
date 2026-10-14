"""Live view of an ESP32-S3 camera over USB, for focusing the lens.

Run:   python viewer.py              (auto-picks the serial port)
       python viewer.py COM5         (Windows)   /   python viewer.py /dev/ttyUSB0   (Mac/Linux)

Keys (click the video window first):
  1-7  picture size: 1=320x240  2=640x480  3=800x600  4=1024x768  5=1280x1024  6=1600x1200  7=2048x1536
  r    reset the "best focus" peak
  q    quit (or Esc)

The focus score is the sharpness of the centre box. Twist the lens until the number is as
high as you can get it, and the peak stops growing. Compare scores only at the same size.
"""
import argparse
import struct
import sys
import time

import cv2
import numpy as np
import serial
import serial.tools.list_ports

MAGIC = b"\xA5\x5A\xC3\x3C"
MAX_JPEG = 1_500_000   # sanity limit for a frame length


def find_port():
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        sys.exit("No serial ports found. Plug the board into the TTL USB-C port.")
    preferred = [p for p in ports if any(k in (p.description + p.hwid).upper()
                                         for k in ("CH340", "CH341", "CP210", "UART", "USB-SERIAL", "USB SERIAL"))]
    return (preferred or ports)[0].device


def read_frame(ser):
    """Return one JPEG as bytes, or None on timeout. Skips anything that isn't a frame."""
    window = bytearray()
    while True:                         # find the magic bytes
        b = ser.read(1)
        if not b:
            return None
        window += b
        if len(window) > 4:
            del window[0]
        if bytes(window) == MAGIC:
            break
    raw_len = ser.read(4)
    if len(raw_len) < 4:
        return None
    (length,) = struct.unpack("<I", raw_len)
    if not 0 < length <= MAX_JPEG:
        return b""                      # garbage length: caller just tries again
    data = ser.read(length)
    if len(data) < length:
        return None
    return data


def focus_score(gray):
    h, w = gray.shape
    roi = gray[h // 4: 3 * h // 4, w // 4: 3 * w // 4]
    return cv2.Laplacian(roi, cv2.CV_64F).var()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("port", nargs="?", help="serial port (default: auto)")
    ap.add_argument("--baud", type=int, default=921600, help="must match BAUD in the sketch")
    args = ap.parse_args()

    port = args.port or find_port()
    ser = serial.Serial()
    ser.port, ser.baudrate, ser.timeout = port, args.baud, 2
    ser.dtr = ser.rts = False           # don't reset the board when opening the port
    ser.open()
    print(f"Reading {port} at {args.baud} baud. Press q in the video window to quit.")

    cv2.namedWindow("ESP32 live feed", cv2.WINDOW_NORMAL)
    peak, frames, t0, fps = 0.0, 0, time.time(), 0.0
    while True:
        data = read_frame(ser)
        if data is None:
            print("Waiting for frames... (tap RST on the board; is the sketch's BAUD the same?)")
        elif data:
            img = cv2.imdecode(np.frombuffer(data, np.uint8), cv2.IMREAD_COLOR)
            if img is not None:
                gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
                score = focus_score(gray)
                peak = max(peak, score)
                frames += 1
                if time.time() - t0 >= 1:
                    fps, frames, t0 = frames / (time.time() - t0), 0, time.time()
                h, w = gray.shape
                cv2.rectangle(img, (w // 4, h // 4), (3 * w // 4, 3 * h // 4), (0, 255, 0), 1)
                label = f"{w}x{h}  {fps:.1f} fps  focus {score:.0f}  peak {peak:.0f}"
                cv2.putText(img, label, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 0), 4)
                cv2.putText(img, label, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 1)
                cv2.imshow("ESP32 live feed", img)

        key = cv2.waitKey(1) & 0xFF
        if key in (ord("q"), 27):
            break
        if key == ord("r"):
            peak = 0.0
        if ord("1") <= key <= ord("7"):
            ser.write(bytes([key]))
            peak = 0.0                  # scores aren't comparable across sizes
        if cv2.getWindowProperty("ESP32 live feed", cv2.WND_PROP_VISIBLE) < 1:
            break

    ser.close()
    cv2.destroyAllWindows()


if __name__ == "__main__":
    main()
