# Source layout

Version 0.1.0 uses these portable components and alternative platform services:

| Directory | Responsibility |
| --- | --- |
| `app/` | Application lifecycle and commands |
| `engine/` | Narrow adapter to the selected third-party spreadsheet engine |
| `ui/` | Worksheet viewport, editor, selection, menus, and text layout |
| `storage/` | Versioned workbook persistence, checksums, and fallback generations |
| `platform/dmcp5/` | SDK integration, LCD, input, heap, files, sleep, and power |
| `platform/host/` | Host harness using the same engine and application behavior |

Dependency source belongs under `third_party/`, with local changes documented
there. Keep vendor-specific cell structures out of UI code. Do not implement a
second formula parser or recalc engine in the UI layer.

The adapter retains bounded source/format records for persistence and candidate
transactions. Spreader owns calculated values, dependency work, reference
rewriting, and recalculation. Candidate transactions temporarily need both old
and new calculation state; see the documented memory tradeoff in architecture.md.
