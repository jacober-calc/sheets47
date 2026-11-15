# Sheets47

A standalone graphical spreadsheet application for the SwissMicros **DMCP5**
platform: DM42n and DM32 hardware, including devices used with C47/R47.

The product is a **familiar graphical spreadsheet** that makes full use of the
DM32/DM42n display: a visible cell grid, column letters and row numbers, active
cell and range selection, formula editing, and scrolling. Users work directly
in the worksheet, with keypad input and six labelled softkeys along the bottom.

The pinned **Spreader** engine supplies calculation and cell semantics
underneath that interface. “Headless engine” describes an internal component;
the application itself has a graphical user interface. Chess47/Nav47 provide
native drawing and softkey references; familiar spreadsheet interaction defines
the worksheet experience. Engine selection must support that experience while
leaving memory for the interface and reliable workbook storage.

**Version 0.1.0 is in development on `dev`.** It includes a graphical worksheet,
cell/formula editor, reference/function pickers, copy/paste, fill down, row edits,
number formats, and versioned save/load under `/SHEETS47`. Device builds produce
`sheets47.pg5`; the host view runs the same commands, renderer, and engine.
Hardware qualification remains separate from host checks and cross-link results.

Use **FILE → DEMO** to explore the sample worksheet. See the
[implemented worksheet, range selection, and editor](docs/screenshots/README.md).

## Start here

- [Build, test, and run the host view](docs/building.md)
- [Version 0.1.0 user guide and limitations](docs/user-guide.md)
- [Version 0.1.0 validation evidence](docs/validation-0.1.0.md)
- [Screenshots of the implemented application](docs/screenshots/README.md)
- [Graphical worksheet concepts based on Chess47/Nav47 references](docs/design/README.md)
- [Detailed engine and application report, 12 November 2026](docs/reports/2026-11-12-engine-and-application-report.pdf)
  ([editable Markdown](docs/reports/2026-11-12-engine-and-application-report.md),
  [HTML](docs/reports/2026-11-12-engine-and-application-report.html))
- [Project architecture and interface direction](docs/architecture.md)
- [Spreadsheet engine study and recommendation](docs/research/engines.md)
- [Brainbox inventory and source findings](docs/research/brainbox.md)
- [Development workflow and staged implementation](docs/development.md)
- [Dependency and attribution policy](third_party/README.md)

## Repository layout

| Path | Purpose |
| --- | --- |
| `src/` | Application, engine adapter, UI, storage, and platform code |
| `tests/` | Portable engine, storage, and calculator behavior checks |
| `tools/` | Reproducible build, packaging, and inspection utilities |
| `examples/` | Small, public sample worksheets and fixtures |
| `third_party/` | Explicitly selected, pinned dependencies and their notices |
| `docs/` | Design, research, workflow, and reference fingerprints |
| `.local/` | Ignored local research downloads and experiments |
| `build/`, `dist/` | Ignored build logs, tests, previews, and packaged output |

Brainbox remains an external, read-only reference collection at
`/home/jacober/Projects/Brainbox`. Builds use pinned dependencies in this
repository without requiring that private local path. Implementation imports
only selected SDK/engine/helper sources with their licenses and documented
patches. Firmware and complete reference application trees remain local.

## License

Original project material retains the existing [MIT license](LICENSE).
Third-party material keeps its own license. Choosing a GPL engine or copying
GPL application code requires recording the resulting distribution terms before
shipping a combined application. Current imports use BSD-family engine/helper
licenses and the official SDK's notices; original UI code uses MIT. See
`third_party/` for the exact source and toolchain-runtime notices.
