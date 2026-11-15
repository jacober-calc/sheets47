# Development workflow

## Current stage: version 0.1.0

The user created `dev` and authorized a basic functional application with
Spreader, the agreed graphical worksheet, `/SHEETS47` persistence, minimal LCD
refresh, and battery-conscious idle behavior. See [building](building.md),
[the user guide](user-guide.md), and [validation](validation-0.1.0.md).
The foundation handoff below records the earlier scope, which is now complete.

## Foundation handoff

The repository already had commit `e89e3b0` with a short README and MIT license
when this study began. Preserve that history and add the repository foundation
as a new commit on `main`. The user will create `dev` next. No application
implementation, engine import, hardware update, or branch creation is part of
this foundation. The requested local commit does not imply a remote push.

The foundation contains documentation, directory responsibilities, ignore and
line-ending rules, and Brainbox inventory metadata. No application build or
runtime test exists yet. Check documentation links, manifest validity, and the
staged diff before committing.

## Local resources versus tracked inputs

| Resource | Location/policy |
| --- | --- |
| Brainbox collection | External read-only `/home/jacober/Projects/Brainbox` |
| Inventories, measurements, source links, design decisions | Tracked `docs/` |
| Selected SDK and engine source with licenses | Future tracked `third_party/` |
| Unselected archives and scratch research | Ignored `.local/research/` |
| Toolchain executables and firmware recovery files | Local resources; never commit incidentally |
| Build objects, generated headers, link maps, packages | Ignored `build/` and `dist/` |
| Public examples and required test fixtures | Tracked `examples/` and `tests/` |
| Calculator disk backups and personal workbooks | Local only; keep out of fixtures and releases |

Do not introduce symlinks to Brainbox or absolute build paths into tracked
configuration. A fresh clone must eventually build from pinned inputs. Keep
dependency downloads inside the project while evaluating; do not modify
Brainbox. Do not run cleanup, installation, or reference build targets before
checking them for file deletion.

## First work on dev: define the graphical worksheet and qualify its engine

The required product is a familiar graphical spreadsheet. Begin with reviewable
screen and interaction designs for the cell grid, active cell, formula editor,
scrolling, and range selection. These designs establish what an engine must
enable. Any initial visual prototype should state which behavior is simulated;
it cannot establish formula correctness or hardware compatibility.

Engine feasibility proceeds alongside this interface work. A minimal target
link is still required before extensive implementation or engine commitment,
with an explicit memory reserve for the graphical worksheet and editor.

1. Pin the official DMCP5 SDK and an ARM compiler already available to the
   developer. Inspect version commands and build recipes without installing
   another toolchain. Record all versions and required interface levels.
2. Start with **Spreader** for its complete headless API, using a single-threaded
   configuration and an explicit pinned helper dependency set. Exercise formulas
   and structural edits on a host harness. See the [engine study](research/engines.md).
3. Cross-link a minimal DMCP5 harness before extensive worksheet implementation. Measure
   empty-engine cost, section sizes, stack, peak heap, sparse/dense cell cost,
   formula temporaries, and failed allocations. Verify C++ runtime symbols and
   recovery behavior under the SDK allocation model.
4. If the runtime or footprint requires substantial engine changes, evaluate a
   deliberately reduced **mtCellEdit** core with the same cases. Qualify its
   recalculation ordering and reference notation explicitly. Do not substitute a
   scalar expression parser and then quietly rebuild spreadsheet semantics.
5. Record the winner, exact source closure, supported functions, limits, license
   notices, patches, and measured tradeoffs in an architecture decision. Import
   only the chosen dependencies after this comparison.

Stop an unsuitable candidate early: if it cannot fit the SDK's static region,
needs major runtime emulation, corrupts a workbook after allocation failure, or
requires a new dependency scheduler to be useful, it does not meet the intended
low-maintenance reuse goal. No candidate has passed these checks yet.

## Later milestones

| Stage | Reviewable result |
| --- | --- |
| Graphical worksheet design | Familiar cell grid, editor, scrolling, range selection, and keypad/softkey flows |
| Engine qualification | Portable tests plus a cross-linked and measured DMCP5 harness |
| Calculator shell | Grid, cursor, editor, softkeys, setup access, sleep/resume |
| Workbook persistence | Versioned save/load, failed-write recovery, CSV/TSV exchange |
| Useful worksheet | Range editing, reference picking, functions, formats, bounded undo |
| Hardware qualification | DM42n and DM32 test records for keymaps, battery/USB behavior, memory limits |
| Release | Reproducible `.pg5`, notices, install/restore guide, supported firmware matrix |

Original DM42, Excel/ODS file parsing on-device, charts, macros, networking,
custom USB stacks, and direct calculator-register interchange are later scope
decisions. They should not delay the engine and worksheet foundation.

## Hardware records to collect

For each calculator: hardware model, installed DMCP5 version, system keymap and
overlay, loader/interface requirement, free/largest heap at startup, successful
package load, power-off/restart results, file behavior, and the calculator
program used for returning to normal operation. Test records must separate
observations from estimates. Do not flash or replace a user's calculator
program as part of an unattended check.
