# Working on Sheets47

## Scope and branches

- Target standalone DMCP5 `.pg5` programs for DM42n and DM32. C47/R47 are
  alternative calculator applications, not an API that runs alongside Sheets47.
- The product must have a familiar graphical spreadsheet UI: a visible cell
  grid, row/column headings, selection, formula editing, ranges, and scrolling.
  Use the DM32/DM42n display as the main workspace. A headless engine is an
  internal calculation component; engine selection must serve the graphical UI.
- The foundation is on `main`. The user will create `dev`; do not create it or
  begin application implementation as part of the foundation task.
- Keep the portable spreadsheet backend behind an adapter. Keep DMCP headers,
  keypad codes, LCD access, and filesystem calls in the platform layer.
- Read `docs/research/engines.md` before selecting or importing an engine.
  Recommendations are provisional until cross-linking and memory measurements.

## Resources and dependencies

- Use `/home/jacober/Projects/Brainbox` as an authorized, read-only reference
  collection. Its inventory and findings are in `docs/research/brainbox.md`.
- Do not make builds depend on Brainbox or copy entire resource trees into Git.
  Import only selected dependency files, with exact upstream identity, licenses,
  notices, checksums, and any local changes recorded in `third_party/`.
- Use the official DMCP5 SDK as the future baseline. Brainbox's standalone
  `DMCP_SDK-master` is for the older platform.
- Do not blindly run reference Makefiles: build and failure paths can delete
  files, and some USB interfaces are preliminary or newer than local firmware.

## Filesystem and deletion

- Ordinary task-related reading, creation, edits, and command execution are
  authorized within `/home/jacober/Projects/github` and its repositories.
  Existing workspace authorizations also apply.
- Ask before accessing or modifying other locations unless already authorized
  by the user or required by applicable task instructions. Sandbox rules still
  apply. Brainbox reference access is explicitly authorized.
- **Always obtain explicit user approval before deleting any file or directory,
  anywhere.** This includes temporary files, generated output, diagnostics,
  caches, setup scripts, backups, and empty directories.
- Before deletion, identify the exact paths and reason and wait for the user's
  explicit response. General task authorization or automatic approval does not
  authorize deletion. Do not bundle deletion into another action.
- Inspect build scripts for cleanup side effects, including `make clean`, error
  recovery, installers, `git clean`, and recursive removal. Prefer a fresh output
  directory when possible. Never implement automatic cleanup without approval.

## Development

- This is an ARM/DMCP5 project. Do not use TI CEdev tools for DMCP5 builds.
- For any actual TI-84 Plus CE C/C++ work, the sole tool installation is
  `/home/jacober/Projects/CEdev`; source its `env.sh` in the command shell before
  building, generating resources, or testing. Reuse its configured ROM, library,
  and launcher paths. Keep CE tool updates inside that kit, use real tool
  directories, and do not create parallel installs or compatibility symlinks.
- Keep project sources and build output in this repository. Do not install
  toolchains or modify hardware firmware as an incidental setup step.
- Record the ARM compiler, SDK revision, interface requirement, target flags,
  program/section sizes, and measured heap use when builds become available.
- Test meaningful formula behavior, structural edits, allocation failure,
  persistence failure, and power/resume behavior. Do not claim hardware support
  from a host test or a successful compile alone.
