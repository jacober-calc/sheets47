# sheets47 0.1.0 notices

Original application, UI, tests, and tooling are covered by the repository's
MIT LICENSE. The source distribution also contains selected third-party code:

| Component | Copyright / terms | Exact notices |
| --- | --- | --- |
| Spreader | Eugene Gershnik; BSD-3-Clause | third_party/spreader/LICENSE |
| sys_string | Eugene Gershnik; BSD-3-Clause | third_party/sys_string/LICENSE |
| intrusive_shared_ptr | Eugene Gershnik; BSD-style terms as supplied | third_party/intrusive_shared_ptr/LICENSE.txt |
| DMCP5 SDK | SwissMicros; BSD-3-Clause and NOMAS notice | third_party/dmcp5/LICENSE |
| Selected GCC/libstdc++/libgcc/newlib runtime | Multiple upstream authors; supplied runtime exceptions and notices | third_party/arm-runtime/LICENSES.txt |

Original per-file notices remain in the source. Each selected snapshot has its
revision and original checksums in UPSTREAM.md and SNAPSHOT.json. Local source
changes are explained in PATCHES.md and supplied as LOCAL.patch, with before/after
checksums in LOCAL_CHANGES.json. See docs/building.md for reproducible builds.

Chess47 and Nav47 were inspected as visual/structural references. No code, font,
bitmap, firmware, or personal calculator data from those application collections
is part of sheets47's implementation. Their upstream locations are recorded in
docs/research/brainbox.md. The monochrome glyphs and graphical worksheet renderer
in src/ui/ are original project material.

SwissMicros, DM32, DM42n, C47/R47, and HP names identify their respective products
and owners. This independent application does not imply manufacturer endorsement
or support. The SDK's original NOMAS statement is preserved in its license.
