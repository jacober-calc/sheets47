# Brainbox resource study

Reviewed 2026-11-12 at `/home/jacober/Projects/Brainbox`. The user's shorthand
`/Projects/Brainbox` resolves to this collection in the current environment.
Brainbox was read without modifying its files.

All top-level resources were inventoried. Source review concentrated on build
and link configuration, SDK headers/syscalls, application lifecycle and input,
graphics, allocation, persistence examples, licenses, and project documentation.
Screenshots from Chess47 and Nav47 were inspected. This was not a line-by-line
audit of every engine, data table, test, or binary asset.

[brainbox-manifest.json](brainbox-manifest.json) records file counts, byte totals,
whole-tree fingerprints, and selected file SHA-256 hashes. The extracted folders
do not establish exact upstream commit IDs; do not invent revisions from their
names or timestamps. Whole-tree hashes identify these local snapshots, not an
authenticated upstream release.

## Collection

Counts include hidden regular files, exclude `.git`, and do not include empty
directories. Byte totals and hashes are in the manifest.

| Resource | Files | Relevance | Repository decision |
| --- | ---: | --- | --- |
| `chess47.pg5-main/` | 95 | Native DMCP5 shell, portable C engine separation, menu/power handling, packager, host simulator | Keep local; reference architecture; import specific files only with provenance |
| `nav47.pg5-main/` | 117 | Almanac-derived rich tables/graphics, six-key pages, fonts, persistence and lifecycle | Keep local; primary graphical interface reference |
| `DMCP_SDK-master/` | 22 | Original DMCP SDK, startup, syscalls, keymap helpers | Keep local; use official **DMCP5** SDK for the new target |
| `DM42-native-programs-main/` | 144 | Life, Ackermann, ZMachine, Anim, DiskTest, USBTest, guide-generation scripts | Keep local; selected input/storage/rendering patterns only |
| `db48x-0.10.0/` | 1,788 | Large calculator runtime, UI, files, memory management and DM32 build support | Keep local; avoid importing a second calculator runtime |
| `c47-dmcp5-00.109.05.00b2/` | 261 | C47 release, installation notes, programs, states, images, packaged resources | Keep local; hardware/workflow reference, not native engine source |
| Loose firmware/QSPI/keymap files | 5 | Recovery/platform resources and keyboard mapping | Keep local; never incidental application dependencies |

The loose files are `DMCP5_flash_3.55.bin`,
`DMCP5_flash_3.55_DM42-3.26.bin`, `DMCP5_flash_3.56_DM32-2.11.bin`,
`DM42_qspi_3.x.bin`, and `keymap_dm32.bin`.

## Native DMCP5 applications

Chess47 separates its portable engine (`src/engine.c`) from `src/main.c`, and
has a fake DMCP host layer under `test/sim/`. This is a useful pattern for
testing the same calculation and interaction behavior away from hardware.

Its `dmcp/` snapshot defines a 400 × 240 LCD and 32-pixel menu area. The linker
script gives `.data`/`.bss` an 8 KiB application region. Larger engine/game state
is heap allocated, and its info screen reports total and largest free heap.
The new spreadsheet must follow the memory distinction rather than placing
worksheet arrays in globals.

Nav47's README identifies its Almanac, split view, sky, and Moon pages as derived
from [Almanac47](https://github.com/valdesvj/almanac47). No separate native
Almanac47 source tree was present; the C47 release's `PROGRAMS/ALMA47.p47` is a
calculator-language program, not the native DMCP5 application. Nav47 provides
the available native Almanac-derived reference here.

Useful Nav47 paths:

- `src/main.c`: paged softkeys, edit modes, config/route persistence, power and
  sleep loop, USB power-change handling.
- `src/gfx.*`, `src/ntext.*`, `src/c47scr.*`, `src/views.*`: clipped graphics,
  compact text, C47-style screen organization, tables and multiple views.
- `src/fonts*.c` and `NOTICE`: font provenance; do not assume bitmap fonts are
  unlicensed reusable data.
- `test/sim/` and `tools/dm42view.py`: host display/testing approach.
- `tools/pg5tool.py`: program size-word/SHA-1 packaging reference.

The normal worksheet should retain the six-key bar and use the central display
for a scrolling grid and an explicit selected-cell editor. There is no reason
to import chess, astronomical, tide, time-zone, or navigation datasets.

**Version caveat:** these applications' bundled headers identify DMCP5 3.57 /
interface 3.17. Their default serial-console builds explicitly request interface
3.18 and describe DMCP5 3.58 as preliminary. The loose firmware images are
3.55/3.56. Their Makefiles also have TinyUSB alternatives. These local versions
do not prove compatibility with each other or with the user's installed device.
Start from official SDK functionality without adding a custom USB stack; check
the actual firmware/interface requirement before enabling console features.

**Build caveat:** reference Makefiles contain deletions in `clean` and other
paths. Nav47's normal private-data generation removes an intermediate, and the
QSPI rebuild path removes objects/ELF output. Reading these recipes is safe;
running them unmodified would violate the user's deletion-approval requirement.
None was run during this study.

## Older DM42 native examples

These programs target original DMCP `.pgm` builds, not DMCP5 `.pg5` builds.
Their measured timing and USB limitations must not be transferred to DM42n or
DM32 as if they were target measurements.

| Example | Useful lesson |
| --- | --- |
| Life | Compact monochrome rendering and separation of a compute kernel |
| Ackermann | Graphical feedback and bounded computation modes |
| ZMachine | Portable C interpreter plus calculator frontend; keypad alpha entry and file selection |
| Anim | LCD buffer packing/orientation and streaming; do not infer direct buffer layout without checking the target SDK |
| DiskTest | Measure filesystem buffering and power-source differences on real hardware |
| USBTest | API limitations must be verified against a specific SDK/firmware version |
| Guide | Scripts for a draft programming guide; the final document is absent |

The collection's project code is MIT; bundled SDK files retain their own terms.

## DB48X

The tree includes source for the parser, numerical types, runtime, menus,
graphics, files, keymaps, and simulator/tests. `src/dmcp/main.cc` obtains a large
heap area at startup; `runtime.cc` manages calculator objects and garbage
collection. These are useful lifecycle/memory references, but adopting that
runtime would be a second substantial engine integration and does not supply a
drop-in spreadsheet model.

The `.gitmodules` file names `dmcp`, `dmcp5`, `recorder`, and `emsdk`; all four
local directories are empty. This extracted archive is not a complete build
checkout. Do not copy it into Sheets42 and assume it is reproducible. Its root
`LICENSE.txt` is LGPLv3, with additional notices in the tree; inspect exact
selected-file terms before reuse.

## C47 bundle and firmware

The C47 folder contains `C47.pg5`, state files, calculator-language examples,
off images, a map archive, tests, keymap/QSPI resources, and installation notes.
It does not contain a native C/C++ source tree. `PACKAGES.txt` distinguishes the
full DMCP5 release from smaller original-DM42 builds, reinforcing that original
DM42 compatibility is a separate target decision.

Keep this complete release and all recovery images local. They are useful for
understanding how standalone applications are loaded and normal calculator
operation is restored, but they are not Sheets42 source dependencies. Do not
copy installation instructions that format a disk, replace QSPI, or override
loader warnings into the new application's routine setup.

## What enters Git now and later

**Now:** the resource inventory, selected source fingerprints, design findings,
engine study, and repository policies. No external code or screenshots copied.

**Later:** a pinned official DMCP5 SDK subset, the selected engine and required
helper sources, complete licenses/notices, small original examples, and measured
qualification results. Keep dependency imports reviewable and independent of the
Brainbox directory.

Primary platform references:

- [Official DMCP5 SDK](https://github.com/swissmicros/DMCP5_SDK)
- [Official SDK5 demo](https://github.com/swissmicros/SDK5demo)
- [DMCP development manual](https://technical.swissmicros.com/dmcp/devel/dmcp_devel_manual/)
- [Chess47 upstream](https://github.com/valdesvj/chess47.pg5)
- [Nav47 upstream](https://github.com/valdesvj/nav47.pg5)

Local snapshot fingerprints remain the evidence for files actually inspected
here; upstream branches can change independently of these extracted folders.
