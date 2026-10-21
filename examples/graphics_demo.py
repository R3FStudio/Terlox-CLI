"""Render an RGB plot as a real CLI image without Pillow or a web runtime."""
import math
import struct
import zlib
from protocol import png

width, height = 576, 192
pixels = bytearray()
for y in range(height):
    pixels.append(0)  # PNG scanline filter
    for x in range(width):
        wave = height / 2 - math.sin(x / 42) * 58
        grid = x % 48 == 0 or y % 32 == 0
        if abs(y - wave) < 2:
            pixels.extend((110, 236, 215))
        elif abs(y - (height / 2 - math.cos(x / 60) * 38)) < 2:
            pixels.extend((246, 104, 139))
        else:
            pixels.extend((27, 42, 61) if grid else (10 + x * 12 // width, 16, 26 + y * 12 // height))

def chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))

image = b'\x89PNG\r\n\x1a\n'
image += chunk(b'IHDR', struct.pack('>2I5B', width, height, 8, 2, 0, 0, 0))
image += chunk(b'IDAT', zlib.compress(bytes(pixels))) + chunk(b'IEND', b'')
print('\033[1;38;2;246;104;139mTERLOX / CLI GRAPHICS\033[0m')
png(image, 72, 12)
print('\n' * 13 + 'RGB PNG plot rendered by the terminal GPU.')
