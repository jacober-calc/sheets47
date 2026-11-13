# Project tools

Future tools belong here when needed for reproducible SDK/engine inspection,
host checks, memory reports, and `.pg5` packaging. No tool is implemented yet.

Keep generated headers and packaging output under `build/`. Use separate output
directories for configurations. Scripts must not silently delete intermediates
or existing output; the repository's explicit deletion-approval rule applies to
build and failure paths too.

Do not copy Chess47's packager without recording its source and license. Its
size-word and SHA-1 layout is useful reference material, but the official SDK
must be the packaging baseline.
