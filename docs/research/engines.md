# Spreadsheet engine study

Reviewed 2026-11-12 using primary project sources and a local mtCellEdit archive.
This is a source-level feasibility study; no engine has been compiled, ported,
benchmarked, or proven to fit DMCP5.

## Recommendation

**Evaluate Spreader first, mtCellEdit second; keep SC-IM as a fallback.** Spreader
most directly matches the goal of reusing a complete spreadsheet engine behind
a new calculator UI. mtCellEdit is the more promising compact C-oriented source
base, but current source is not the old tiny pure-C engine and its recalculation
model needs careful qualification. SC-IM is a mature C spreadsheet application,
but extracting its backend is a broader port than its terminal appearance
suggests.

This order favors integration effort over popularity alone. A headless library
avoids transplanting terminal control, process management, or a desktop GUI.
There is no verified drop-in engine for this target yet. The first `dev` task
should compare actual memory and link requirements before extensive UI work.

## Shortlist

| Candidate | Reusable backend | Main DMCP5 issue | Position |
| --- | --- | --- | --- |
| Spreader | C++20 library: cell storage, formulas, automatic recalculation, copy/move, row/column edits | Runtime, allocation-failure behavior, heap and flash footprint; upstream labels it alpha | First feasibility candidate |
| mtCellEdit / libmtcelledit | C API, sparse row/column trees, formulas, workbook and editing facilities | C++ default parser, helper library, ordered recalculation passes, reference notation, desktop parts to exclude | Compact-core alternative |
| SC-IM | C spreadsheet with formulas, dependency graph, undo, structural edits | UI/configuration coupling, POSIX services, dense pointer-table overhead, extraction work | Established application fallback |
| Ixion | C++ formula interpreter, cell model, dependency tracking | C++20 plus Boost/mdds and a larger runtime surface | Architectural reference; lower priority |
| Gnumeric | Extensive mature spreadsheet functionality | Desktop library stack and a broad application extraction | Feature/reference tool; lower priority |
| HyperFormula | Headless TypeScript/JavaScript engine | Requires a JS runtime or a major language port | Useful desktop comparison; lower priority |

## Spreader

The [upstream README](https://github.com/gershnik/spreader/blob/master/README.md)
describes a portable headless C++20 library with automatic recalculation and
structural editing, explicitly still alpha. Its
[Sheet API](https://github.com/gershnik/spreader/blob/master/lib/code/inc/spreader/sheet.h)
provides formula/value setting, range copy/move, row/column insertion/deletion,
and recalculation suspension/resumption. These are exactly the operations that
would otherwise be easy to get subtly wrong in our own engine.

The [build](https://github.com/gershnik/spreader/blob/master/CMakeLists.txt)
offers `SPR_SINGLE_THREADED`. It can use shipped generated parser sources when
Flex/Bison are unavailable. The
[dependency file](https://github.com/gershnik/spreader/blob/master/cmake/dependencies.cmake)
fetches `sys_string` v2.12, `intrusive_shared_ptr` v1.4, and, for normal non-Python
builds, Catch2 v3.3.0. Do not describe the actual source build as dependency-free,
even where a wrapper's overview does. Pin required helper sources and keep the
host testing framework out of the target link.

The [cell grid](https://github.com/gershnik/spreader/blob/master/lib/code/inc/spreader/cell-grid.h)
uses hierarchical tiles and documents `std::bad_alloc` for allocation failure.
Measure the cost of scattered cells as well as a dense block; sparse storage
alone does not prove low overhead. Investigate C++ strings, containers,
coroutines, exception tables, operator `new`/`delete`, and parser allocation
under the DMCP SDK. Verify whether an interrupted or failed operation leaves
valid cells and dependencies before designing undo or recovery around it.

The [license](https://github.com/gershnik/spreader/blob/master/LICENSE) is
BSD-3-Clause. This is a convenient starting point for retaining the project's
existing original-code license, provided helper licenses and any later copied
GUI code are also accounted for. Persistence, UI, and calculator lifecycle
still belong to Sheets47. Do not promise Excel file support from formula support.

## mtCellEdit

The [current author page](https://www.marktyler.org/ced/) identifies a reusable
C/C++ library and GPL-3.0-or-later licensing. Its desktop frontend is Qt-based;
the native workbook format uses ZIP/TSV. Neither frontend nor compressed
workbook support needs to be part of an initial calculator engine port.

The [older project page](https://mtmintkit.sourceforge.net/mtcelledit.html)
describes a pure-C 0.4.0 core below 32 KiB and R1C1 notation. Those historical
claims are not footprint measurements or full specifications for current 4.0.

Inspected archive:

- [mtcelledit-4.0.tar.xz](https://www.marktyler.org/download/mtcelledit-4.0.tar.xz)
- SHA-256: `dee2f85798ab538e4e0b0d2c662f2dff5c85f8be3c9fe417c73354b7aadcf7d8`
- Local cache: `.local/research/mtcelledit-4.0.tar.xz` (ignored; not vendored).

Findings from `libmtcelledit/src/` and `libmtkit/src/`:

- `CedSheet.rows` holds an `mtTree` of populated rows, with another tree for
  cells. `CedCell` stores source text, a double result, type, and optional prefs.
  This is a credible sparse model to measure on 32-bit ARM.
- `ced_eval.cpp` implements the default recursive-descent parser using C++
  strings/containers and exceptions. A Bison grammar remains in
  `ced_eval_bison.y`; `ced_eval.h` has a commented `CED_USE_BISON_PARSER` switch.
  Its presence is a possible evaluation path, not proof of a supported pure-C
  configuration or identical behavior.
- `ced_sheet_recalculate()` scans formula/error cells in forward or reverse
  order, returning the number of changed results. This call is not a topological
  dependency scheduler. Check the complete workbook/caller convergence behavior
  for reversed chains, cycles, volatile functions, and edit errors before reuse.
- Headers include POSIX facilities; the default library Makefile collects both
  `ced_*` engine and `cui_*` rendering/UI objects. A target source list must
  explicitly exclude desktop parts and select the needed `mtkit_*` helpers.
- R1C1-based cell references deserve an early UX decision. Verify the actual
  selected parser before offering A1 formulas; a notation conversion must also
  preserve absolute/relative references after copy and structural edits.

Prefer a small source-selection/build adaptation over rewriting its evaluator.
If dependable dependency ordering or desired formula semantics require building
a substantial new engine around it, it no longer meets the reuse objective.

## SC-IM

The [project README](https://github.com/andmarti1424/sc-im/blob/main/Readme.md)
documents C-based spreadsheet editing, undo, sorting, file exchange, scripting,
and noninteractive use. This is the strongest established C application on the
shortlist, but noninteractive operation is not an embeddable backend contract.

The checked [Makefile](https://github.com/andmarti1424/sc-im/blob/main/src/Makefile)
has an ncurses build and optional desktop libraries. The
[interpreter](https://github.com/andmarti1424/sc-im/blob/main/src/interp.c)
includes UI, command, configuration, graph, undo, signal, regular-expression,
and POSIX headers. Disabling optional XLSX/Lua functionality does not by itself
isolate the engine. The
[cell model](https://github.com/andmarti1424/sc-im/blob/main/src/sc.h) uses a
row/column pointer table; estimate its actual allocation strategy before
assuming unused grid space is cheap.

The checked [LICENSE](https://github.com/andmarti1424/sc-im/blob/main/LICENSE)
contains four BSD-style conditions, including advertising acknowledgement.
Audit the selected files and provenance before combining this code with other
licensed application code. No SC-IM source has been imported.

## Other candidates and boundaries

[Ixion](https://github.com/kohei-us/ixion/blob/master/README.md) is a real
headless engine, with dependency tracking and multiple reference syntaxes. The
checked upstream README specifies C++20, Boost, mdds, and MPL-2.0 licensing.
It is a sound architectural comparison but a less direct fit for a constrained
first port than the two leading candidates.

[Gnumeric](https://github.com/GNOME/gnumeric) and LibreOffice Calc offer rich
desktop behavior, but their application/library footprint and extraction work
make them poor initial bets here. This is a portability inference, not a
measured impossibility. [HyperFormula](https://github.com/handsontable/hyperformula)
is headless but its TypeScript/JavaScript implementation introduces a runtime
that the native C/C++ candidates avoid.

File-format libraries and scalar expression parsers solve different problems.
An XLSX writer cannot supply formula recalculation, reference rewriting, cell
storage, or circular-reference handling. A small math parser would leave most
of the spreadsheet engine for this project to implement.

## Decision evidence required on dev

Use the same cases for the leading candidates: empty worksheet; 1,000 short
numeric/text cells; dense versus widely separated cells; 100 formula cells;
forward and backward chains; a large range reduction; copied relative and
absolute references; structural edits; cycles; malformed expressions; and
deliberate allocation failure. These are experiment sizes, not promised limits.

Record flash, `.data + .bss`, peak/steady heap, largest allocation, stack high
water, runtime dependencies, and recalculation time. Separate host observations,
target-link results, and real-device behavior. Choose the backend only after
it can fit with room for the UI and storage and preserve workbook integrity
when resources run out.
