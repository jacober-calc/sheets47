# Arm GNU runtime notices

Toolchain: Arm GNU Toolchain 14.3.rel1, x86_64 host, arm-none-eabi target.
Archive SHA-256:
`8f6903f8ceb084d9227b9ef991490413014d991874a1e34074443c2a72b14dbd`.

`LICENSES.txt` is the complete, unmodified `license.txt` from that official
toolchain archive. It includes notices for more tools than are linked into the
application, so obligations can be inspected without hiding any bundle notices.
GCC/libstdc++/libgcc runtime provisions and newlib notices apply to selected
runtime pieces. Compiler executables remain ignored under `.local/tools/`.

SwissMicros' official DMCP5 SDK README directs developers to Arm for the GNU ARM
compiler. Its historical GCC 10.3 example predates Spreader's C++20 requirements.
This project pins 14.3.rel1 and records the actual compiler version in every
build's JSON. No toolchain or firmware is installed system-wide.

- https://github.com/swissmicros/DMCP5_SDK/blob/71a04c2bb2e694da99e62318c1dc1b2ee5b883b8/README
- https://learn.arm.com/install-guides/gcc/arm-gnu/
- Archive: https://developer.arm.com/-/media/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz
- Published digest: same URL with `.sha256asc` appended.

See `tools/setup_arm.py` and `docs/building.md` for local reproduction. Runtime
source revisions are identified in the archive's
`14.3.rel1-x86_64-arm-none-eabi-manifest.txt`; that manifest is retained with the
local toolchain. Release source and binary notices accompany the package.
