# Verification plan

There are no application tests to run yet. The first engine feasibility stage
must exercise real engine behavior on the host, then link the same backend for
DMCP5. Preserve upstream tests where practical.

- Formulas: literals, strings, arithmetic, ranges, absolute/relative references,
  dependency chains in both directions, error propagation, circular references.
- Editing: range copy/move/fill, overlapping moves, row/column insertion and
  deletion, and the resulting rewritten references.
- Limits: sparse and dense sheets, long expressions, deep dependencies, memory
  exhaustion, and a usable workbook after an edit fails.
- Storage: formula/format round trips, truncated or invalid files, failed writes,
  recovery, and locale-independent numeric serialization.
- Platform: viewport clipping, softkeys, alpha input, repeat/cancel, power off,
  resume, auto-off, USB disk handoff, and DM32 versus DM42n key layouts.

Report target compiler/flags and peak heap, largest allocation, static RAM,
stack use, flash size, and battery/USB recalculation time separately. Numerical
expected results should be independent of the implementation under test.
