# Local integration

The project build compiles the core and shipped parser/lexer sources directly, avoiding upstream
CMake's dependency downloads, Catch2, wrappers, and parser regeneration.
`SPR_SINGLE_THREADED=1` selects single-threaded reference counting. Release
assertions are disabled with `NDEBUG`; application validation remains explicit.
Exceptions stay enabled because Spreader uses allocation exceptions. The target
disables RTTI; Spreader's checked dynamic casts are assertions, absent in this
release configuration. Tests use the same engine configuration.

Version 0.1.0 exposes a bounded subset of upstream functions through the adapter:
SUM, AVERAGE, MIN, MAX, COUNT, IF, ROUND, ROUNDUP, ROUNDDOWN. ABS and SQRT are not
implemented by this pinned engine and were found to produce #ERROR! during host
qualification; they are rejected at the adapter boundary and not offered in the
function picker. No alternate evaluator is substituted.

Local source changes (original hashes are retained in SNAPSHOT.json):

- `generated/formula-parser.h`: remove `YY_NOEXCEPT` from the parser stack
  constructor. Its vector allocation was observed to terminate the process
  at the seventh injected C++ allocation, preventing candidate rollback.
- `inc/spreader/cell.h`: remove `noexcept` from the three allocating cell factory
  methods. Their pool allocation can throw independently of value construction.
  Cell constructors/destructors that do not allocate retain their declarations.
- `src/scalar-date-functions.h`: permit the DMCP5/newlib port to compile the
  existing `time`/`localtime_r` branch. No Unix platform macro is spoofed.
  NOW/TODAY are still blocked by the version 0.1.0 adapter; this is a build
  adaptation, not a qualified date-function feature.

Do not regenerate the parser without carrying its allocation patch forward.

- `src/formula.cpp`: RAII-owned scanner input, scanner cleanup if construction
  throws, and no redundant buffer switch. `generated/formula-lexer.cpp`: throw
  `bad_alloc` instead of exiting on scanner fatal allocation failures, and free
  the not-yet-owned buffer state if its stack setup throws. The adapter catches
  the exception and retains the committed workbook. Parser regeneration must
  preserve these generated-file changes too.
- `inc/spreader/cell-grid.h`: under `SHEETS47_SMALL_GRID`, use three levels of
  4-by-8 pointers instead of the desktop 16-by-1024/256-by-2048 tiles. The native
  maximum remains above the application's A1:Z256 limit. This changes storage
  granularity, not cell/reference/recalculation algorithms. An initial host
  fault-sweep run showed a 22.5 MB C++ allocation peak with desktop tiles,
  making that configuration inappropriate for this target.
- `src/cell.cpp`: under the same option, allocate cell pools in pages of 32
  units instead of 1,024. Both host tests and device builds select the small
  configuration. Host memory observations are not target heap measurements.
- `inc/spreader/cell-grid.h`: the tile destructor now releases owned child
  pointers even when the populated count is zero. A failed child allocation can
  leave a partially built branch before its count propagates upward. The first
  Address/LeakSanitizer run detected two leaked 280-byte host tiles during the
  fault sweeps; its diagnostic is retained locally under `build/qualification/`.
