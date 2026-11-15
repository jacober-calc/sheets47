# Verification

Run `python3 -B tools/build.py host` from the repository root. The build runs
`host_tests.cpp` against the real Spreader adapter, application, renderer, and
storage implementation. Version 0.1.0 has 838 checks covering formulas,
references, copy/fill/row edits, limits, input, viewport drawing, persistence
corruption and write failure, and recovery after allocation failure. Expected
numeric results are specified independently of the engine.

Allocation sweeps exercise 72 successive C++ allocation points and 92 successive
C allocation points. The same suite runs with address, undefined-behavior, and
leak sanitizers using `python3 -B tools/build.py host-sanitize`. Instrumented
host C++ allocation peaks are partial measurements, not target heap figures.

`python3 -B tests/host_roundtrip.py` checks save, reopen, edit/recalculate, save,
and reopen again across three real host processes. Tests retain their output
under `build/`; no cleanup is performed. The host executable also provides a
REPL for the optional graphical viewer and reproducible interaction scripts.

See [build instructions](../docs/building.md) and the
[versioned validation record](../docs/validation-0.1.0.md) for commands and exact
results. Actual keypad layouts, LCD refresh, power/resume, auto-off, USB handoff,
total heap/stack use, and battery current still require hardware qualification
on both DM32 and DM42n. A successful cross-link does not establish these results.
