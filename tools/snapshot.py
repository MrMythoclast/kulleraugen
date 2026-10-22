"""Grab a frame from the device and save it as PNG.

python tools/snapshot.py COM23 out.png [befehl ...]

Extra arguments are sent as serial commands first (e.g. p f m2 w1).
"""
import sys
import time

import serial
from PIL import Image


def main():
    port, out = sys.argv[1], sys.argv[2]
    cmds = sys.argv[3:]
    with serial.Serial() as s:
        s.port, s.baudrate, s.timeout = port, 115200, 3
        s.dtr = s.rts = False  # opening must not reset the board
        s.open()
        for c in cmds:
            s.write((c + "\n").encode())
            time.sleep(0.15)
        if cmds:
            time.sleep(1.2)  # let mood blends settle
        s.reset_input_buffer()
        s.write(b"s\n")
        deadline = time.time() + 10
        while True:
            if time.time() > deadline:
                sys.exit("no snapshot received")
            ln = s.readline()
            if ln.startswith(b"SNAP"):
                break
        _, w, h = ln.split()
        w, h = int(w), int(h)
        raw = s.read(w * h * 2)
        if len(raw) != w * h * 2:
            sys.exit(f"short read: {len(raw)}")
    img = Image.new("RGB", (w, h))
    px = []
    for i in range(0, len(raw), 2):
        v = (raw[i] << 8) | raw[i + 1]  # sprite stores big-endian RGB565
        r, g, b = v >> 11, (v >> 5) & 63, v & 31
        px.append((r * 255 // 31, g * 255 // 63, b * 255 // 31))
    img.putdata(px)
    img.save(out)
    print("saved", out)


if __name__ == "__main__":
    main()
