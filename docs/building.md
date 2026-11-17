# Building sheets47 0.1.0

All application source, selected dependencies, tools, tests, logs, and output
live in this repository. Brainbox is a read-only structural reference, never a
build input. No build command downloads dependencies, installs system software,
changes firmware, or runs cleanup. Host work uses the normal C++ compiler;
DMCP5 work uses the ARM compiler below. TI CEdev tools are unrelated.

## Host checks and graphical view

Requirements: Python 3 and a host compiler with C++20 support (tested with GCC
15.2). The engine and helper source closure is already in `third_party/`.

```sh
python3 -B tools/build.py host --jobs 2
python3 -B tools/host_viewer.py
```

The first command builds and runs the checks. The second opens an optional Tk
window showing the actual application's monochrome renderer and Spreader
backend. It needs Python's standard tkinter module and a graphical session.
It is a host interface, not a DMCP5 emulator. Its data is confined to
`build/host/SHEETS47/`; pixel output and test results are in `build/host/`.
Closing the viewer does not implicitly save a draft or delete files.

The host suite also compiles the real DMCP5 file/input/menu adapters against
the pinned SDK, redirecting firmware services to test stubs. These checks cover
protected writes and system-menu access after save failure; they do not emulate
the DM32 keyboard scanner or physical storage. See the
[0.1.0 repair notes](changes/0.1.0-storage-menu-fix.md).

An additional compiler instrumentation pass runs the same checks under
AddressSanitizer, UndefinedBehaviorSanitizer and LeakSanitizer:

```sh
python3 -B tools/build.py host-sanitize --jobs 2
```

Its outputs are separate in `build/host-sanitize/`. LeakSanitizer may need an
untraced execution environment; it cannot inspect threads under the managed
sandbox's tracing. Keep a failed diagnostic when investigating that distinction.
Compiler temporary directories are configured under each build directory.

`python3 -B tests/host_roundtrip.py` additionally checks Save/Open through the
actual host filesystem across three application processes. Its append-only
sample generations and logs stay under `build/host/`.

## Portable ARM compiler

SwissMicros' own DMCP5 SDK [README](https://github.com/swissmicros/DMCP5_SDK/blob/71a04c2bb2e694da99e62318c1dc1b2ee5b883b8/README)
directs developers to obtain the compiler from Arm. The SDK does not bundle one.
Spreader requires a newer C++ implementation than its GCC 10.3 example.
The approved local installation is Arm GNU Toolchain 14.3.rel1 for Linux x86_64,
target arm-none-eabi. Its directory is real, local, and ignored by Git.

```sh
mkdir -p .local/downloads .local/tools
curl -fL --max-time 1800 -C - https://developer.arm.com/-/media/Files/downloads/gnu/14.3.rel1/binrel/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz -o .local/downloads/arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi.tar.xz
python3 -B tools/setup_arm.py
```

The setup tool verifies Arm's published SHA-256 before unpacking. It refuses
an existing installation directory rather than overwriting or removing it.
The archive remains cached. No shell profile or global PATH changes are needed.
Other hosts can provide `ARM_CXX` and `ARM_CC` paths to suitable installed tools;
the tested packaging tools currently default to the pinned local toolchain.
Using another compiler requires a new validation record.

## Device build and artifacts

```sh
python3 -B tools/build.py device --jobs 2
```

Output: `build/device/sheets47.elf`, raw image, link map, compiler stack-use
reports, per-source logs, `compile_commands.json`, `build.json`, and the packaged
`dist/sheets47-0.1.0/sheets47.pg5`. Program identity is lowercase `sheets47`.
This build explicitly requires the **DM32** system keymap (`0x32334d44`, ASCII
`DM32`) in its program header, matching the native DM32 application. Packaging
rejects a wildcard or another keymap. A DM42n build would need its own appropriate
keymap requirement and validation; this artifact is prepared for the user's DM32.
The [16 November header verification](validation/2026-11-16-dm32-header.json)
records the binary hash and confirms only the keymap header word changed from
the storage/menu repair build, apart from the loader digest.
The ARM flags follow the SDK's Thumb/ARMv7E-M, hard-float, FPv4-SP-D16 ABI.
The project uses size optimization, section garbage collection, C++20,
single-threaded Spreader, exceptions, and release assertions. The derived
linker script retains the SDK's 8 KiB static RAM and 1 MiB flash declarations.
The build records actual requirements and must fit those regions.

An engine exception does not imply a transaction succeeded; see the adapter
and failure tests. A linked `.pg5` is not proof of device compatibility. Real
DM42n and DM32 keymap, memory, filesystem, and power trials are still required.
See [the user guide](user-guide.md) and [validation record](validation-0.1.0.md).

## Dependencies and transparency

`third_party/sources.lock.json` pins source revisions. Every imported snapshot
has checksums, upstream identity, original licenses, and integration/patch notes.
`tools/vendor_sources.py` can establish selected snapshots from cached archives;
it refuses to overwrite modified files. Re-importing a patched snapshot therefore
needs deliberate handling, not an automatic reset. The compiler's original
license bundle is in `third_party/arm-runtime/`.

Build output and toolchains are ignored, while scripts, patches, source, tests,
and human-readable evidence are shared. There is no `clean` target. Before
deleting any archive, intermediate, generated output, or other path, identify
the exact paths and obtain explicit approval under AGENTS.md.

After the three build/check modes and the roundtrip test pass, run
`python3 -B tools/export_screenshots.py` and `python3 -B tools/prepare_release.py`
to prepare the local review package, matching source archive, licenses, hashes,
and tracked validation record. This performs no commit, remote publication, or
hardware upload. `python3 -B tools/vendor_audit.py` verifies imported sources
without requiring the archive cache; `--record-patches` deliberately records
new local source changes against the original cached archives.
