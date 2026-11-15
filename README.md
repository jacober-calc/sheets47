# Sheets47

A standalone spreadsheet application planned for the SwissMicros **DMCP5**
platform: DM42n and DM32 hardware, including devices used with C47/R47.

The product is a **familiar graphical spreadsheet** that makes full use of the
DM32/DM42n display: a visible cell grid, column letters and row numbers, active
cell and range selection, formula editing, and scrolling. Users work directly
in the worksheet, with keypad input and six labelled softkeys along the bottom.

An existing open source engine will supply the calculation and cell semantics
underneath that interface. “Headless engine” describes an internal component;
the application itself has a graphical user interface. Chess47/Nav47 provide
native drawing and softkey references; familiar spreadsheet interaction defines
the worksheet experience. Engine selection must support that experience while
leaving memory for the interface and reliable workbook storage.

**Status: product design and feasibility research on the user-created `dev`
branch.** There is no runnable application, selected engine, or build target yet.

## Start here

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
| `build/`, `dist/` | Future ignored build and packaging output |

Brainbox remains an external, read-only reference collection at
`/home/jacober/Projects/Brainbox`. Builds must eventually use pinned dependencies
in this repository, without requiring that private local path. The foundation
commit contains inventory metadata and research notes rather than copied
firmware, complete application trees, or unselected engine sources.

## License

Original project material retains the existing [MIT license](LICENSE).
Third-party material keeps its own license. Choosing a GPL engine or copying
GPL application code requires recording the resulting distribution terms before
shipping a combined application; this foundation does not select those terms.
