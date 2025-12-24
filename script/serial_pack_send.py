#!/usr/bin/env python3
import argparse
import struct
import sys
from pathlib import Path

import serial


def main() -> int:
    parser = argparse.ArgumentParser(description="Send a serial pack over USB Serial/JTAG.")
    parser.add_argument("--port", default="/dev/cu.usbmodem1101", help="Serial device path")
    parser.add_argument("--path", default="sync", help="Pack path (no spaces)")
    parser.add_argument("--data", help="Payload string")
    parser.add_argument("--file", help="Binary payload file")
    parser.add_argument("--baud", type=int, default=460800, help="Baud rate")
    args = parser.parse_args()

    path_bytes = args.path.encode("ascii")
    if b" " in path_bytes or b"\n" in path_bytes:
        print("path must not contain spaces or newlines", file=sys.stderr)
        return 2

    if args.data is None and args.file is None:
        print("must provide --data or --file", file=sys.stderr)
        return 2
    if args.data is not None and args.file is not None:
        print("use only one of --data or --file", file=sys.stderr)
        return 2

    if args.file is not None:
        file_path = Path(args.file)
        if not file_path.exists():
            print(f"file not found: {file_path}", file=sys.stderr)
            return 2
        data_bytes = file_path.read_bytes()
    else:
        data_bytes = args.data.encode("utf-8")
    pkt = path_bytes + b"\n" + struct.pack("<I", len(data_bytes)) + data_bytes

    with serial.Serial(args.port, args.baud, timeout=1, write_timeout=2) as ser:
        offset = 0
        while offset < len(pkt):
            written = ser.write(pkt[offset:])
            if written is None or written <= 0:
                print("serial write failed", file=sys.stderr)
                return 1
            offset += written
        ser.flush()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
