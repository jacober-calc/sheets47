#!/usr/bin/env python3
"""Package and verify a DMCP5 flash image; no cleanup or hardware writes.

Size word + trailing SHA-1 follow SwissMicros' add_pgm_chsum format. SHA-1 here
is the loader's integrity format, not a publisher signature. Reject QSPI payloads.
"""
import hashlib
from pathlib import Path
import struct
import sys


def package(source, destination):
    data=bytearray(source.read_bytes())
    if len(data)<64 or struct.unpack_from('<I',data)[0]!=0xd377c0de:
        raise ValueError('Missing DMCP program header at image start')
    if len(data)>1024*1024:
        raise ValueError('Image exceeds the SDK flash region')
    if struct.unpack_from('<II',data,20)!=(0,0):
        raise ValueError('Unexpected QSPI dependency')
    if data[28:44].split(b'\0')[0]!=b'sheets47' or data[44:60].split(b'\0')[0]!=b'0.1.0':
        raise ValueError('Program identity/version mismatch')
    struct.pack_into('<I',data,4,len(data))
    output=bytes(data)+hashlib.sha1(data).digest()
    destination.parent.mkdir(parents=True,exist_ok=True)
    destination.write_bytes(output)
    verified=destination.read_bytes()
    assert hashlib.sha1(verified[:-20]).digest()==verified[-20:]
    print(destination,len(output),'bytes; SHA256',hashlib.sha256(output).hexdigest())


if __name__=='__main__':
    package(Path(sys.argv[1]),Path(sys.argv[2]))
