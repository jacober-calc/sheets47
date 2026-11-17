# sheets47 0.1.0 validation

**Historical initial-build record.** For subsequent repairs, see the
[storage/menu notes](changes/0.1.0-storage-menu-fix.md) and the
[16 November DM32-header verification](validation/2026-11-16-dm32-header.json).
The artifact size, hash, and test count below identify the original build.

Recorded 14 November 2026. This is the first implementation on `dev`.
**Status: host-tested and cross-linked; no calculator hardware test has been performed.**

## Artifact and platform

- Program: `sheets47`; metadata version `0.1.0`.
- Artifact: `dist/sheets47-0.1.0/sheets47.pg5`, 453,468 bytes.
- SHA-256: `1ac663588bf496ddb3df1414b2e5890cf624ee9922df0b3d2b08f50038ef2852`.
- DMCP5 interface requirement: 3.17, from the pinned official SDK 3.57.
- No QSPI payload, custom USB stack, serial console, or firmware update.
- ARM compiler: arm-none-eabi-g++ (Arm GNU Toolchain 14.3.Rel1 (Build arm-14.174)) 14.3.1 20250623.
- Host compiler: g++ (Ubuntu 15.2.0-16ubuntu1) 15.2.0.
- Exact dependency revisions, original checksums and seven Spreader/one helper
  source patches are recorded in `third_party/`; `tools/vendor_audit.py` passes.

## Link measurements

| Item | Bytes | What this proves |
| --- | ---: | --- |
| Flash image before loader digest | 453,448 / 1,048,576 | Fits the selected SDK program region |
| `.data` | 984 | Initialized application RAM section |
| `.bss` | 1,296 | Zero-initialized application RAM section |
| `.data + .bss` | 2,280 / 8,192 | Fits the selected static RAM region |
| Largest reported compiled function frame | 632 | Compiler report only; not total call-stack use |

The ordinary `size` summary classifies some flash-resident tables as “data”.
The table above uses actual section placement (`size -A` and the link map),
not that summary. Heap use, largest free target block, stack high-water, target
response time, and battery current/life have **not** been measured.
The INFO screen exposes current target free/largest heap for future trials.

## Host evidence

- 838 checks passed with the actual patched Spreader backend.
- 72 successive C++ allocation failure points and
  92 project `malloc`/`realloc` failure points
  passed for candidate cell edits, preserving old contents and continued use.
- AddressSanitizer, UndefinedBehaviorSanitizer and LeakSanitizer passed the
  same 838 checks. The sanitizer run required execution outside the
  sandbox because LeakSanitizer cannot inspect its threads under tracing.
- Formula results, dependency updates, invalid syntax, division errors, cycles,
  relative/absolute references, captured clipboard contents, insert/delete/fill,
  clustered/scattered 256-cell sheets, and forward/reverse 128-formula chains
  are exercised. Whole-row/column ranges and unsupported arrays are rejected.
- Save/load preserves formulas, formats and viewport; corrupt latest generations
  fall back. Partial data/header writes, unavailable storage, and a failed Save
  after recovery from a corrupt newer generation preserve a usable prior save.
- UI tests exercise commit/cancel, reference insertion into the original edited
  cell, letter palette, range mode, unchanged-frame zero updates, partial
  navigation updates, and forced redraw after simulated resume.
- A separate three-process test uses the real host filesystem: save the sample
  total 18.50, reopen and edit quantity B2 to obtain 23.50, save again, then
  reopen in another process and recover 23.50. This verifies the host FilePort,
  not the device filesystem.
- The complete normal host suite took 17.68 ms in this run;
  the sanitizer run took 192.35 ms. These are test-suite times,
  not calculator response measurements or benchmark guarantees.
- Instrumented host C++ peak live allocation: 216,462
  bytes across the suite. This excludes direct libc allocations, renderer memory
  allocated on the host test stack, runtime overhead and target ABI differences.
  It must not be presented as total calculator heap use.

## Integration findings and remedies

The upstream desktop tile configuration produced a roughly 22.5 MB host C++
peak during early failure testing. Three smaller tile levels and 32-unit pools
now support the bounded worksheet without changing its calculation algorithms.
The generated parser stack and allocating cell factories incorrectly declared
no-throw paths; those declarations were corrected. Scanner allocation failure
now propagates with owned-buffer cleanup. The first leak check found two
partially constructed tiles retained after failure; unconditional pointer
cleanup in the tile destructor fixed them. The earlier diagnostic is retained
locally in `build/qualification/asan-before-tile-cleanup-fix.log`.

ABS/SQRT are absent from this Spreader revision and are not offered. The nine
exposed functions are tested. All changes and before/after fingerprints are
reviewable in the dependency patch files; original license notices remain.

## Hardware work still required

Use public sample workbooks with a recoverable calculator backup. Record each
DM42n/DM32 model, firmware/interface, keymap/overlay, artifact hash, free/largest
heap, actual stack high-water, load/edit/save timing, and observed outcomes.
Verify every input/softkey, screen row updates, automatic and explicit off,
draft/committed-state resume, USB disk handoff, full-storage/failed-close behavior,
and controlled save interruption. The host suspend/resume tests cover the
portable state model, not the firmware power state machine. The optional Tk
viewer has not been manually exercised in a graphical desktop session here.

No physical calculator was flashed or modified. No compatibility or durability
claim follows merely from this successful link. Binary, source, notices and
build evidence are prepared locally for review rather than published remotely.

See [the user guide](user-guide.md), [build instructions](building.md),
[actual application images](screenshots/README.md), and
[machine-readable evidence](validation/0.1.0.json).
