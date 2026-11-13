# Source layout

No application source is included in the foundation stage. Introduce these
directories as implementation reaches them:

| Directory | Responsibility |
| --- | --- |
| `app/` | Application lifecycle and commands |
| `engine/` | Narrow adapter to the selected third-party spreadsheet engine |
| `ui/` | Worksheet viewport, editor, selection, menus, and text layout |
| `storage/` | Versioned workbook persistence and CSV/TSV exchange |
| `platform/dmcp5/` | SDK integration, LCD, input, heap, files, sleep, and power |
| `platform/host/` | Host harness using the same engine and application behavior |

Dependency source belongs under `third_party/`, with local changes documented
there. Keep vendor-specific cell structures out of UI code. Do not implement a
second formula parser or recalc engine in the UI layer.
