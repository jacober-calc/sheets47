#!/usr/bin/env python3
"""Convert actual 400x240 host-renderer PBM frames to PNG without extra packages.

This only renders original application output. No screenshots/fonts from other
applications are imported. Native files and enlarged previews are kept together.
"""
from pathlib import Path
import struct
import zlib

ROOT=Path(__file__).resolve().parents[1]


def chunk(tag,data):return struct.pack('>I',len(data))+tag+data+struct.pack('>I',zlib.crc32(tag+data))


def convert(path,output,scale):
    data=path.read_bytes();header=b'P4\n400 240\n'
    if not data.startswith(header) or len(data)!=len(header)+12000:raise ValueError('Unexpected PBM frame')
    pixels=data[len(header):];rows=[]
    for row in range(240):
        line=bytearray()
        for x in range(400):line.extend((b'\0' if pixels[row*50+x//8]&(0x80>>(x%8)) else b'\xff')*scale)
        rows.extend([b'\0'+line]*scale)
    png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',400*scale,240*scale,8,0,0,0,0))
    png+=chunk(b'IDAT',zlib.compress(b''.join(rows),9))+chunk(b'IEND',b'')
    output.write_bytes(png)


if __name__=='__main__':
    target=ROOT/'docs/screenshots';target.mkdir(exist_ok=True)
    for name in ['worksheet','range','editor']:
        source=ROOT/'build/host'/f'{name}.pbm'
        convert(source,target/f'{name}-0.1.0.png',1)
        convert(source,target/f'{name}-0.1.0-3x.png',3)
        print('Exported actual renderer frame:',name)
