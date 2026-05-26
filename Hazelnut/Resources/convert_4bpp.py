"""Convert .4bpp (packed: 1 byte = 2 pixels, nibble-per-pixel) to .htileset v2 (unpacked: 1 byte = 1 pixel, value 0-15).

Usage: python convert_4bpp.py <input.4bpp> [input2.4bpp...]
Output: <input>.htileset  (same directory)
"""

import struct
import sys
import os


HEADER_FORMAT = '<4sHHI I'
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)  # 16 bytes

MAGIC = b'TSET'
VERSION = 2
TILE_WIDTH = 8    # pixels per subtile dimension
UNPACKED_PIXEL_WIDTH = 8  # 8 pixels per row, 1 byte each


def convert(input_path: str) -> None:
    with open(input_path, 'rb') as f:
        raw = f.read()

    raw_size = len(raw)                  # packed: 1 byte = 2 pixels
    pixel_height = raw_size * 2 // UNPACKED_PIXEL_WIDTH  # unpacked rows
    subtile_count = pixel_height // TILE_WIDTH

    # Unpack nibbles: 1 packed byte → 2 unpacked bytes (hi, lo)
    unpacked = bytearray(raw_size * 2)
    for i, b in enumerate(raw):
        unpacked[i * 2]     = b >> 4       # high nibble
        unpacked[i * 2 + 1] = b & 0x0F     # low nibble

    out_path = os.path.splitext(input_path)[0] + '.htileset'
    with open(out_path, 'wb') as f:
        header = struct.pack(HEADER_FORMAT,
            MAGIC,
            VERSION,
            TILE_WIDTH,
            UNPACKED_PIXEL_WIDTH,
            pixel_height
        )
        f.write(header)
        f.write(unpacked)

    print(f'{input_path}  →  {out_path}')
    print(f'  subtiles: {subtile_count}  ({raw_size} packed → {len(unpacked)} unpacked bytes)')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Usage: python convert_4bpp.py <input.4bpp> [...]')
        sys.exit(1)

    for path in sys.argv[1:]:
        if not os.path.isfile(path):
            print(f'SKIP: not a file: {path}')
            continue
        convert(path)
