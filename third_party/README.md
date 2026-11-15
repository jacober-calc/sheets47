# Dependencies and attribution

The foundation commit contained no vendored dependency source. Version 0.1.0
now pins the selected Spreader, sys_string, intrusive_shared_ptr, and official
DMCP5 SDK subsets in `sources.lock.json`. Each has an UPSTREAM.md, original
file checksums, license text, and PATCHES.md where applicable. Compiler/runtime
notices are in `arm-runtime/`; executables stay ignored under `.local/tools/`.
The existing MIT license covers original Sheets47 material, not imported files.

Import the official DMCP5 SDK and the winning spreadsheet engine only when the
`dev` feasibility work identifies the required versions and source subset.
Prefer a small vendored snapshot for the embedded build: a checkout should be
usable without Brainbox or an unpinned configure-time download.

For each import, add an `UPSTREAM.md` with:

- Canonical source URL, release and full commit ID, or archive SHA-256.
- Exact imported paths, exclusions, generation steps, and dependency closure.
- Complete upstream license files, per-file notices, and binary attribution.
- Local modifications, their reason, and a reproducible patch or clear diff.
- Supported configuration, target compiler/flags, and validation evidence.

SDK material has BSD-3-Clause terms plus SwissMicros' NOMAS notice; preserve
both. Chess47 and Nav47 application code declares GPL-3.0-or-later. Nav47's
fonts and datasets have additional origins documented in its `NOTICE`. DB48X
includes an LGPLv3 license and other notices; audit individual files before
reuse. C47's local release is not a complete native-source distribution.

Engine findings: Spreader carries BSD-3-Clause terms and has helper dependencies
to audit; mtCellEdit source declares GPL-3.0-or-later; SC-IM's checked `LICENSE`
contains four BSD-style conditions, including advertising acknowledgement.
Record exact selected-file terms and the combined application's distribution
requirements before import/release. Do not assume every candidate can be mixed
with every reference application under the repository's MIT label.

Keep firmware, toolchains, calculator state, unselected source archives, and
complete Brainbox mirrors outside tracked dependency directories. The local
mtCellEdit research archive remains ignored under `.local/research/`.
