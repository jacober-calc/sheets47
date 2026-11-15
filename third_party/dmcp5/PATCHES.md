# Local integration

Imported SDK files are unchanged. `src/platform/dmcp5/sheets47.ld` is derived from
the pinned official `stm32_program.ld`, retaining its original notice, memory
regions, startup symbols, and program header layout. It permits the selected
toolchain's C/C++/math/compiler-support library sections instead of the SDK's
default `/DISCARD/` block. This enables Spreader's required runtime. See the
actual build map for what is retained.

The SDK startup and allocator/service bindings are compiled directly. Application
metadata is `sheets47`, version `0.1.0`; interface requirement is the pinned
SDK's 3.17. No custom USB stack, serial console, or QSPI payload is included.
The packaging implementation follows the official size-field + SHA-1 format
and independently verifies the resulting header and digest. Upstream Makefiles
and packaging scripts are not executed, including their error cleanup paths.
