# Project tools

The current reproducible tool entry points are documented in
[building.md](../docs/building.md):

- `build.py`: offline host/device compilation, logs, tests, and packaging.
- `setup_arm.py`: checksum-verified, repository-local compiler unpacking.
- `vendor_sources.py`: selected source import with original fingerprints.
- `vendor_audit.py`: source fingerprint and recorded local patch verification.
- `package_pg5.py`: official SDK-compatible size word and SHA-1 packaging.
- `prepare_release.py`: validated executable, source archive, notices, and evidence.
- `host_viewer.py`: optional graphical view of the real portable application.
- `export_screenshots.py`: PNG exports from the actual renderer's test frames.
- `render_worksheet_concept.py`, `render_decision_report.py`: design/report assets.

Keep generated headers and packaging output under `build/`. Use separate output
directories for configurations. Scripts must not silently delete intermediates
or existing output; the repository's explicit deletion-approval rule applies to
build and failure paths too.

Do not copy Chess47's packager without recording its source and license. Its
size-word and SHA-1 layout is useful reference material, but the official SDK
must be the packaging baseline.
