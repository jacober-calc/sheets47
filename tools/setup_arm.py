#!/usr/bin/env python3
"""Verify and unpack the approved portable compiler inside this repository.

Download the archive from the URL below first (resume with curl -C - if needed).
No installers, global PATH changes, cleanup, or modifications outside .local/.
"""
import hashlib
from pathlib import Path
import tarfile

ROOT=Path(__file__).resolve().parents[1]
NAME='arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi'
URL=f'https://developer.arm.com/-/media/Files/downloads/gnu/14.3.rel1/binrel/{NAME}.tar.xz'
SHA256='8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd'


def main():
    archive=ROOT/'.local/downloads'/f'{NAME}.tar.xz'
    if not archive.exists():
        raise SystemExit(f'Download {URL}\nto {archive}; published SHA256 {SHA256}')
    digest=hashlib.sha256()
    with archive.open('rb') as stream:
        for chunk in iter(lambda:stream.read(1024*1024),b''):digest.update(chunk)
    if digest.hexdigest()!=SHA256:raise SystemExit('Archive checksum mismatch or download incomplete; nothing unpacked')
    destination=ROOT/'.local/tools'
    if (destination/NAME).exists():raise SystemExit('Tool directory already exists; inspect it rather than overwriting or deleting')
    destination.mkdir(parents=True,exist_ok=True)
    with tarfile.open(archive) as tar:
        for member in tar.getmembers():
            if (member.name!=NAME and not member.name.startswith(NAME+'/')) or '..' in Path(member.name).parts:
                raise SystemExit('Unexpected archive path')
        tar.extractall(destination,filter='data')
    print('Verified and unpacked',destination/NAME)


if __name__=='__main__':main()
