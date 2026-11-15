# Sheets47: engine options and the calculator application

**Decision report for an engaged development manager · 12 November 2026**

## 01 · The decision in front of us

**The required product is a familiar graphical spreadsheet using the DM42n/DM32 display as its main workspace.** Users see cells, column letters, row numbers, a selected cell or range, and formula editing; they scroll, copy calculations, and save worksheets. The proposed deliverable is a standalone program named `SHEETS47.pg5`. A headless engine is an internal calculation component. Sheets47 itself has a graphical user interface, and engine selection must serve that experience.

**My recommendation is to qualify Spreader first, compare a reduced mtCellEdit core if necessary, and retain SC-IM as a more expensive fallback.** This is an order for investigation, not a final engine selection. The strongest reason to start with Spreader is its close fit to the required job: it already presents spreadsheet operations through a headless C++ interface. The most serious uncertainty is whether its runtime and memory demands can coexist with the DMCP5 environment. [Spreader overview](https://github.com/gershnik/spreader/blob/master/README.md)

The development objective is a familiar, dependable graphical worksheet operated through the keypad and softkeys. Engine reuse must leave resources for that interface. Buying more features through a larger engine can increase work if they bring desktop assumptions or difficult recovery. Conversely, choosing a tiny arithmetic parser would transfer spreadsheet correctness back to us: dependencies, copying, insertion, deletion, and persistence would still need substantial implementation.

Three decisions should remain separate:

| Decision | Recommended position now |
| --- | --- |
| Product direction | A familiar graphical cell-grid spreadsheet for DMCP5 hardware |
| Engine commitment | Conditional on formula, memory, runtime, and failure tests |
| First useful release | One worksheet at a time, focused editing, dependable saves |

The proposed application uses a selected engine behind a project-owned adapter. Sheets47 supplies the graphical worksheet, input modes, softkeys, file format, resource limits, and device lifecycle. DMCP5 supplies the platform services through the official SDK. C47/R47 are alternative calculator applications; they do not supply a callable spreadsheet or numerical service while Sheets47 is active. The saved [architecture](../architecture.md) establishes this application model.

**Evidence status:** the repository is now on `dev`; it still contains a foundation rather than an application. No candidate has been compiled or benchmarked for this report, and no hardware compatibility has been demonstrated. Source inspection supports the comparisons. Architecture, screen layouts, release scope, and acceptance criteria are proposals. Numerical capacities and performance remain unmeasured.

This report has twenty substantive pages followed by two pages of evidence references. Pages 2–10 explain the options and reuse obligations; pages 11–19 explain the complete device application and user experience; page 20 describes the proposed development decision process. Reading the comparison on page 3 alongside the qualification plan on page 20 gives the shortest route to a management decision.

<!-- pagebreak -->

## 02 · What an engine contributes—and what we still build

A spreadsheet engine gives meaning to the grid. It stores the contents of cells, interprets formulas, obtains values from referenced cells, and updates results when inputs change. A strong engine also understands how references change when cells are copied or rows are inserted. A library that evaluates `2+3` solves only a small part of this problem.

Consider a sheet in which `B2` contains a quantity, `C2` contains a price, and `D2` contains `=B2*C2`. If the quantity changes, the result must change. If `D2` is copied down, its references should normally become `B3` and `C3`. If a row is inserted, existing references must continue to identify the intended data according to the chosen engine's rules. These behaviors are part of the calculation model, not drawing operations.

**Dependency management** means remembering which formulas rely on which inputs. A chain such as `A1 → B1 → C1` must calculate in an order that produces current answers. Circular references need an explicit policy. Merely scanning every formula once can leave old values in some arrangements. Repeating scans can be costly and requires rules for termination and changing functions such as random-number generators.

**Headless** means that an internal component has no required user interface. The complete Sheets47 application must still provide a familiar graphical spreadsheet. Spreader and Ixion supply reusable backends; mtCellEdit exposes a core within a larger suite; SC-IM requires backend extraction. None of these options has a graphical interface already qualified for DMCP5. Reusing an engine therefore leaves real UI development work, potentially including selected reusable drawing or interaction code. [Spreader](https://github.com/gershnik/spreader/blob/master/README.md), [Ixion](https://github.com/kohei-us/ixion/blob/master/README.md), [mtCellEdit](https://www.marktyler.org/ced/), [SC-IM](https://github.com/andmarti1424/sc-im/blob/main/Readme.md)

Sheets47 would still own four major responsibilities. First, it must translate physical keys into understandable actions. Second, it must render the selected part of the sheet legibly. Third, it must preserve work through saves, interruptions, and resource failures. Fourth, it must impose limits appropriate to the device. None of these responsibilities disappears because a library supports formulas.

Free and open-source software lets us inspect, adapt, and redistribute components under their licenses. This reduces the need to invent established spreadsheet behavior and makes debugging possible without a vendor's private source. It does not promise target compatibility, ongoing support, or free engineering effort. We become responsible for the exact version we ship and for our integration changes.

The most valuable reuse is therefore **correct behavior with a manageable dependency boundary**. A library's advertised function count matters less than whether its useful operations survive limited memory, whether failures preserve a workbook, and whether we can maintain the port without becoming the primary developers of a new engine. That is the standard applied throughout this report.

<!-- pagebreak -->

## 03 · Comparative view of the options

This table compares suitability for Sheets47, not the general quality of the projects. “Lower priority” means that the expected integration burden is less attractive for this device. It does not mean that a target build has proved impossible. The underlying source review is documented in the saved [engine study](../research/engines.md) and the primary references on pages 21–22.

| Candidate | What we would reuse | Principal development gain | Principal development cost |
| --- | --- | --- | --- |
| Spreader | Headless C++20 sheet library | Direct cell, formula, copy/move, structural-edit API | C++ runtime, exceptions, allocation behavior; alpha status |
| mtCellEdit | C-facing spreadsheet core and selected helpers | Sparse storage and practical workbook operations | Core extraction, C++ parser, recalculation policy, R1C1 notation |
| SC-IM | Selected C calculation and editing subsystems | Established application behavior and dependency machinery | Untangling terminal/POSIX/global application assumptions |
| Ixion | Headless formula and document model | Explicit dependency tracking and several reference syntaxes | C++20 plus Boost/mdds; target source and runtime closure |
| Gnumeric | Selected mature spreadsheet facilities | Extensive numerical and spreadsheet reference behavior | Broad GNOME-oriented application and library dependencies |
| HyperFormula | TypeScript headless model | Rich documented formula and editing interface | JavaScript runtime or substantial rewrite on this target |
| LibreOffice Calc | Desktop comparison and possible host tools | Familiar interoperability and mature user behavior | Extracting an embedded native core is a major separate project |

The first two candidates represent different bets. Spreader bets that a well-shaped API will save enough integration effort to justify modern C++ support. mtCellEdit bets that a selectively built core can provide an economical target implementation, accepting more integration decisions. SC-IM bets that mature application functionality is worth a larger extraction task. Ixion deserves renewed consideration if its dependency closure proves unexpectedly tractable or its calculation model solves requirements the first candidates cannot.

There is no justified numeric ranking for flash use, cell capacity, or speed yet. Sparse storage is a design property, not a measured memory result. A C API is an interface property, not proof that its implementation avoids C++. A command-line program is a presentation choice, not proof that its backend is small.

The decision sequence should reflect these distinctions. First eliminate candidates that cannot link within the platform model or preserve workbook integrity under failure. Next compare their actual cost with the same representative worksheets. Only then weigh additional functions, licensing preferences, and convenience. A favorite should be rejected if keeping it requires substantial runtime emulation or rebuilding its dependency system.

From a management perspective, the main controllable cost is **the amount of upstream behavior we must replace**. Thin adaptation is attractive; a growing private fork is a recurring expense. An engine with fewer features but a stable, understandable integration can be a better product foundation than a richer engine that needs continuing surgery.

<!-- pagebreak -->

## 04 · Spreader: the strongest initial API fit

Spreader offers cell values and formulas, automatic recalculation, copying and moving ranges, and insertion and deletion of rows and columns. Its public `Sheet` interface also exposes formula text and row/column sizing information. These operations closely match the commands a calculator spreadsheet needs. The upstream overview identifies the project as alpha, so API completeness should not be confused with established release maturity. [Overview](https://github.com/gershnik/spreader/blob/master/README.md), [Sheet interface](https://github.com/gershnik/spreader/blob/master/lib/code/inc/spreader/sheet.h)

**What we gain.** We could connect a keypad command such as “copy this range” to an existing spreadsheet operation rather than reinterpret formulas ourselves. Recalculation can be suspended and resumed around a batch, which is useful during loading or filling a range. Its A1-oriented naming helpers fit the familiar column-letter and row-number presentation. The backend does not bring a terminal or desktop GUI that we would first have to remove.

Its cell storage uses a hierarchy of tiles. This can represent a large coordinate space without allocating a complete rectangular grid, although scattered cells may incur more overhead than clustered ones. The project exposes a single-threaded build option; we should explicitly select it rather than accept the general desktop default. The checked build fetches `sys_string` and `intrusive_shared_ptr`, and its ordinary test configuration also brings Catch2. These dependencies need separate attribution and build control. [Grid](https://github.com/gershnik/spreader/blob/master/lib/code/inc/spreader/cell-grid.h), [configuration](https://github.com/gershnik/spreader/blob/master/CMakeLists.txt), [dependencies](https://github.com/gershnik/spreader/blob/master/cmake/dependencies.cmake)

**What we pay.** C++20 includes language and library facilities that an embedded compiler may accept syntactically while still needing runtime support. Strings, containers, coroutines, allocation, and exceptions can add code and working memory. The official DMCP5 example build disables exceptions and discards sections from normal C, math, and compiler-support libraries. This is a concrete integration conflict to investigate, not proof that C++ is unsuitable. [SDK build](https://github.com/swissmicros/DMCP5_SDK/blob/master/Makefile), [linker script](https://github.com/swissmicros/DMCP5_SDK/blob/master/stm32_program.ld)

The grid documents allocation exceptions. Catching one at the adapter boundary is insufficient evidence that a partly completed range edit leaves all cells and dependencies consistent. We need failure injection during single-cell edits, copying, insertion, and recalculation. We must also establish a way to enumerate stored cells efficiently for saving; a convenient rendering API alone does not establish a practical persistence interface.

**Licensing and maintenance.** Its BSD-3-Clause license offers relatively straightforward redistribution with preserved notices, subject to helper dependencies and any other imported code. That simplicity is useful but does not offset a poor target fit. [License](https://github.com/gershnik/spreader/blob/master/LICENSE)

**Decision trigger:** continue if a minimal target harness links with modest adaptation, leaves useful room for the rest of the application, and survives resource failures. Stop if exception/runtime support becomes a substantial new platform project or workbook operations cannot fail safely. This is the candidate I would investigate first, without promising that it will win.

<!-- pagebreak -->

## 05 · mtCellEdit: a plausible compact-core alternative

mtCellEdit is a small spreadsheet application with a reusable C/C++ library. Its desktop interface uses Qt, and its native format is a ZIP archive containing TSV text. The author's page reports testing on ARM32 systems running GNU/Linux. That is encouraging portability evidence, but GNU/Linux provides services that DMCP5 does not. It is not evidence of a bare-device port. [Author's overview](https://www.marktyler.org/ced/)

**What we gain.** The inspected 4.0 archive stores populated rows in a tree and populated cells in a tree within each row. A cell holds source text, a double result, a type, and optional preferences. This gives a concrete sparse model and a C-facing interface to investigate. Its core already provides spreadsheet and workbook operations; the proposal is to select those facilities and required `mtkit` helpers while leaving the desktop UI out.

**What we pay.** The current default evaluator is C++, uses standard strings and containers, and uses exceptions in ordinary parsing/error handling. A Bison alternative exists behind a switch, but we have not demonstrated that it is a supported drop-in pure-C configuration with equivalent behavior. The older claim that an early pure-C engine occupied less than 32 KiB must not be applied to version 4.0. These are findings from the fingerprinted local source archive, described in the [engine study](../research/engines.md).

Recalculation is the central semantic question. In the inspected source, `ced_sheet_recalculate()` scans eligible cells forward or backward and counts changed numerical values. The inspected `ced_book_recalculate()` delegates a pass to each sheet. Those functions do not themselves establish a general dependency-ordered calculation. Additional calling code and observable behavior must be evaluated before concluding how many passes a complete operation performs.

The management risk is that a simple apparent port could expand into ownership of a calculation scheduler. A dependency chain arranged against scan order is an essential test. So are cycles, error-state changes, and volatile functions. A changing random result cannot be treated as evidence that ordinary convergence is merely taking another pass. If a dependable result requires us to construct substantial new machinery, this option loses much of its reuse advantage.

The native parser uses R1C1 references, including relative forms. This can be a coherent product choice, but users expecting `A1` formulas would need either a different presentation or a thoroughly tested translation layer. Replacing text strings is inadequate: copying, absolute references, insertion, deletion, and saved formulas must remain consistent.

**Licensing.** The source is GPL-3.0-or-later. A distributed combined program using this core should be planned around GPL-compatible whole-program distribution and corresponding source. This is manageable for an open-source project, but it changes release obligations from a purely MIT/BSD approach. See page 10.

**Decision trigger:** choose mtCellEdit if the selected core is materially easier to fit and its actual calculation and reference behavior are acceptable without a major rewrite. The examined archive's SHA-256 is recorded on page 22; no source from it is vendored or built by this report.

<!-- pagebreak -->

## 06 · SC-IM: mature application behavior, larger extraction task

SC-IM is a C spreadsheet application built around an ncurses terminal interface and Vim-style interaction. Its documented features include undo/redo, range operations, structural editing, sorting, filtering, and several import/export options. It also supports noninteractive operation. These are useful signs of a substantial spreadsheet system, but the features belong to the desktop application as configured, not automatically to a small reusable target library. [Project overview](https://github.com/andmarti1424/sc-im/blob/main/Readme.md)

**What we gain.** The project already contains calculation, dependency, expression, and editing machinery that has been exercised as part of a usable application. C may make the target runtime easier to understand than a large C++ configuration. Its behavior is worth studying when designing range editing and error presentation. If extraction is manageable, we could reuse more than a parser.

**What we pay.** Calculation source includes application configuration, terminal UI, commands, graph, undo, signals, regular expressions, and other host-oriented facilities. Turning off optional libraries removes some dependencies but does not create a clean embedded interface. Noninteractive execution still runs an application with its own environment; it does not mean that a caller can safely instantiate a small independent workbook object. [Interpreter source](https://github.com/andmarti1424/sc-im/blob/main/src/interp.c)

The cell model contains a row/column pointer matrix. Empty positions may therefore consume pointer-table space even if cell objects are allocated selectively. Actual growth policy needs inspection and measurement. It would be misleading to multiply the application's advertised maximum dimensions and call that the target footprint, but equally misleading to assume blank cells cost nothing. [Cell structures](https://github.com/andmarti1424/sc-im/blob/main/src/sc.h)

Host conveniences create additional questions. External functions, Lua scripting, process interaction, desktop clipboard tools, and rich file converters are not part of the initial calculator product. They may be optional upstream, yet shared command paths can still assume their surrounding infrastructure. Error handlers must also be checked for process termination or global recovery techniques that would be inappropriate inside a recoverable worksheet session.

**Licensing requires special attention.** The checked root license includes four BSD-style conditions, including an advertising acknowledgement. It should not be abbreviated to “ordinary BSD” and then mixed casually with GPL application code. The exact selected-file obligations and compatibility need resolution before integration. A permissive-sounding label is not a substitute for reading the text. [Checked license](https://github.com/andmarti1424/sc-im/blob/main/LICENSE)

From a development perspective, the risk is a permanent extraction fork: changes upstream may presume the original application structure, while our build compiles a carefully selected subset. Every upstream update could require repeating that work. C source can be approachable while the overall integration remains complicated.

**Decision trigger:** pursue SC-IM if the leading libraries fail and a focused extraction study shows a maintainable calculation boundary. Keep its mature behavior as a reference in the meantime. Selecting it merely because it is written in C or appears visually simple in a terminal would underestimate the project.

<!-- pagebreak -->

## 07 · Ixion: a serious backend with a broader dependency question

Ixion is a headless formula parser, interpreter, dependency tracker, and spreadsheet document model. The current README describes a multi-sheet model with numeric, string, Boolean, and formula cells; dependency tracking supports full and partial recalculation. It can run with one thread or multiple threads. Unlike an application extraction, this is a deliberately reusable engine boundary. [Ixion overview](https://github.com/kohei-us/ixion/blob/master/README.md)

**What we gain.** Its explicit dependency model addresses one of the hardest parts of a spreadsheet. It supports several reference syntaxes, including Excel A1 and R1C1 and LibreOffice-related forms. Named expressions, ranges, volatile functions, formula grouping, and sheet-copy facilities are attractive if the product eventually grows. Using its own cell model would avoid inventing a second workbook store.

It also offers an instructive architectural comparison. Formula parsing, storage, dependency identification, and recalculation are exposed as distinct concerns. This helps us judge whether another candidate actually provides a complete backend or merely a convenient evaluator. Its native C++ interface is more naturally aligned with the proposed `.pg5` program than introducing a JavaScript runtime.

**What we pay.** The documented requirements include C++20, Boost, and mdds. These names do not establish a minimum binary size: some library facilities are templates or selectively linked. They do establish a larger dependency investigation. We would have to determine the precise source and runtime closure, configure single-threaded operation, inspect static initialization, and qualify allocation failure under the DMCP5 allocator.

The advertised backend scope also must not be confused with a complete user-facing spreadsheet. Its README explicitly lists missing features, including more built-in functions and caller-defined custom functions. The existence of a parser for several syntaxes does not prove Excel equivalence or establish every copy/insert operation Sheets47 would need. The chosen interface and version must be checked against our command contract.

**Licensing.** Ixion uses MPL-2.0. This generally keeps source obligations at the covered-file level while allowing a larger work under other terms, provided the MPL requirements are met. It offers a different balance from whole-program GPL licensing, but it still requires preserving notices and making covered source available when distributing executable forms. [Mozilla's MPL explanation](https://www.mozilla.org/en-US/MPL/2.0/FAQ/)

**Development judgment:** Ixion is a credible reserve option, not simply a theoretical reference. It ranks behind the two initial candidates because the available study does not establish a smaller or easier embedded configuration, while its dependency set is visibly broader. That is an engineering inference, not a measured result.

**Decision trigger:** reconsider it if Spreader's semantics or API prove unsuitable and Ixion's minimal single-threaded model can be demonstrated with limited platform work. A successful small target link could change the ranking quickly. We should not rule it out solely on the words “Boost” or “C++20,” just as we should not select a C candidate solely on language.

<!-- pagebreak -->

## 08 · Gnumeric and LibreOffice Calc: valuable desktop references

Gnumeric and LibreOffice Calc are full spreadsheet products. They offer a much broader desktop experience than the first calculator release needs. Their value to Sheets47 is strongest as behavioral references and possible components of a future computer-side workflow. Reusing a complete desktop application is a different undertaking from embedding a library with a suitable calculation boundary.

**Gnumeric's offer.** It is a mature spreadsheet with substantial numerical and worksheet functionality. Its project materials emphasize calculation quality. The checked build requirements include GLib, GTK, libgsf, goffice, libxml, and Pango. Some of that supports desktop presentation, but removing the visible windows alone does not demonstrate that the remaining calculation facilities can be isolated economically. [Project site](https://www.gnumeric.org/), [build requirements](https://github.com/GNOME/gnumeric/blob/master/README)

The development gain would be access to well-developed numerical behavior and spreadsheet experience. The drawback is source and dependency scope: we might spend more time defining and maintaining an embedded subset than on the calculator application itself. No stripped target footprint has been measured. The full project is therefore a useful comparison standard and a lower-priority port candidate, rather than an engine already rejected by a size test.

**LibreOffice Calc's offer.** It provides a familiar desktop environment for formulas, formatting, document exchange, and checking small test workbooks. It could help a user prepare source data or examine exported results on a computer. These uses do not require its code inside Sheets47. The project's licensing is principally MPL-2.0 with numerous component-specific notices; importing selected code would require a selected-file and dependency review. [LibreOffice licensing](https://www.libreoffice.org/licenses/)

The main development drawback is the scale of extracting a native embedded backend from a large office suite. The current project has no demonstrated source closure for that route. Treating it as an easy path to Excel/ODS compatibility would conceal a major engineering effort. The saved [engine research](../research/engines.md) treats this as a portability inference rather than a measured impossibility.

**How to use these projects constructively.** Create small public worksheets with known expected results and compare ordinary arithmetic, rounding, reference copying, and structural edits. Where programs disagree, determine whether our supported semantics are deliberate. A desktop result is evidence about that desktop program; it is not a universal correctness oracle. Dates, text conversion, locale, rounding, and unsupported functions can legitimately differ.

File exchange should be a separate decision. CSV can move a simple table into or out of a desktop spreadsheet, but it does not preserve a full workbook's formulas, formatting, and calculation rules without additional conventions. A future desktop converter could handle complex documents while keeping the calculator lean. That converter would be its own supported tool, with explicit conversion losses.

**Decision trigger:** retain both as product and test references. Revisit embedded reuse only if a concrete, maintainable core is identified. They should not delay qualification of the candidates already shaped more closely to our target.

<!-- pagebreak -->

## 09 · HyperFormula: the right abstraction in the wrong runtime

HyperFormula is a headless spreadsheet implemented in TypeScript. It provides formula parsing/evaluation, a substantial function library, clipboard operations, undo/redo, named expressions, sorting, and structural operations through a documented API. It is intended for browsers and Node.js applications. Its documentation makes it a particularly useful example of what a reusable spreadsheet service can look like. [Project overview](https://github.com/handsontable/hyperformula/blob/master/README.md)

**What we gain.** From an application-design perspective, it separates the grid's presentation from its calculation model. That is the same broad separation Sheets47 needs. Its operation-oriented API and documentation can help us formulate a clear contract for our own adapter. On a web or desktop project, this could be a strong candidate with considerable ready-made functionality.

**What we pay on this hardware.** TypeScript normally produces JavaScript, which needs an execution environment. DMCP5 does not supply a browser or Node.js service as the proposed application foundation. We would have to bring a JavaScript runtime and its memory-management behavior, adapt library dependencies, or rewrite a substantial body of logic in native code. None of those paths has been measured or demonstrated here.

The inference is that this creates a second porting project before we solve the spreadsheet port. Garbage collection, language runtime support, and additional startup/working memory would have to be understood alongside the engine. Merely choosing a small JavaScript interpreter would not prove it supports the language and library features HyperFormula expects. A successful browser demonstration would tell us very little about that integration.

Rewriting it in C/C++ would retain conceptual ideas while surrendering much of the economic advantage of upstream reuse. We would become responsible for equivalence, regression tests, and ongoing translation of fixes. This conflicts with the objective of concentrating effort on the calculator interface and reliability.

**Licensing.** HyperFormula offers GPLv3 and proprietary licensing options. The open-source option is relevant to this project; a proprietary license is not required simply because software is used commercially. The actual chosen license terms govern redistribution. Buying a license would not solve the runtime mismatch. [License file](https://github.com/handsontable/hyperformula/blob/master/LICENSE.txt), [licensing guide](https://handsontable.com/docs/hyperformula/guide/licensing.html)

A future computer-side worksheet editor could potentially use HyperFormula, but this creates a semantic consistency issue: formulas might calculate differently on the computer and calculator. A native host build using the same selected engine as the device is a stronger basis for authoritative project tests. A browser editor could be useful without becoming the source of truth for calculation.

**Decision trigger:** keep HyperFormula as an API and user-behavior reference, and as a possible future companion-tool option. Do not make it the initial device engine unless there is compelling target evidence that overturns the runtime concern. Its lower ranking reflects platform fit, not a lack of spreadsheet capability.

<!-- pagebreak -->

## 10 · Open-source integration, licensing, and ongoing ownership

The planned reuse is incorporation of selected source components into the compiled application. The engine would ordinarily be linked with Sheets47 code into one executable image. It would not be downloaded at calculator startup or accessed as an internet service. The same engine could also be compiled into a host test program. This is the integration proposal, not an already completed build.

The repository's MIT license covers original material. It does not replace licenses on imported files. A release must describe the rights and obligations of the actual combination, including SDK files, helper libraries, adapted application code, and separately licensed assets. A thin adapter is useful engineering; it does not by itself remove license obligations from linked code. See the project's [dependency policy](../../third_party/README.md).

| Component choice | Practical planning consequence |
| --- | --- |
| Spreader, BSD-3-Clause | Preserve source and binary notices; audit helpers separately |
| mtCellEdit, GPL-3.0-or-later | Plan the combined application and corresponding source around GPL-compatible distribution |
| SC-IM, checked four-condition license | Preserve the additional acknowledgement; resolve compatibility before mixing code |
| Ixion, MPL-2.0 | Track covered files and modifications; provide covered source as required |
| HyperFormula, GPLv3 option | GPL obligations apply when choosing that route; proprietary terms are an alternative |
| DMCP5 SDK | Preserve BSD notices and the manufacturer's NOMAS statement |

For a GPL engine linked into the proposed single program, we should plan to publish the corresponding application source and the scripts needed to build it alongside a downloadable binary. GPLv3 sections 5 and 6 address combined works and non-source distribution. The inspected mtCellEdit archive includes that license text in `COPYING.txt`. Original MIT notices can remain on original files; describing the complete distributed binary as MIT-only would be misleading. User-created worksheets are not automatically relicensed merely because a GPL application edits them.

MPL has a different boundary: its obligations generally follow covered files, including modifications, within a larger work. BSD-style licenses primarily require notices and other stated conditions. SC-IM's additional advertising condition makes its exact text important, especially if GPL-derived UI code is later introduced. These are release-planning implications; exact compatibility depends on the selected source set. [MPL FAQ](https://www.mozilla.org/en-US/MPL/2.0/FAQ/), [Spreader license](https://github.com/gershnik/spreader/blob/master/LICENSE), [SC-IM license](https://github.com/andmarti1424/sc-im/blob/main/LICENSE), [SDK license](https://github.com/swissmicros/DMCP5_SDK/blob/master/LICENSE)

For each import, record a full upstream revision or archive checksum, imported paths, licenses, excluded features, local patches, configuration, and validation. Keep the patch set small enough to explain. Pinning makes releases reproducible, but it also makes us responsible for monitoring relevant upstream fixes rather than silently following a moving branch.

The management gain is independent access to source and the ability to maintain the product. The recurring cost is integration stewardship: reviewing updates, preserving notices, testing regressions, and documenting limitations. None of the licenses promises that upstream developers will support this calculator port. The chosen release should make ownership of those tasks explicit.

<!-- pagebreak -->

## 11 · The hardware and platform model

DM42n and DM32 are the initial hardware targets. DMCP5 is the resident platform that provides services used by standalone programs. Sheets47 would run as the active calculator application, with its own worksheet and interaction model. The ordinary C47/R47 program would have to be loaded again to return to that environment. Its registers, calculation engine, and private data are not assumed to be live services available to Sheets47.

The official SDK header describes a 400 × 240 monochrome display and a 32-pixel menu area. Six contextual softkeys are the proposed primary command interface. A monochrome display favors a clear grid, inverse selection, readable fonts, and simple indicators rather than desktop-style ribbons and dense icons. [DMCP5 header](https://github.com/swissmicros/DMCP5_SDK/blob/master/dmcp/dmcp.h)

The checked official linker script declares these application regions:

| Region | Declared location | Declared capacity | Meaning for Sheets47 |
| --- | --- | --- | --- |
| Program flash | `0x080a0000` | 1,024 KiB | Executable code and constants must fit the selected image layout |
| Static RAM | `0x28000000` | 8 KiB | Large worksheet state cannot simply be global arrays |
| QSPI region | `0x90000000` | 2,048 KiB | Separate resource mapping, not additional ordinary RAM |

These are linker-region declarations, not measurements of free memory on a user's calculator. The managed heap is a distinct working-memory resource; its available total and largest block need measurement on the selected firmware. Hardware capacities alone cannot establish a safe workbook limit. [Official linker script](https://github.com/swissmicros/DMCP5_SDK/blob/master/stm32_program.ld)

QSPI is external flash with platform-specific sharing and write behavior. The manufacturer's manual explains that its mode changes during disk writes and that it sleeps during power-off. The initial proposal avoids requiring a custom QSPI resource replacement. This reduces installation and recovery complexity and avoids assuming that C47-specific resources will be present. [DMCP development manual](https://technical.swissmicros.com/dmcp/devel/dmcp_devel_manual/)

The SDK interface and keymap also matter. A program declares its interface requirements and can identify an expected keymap. A physical key position, a mapped key code, and a printed overlay label are related but different things. Supporting both hardware targets requires testing the intended mapping rather than assuming an overlay changes what code sees. [Program metadata](https://github.com/swissmicros/DMCP5_SDK/blob/master/dmcp/sys/pgm_syscalls.c)

The Brainbox applications demonstrate useful DMCP5 patterns for drawing, menus, sleep, and persistence. They are references, not guaranteed compatible building blocks. Their interfaces and local firmware images are not all from the same revision. The project should establish a supported hardware/firmware/keymap matrix from evidence. Original DM42 uses the older platform and remains outside the first scope. These boundaries keep the product claim precise: two intended targets, each requiring independent qualification.

<!-- pagebreak -->

## 12 · How source code becomes a working `.pg5` program

The source repository would contain our application, an adapter, selected engine files, the required SDK subset, and reproducible build instructions. An ARM cross-compiler runs on the development computer and produces machine code for the calculator. A linker joins the compiled pieces and places code and data in the regions expected by DMCP5. The developer's normal desktop executable format is not the delivered calculator application.

The official build produces an ELF file for linking/debugging, derives a flash binary, and packages a `.pg5` image. Its packaging script fills in a size field and appends a SHA-1 digest. This is a loader-facing integrity mechanism in the inspected process, not proof of authorship or a modern security signature. The intended public filename would be `SHEETS47.pg5`. [SDK build](https://github.com/swissmicros/DMCP5_SDK/blob/master/Makefile), [packaging script](https://github.com/swissmicros/DMCP5_SDK/blob/master/bin/add_pgm_chsum)

```text
Sheets47 UI + commands + persistence
              + engine adapter + selected engine
              + required SDK startup/service bindings
                           |
                    ARM compile and link
                           |
               ELF + map + size measurements
                           |
                 package SHEETS47.pg5
                           |
                 DMCP5 loads the program
```

The program metadata includes an entry address, identity/version information, interface requirements, and QSPI/keymap information. The startup code copies initial data into RAM, zeros uninitialized static data, calls static constructors, and then enters `program_main`. This is why static data use and C++ initialization must be audited, not just the size of source files. [Service/startup bindings](https://github.com/swissmicros/DMCP5_SDK/blob/master/dmcp/sys/pgm_syscalls.c), [startup assembly](https://github.com/swissmicros/DMCP5_SDK/blob/master/dmcp/startup_pgm.s)

Once running, the application uses the SDK service interface for display, input, allocation, files, and lifecycle actions. The engine is ordinary linked application code and calculates locally. No internet connection, desktop program, or second calculator application is part of the proposed calculation path. Workbook files are separate from the executable; changing a worksheet must not require rebuilding the `.pg5`.

The inspected SDK's `post_main` returns control through a reset into DMCP rather than returning to a desktop shell. The actual user-facing “leave Sheets47” path should save or warn about unsaved work, close files, and use the supported platform route. Returning to C47/R47 requires its own documented loading/restoration procedure; it is not proposed as a window switch.

The engineering evidence includes more than a finished file: compiler identity, SDK revision, required interface, target flags, link map, section sizes, runtime symbols, and hardware test records. The default reference Makefile also has cleanup on an error path, so it must be adapted before execution under this repository's deletion rules. This report has inspected it, not run it or installed an engine.

<!-- pagebreak -->

## 13 · The proposed architecture and the path of one edit

The architecture should separate worksheet meaning from device behavior. The engine owns cell contents and formula semantics. Sheets47 owns commands, presentation, project metadata, and persistence. The platform layer owns DMCP-specific calls. The adapter translates a small project vocabulary into the selected engine's operations. This allows host testing and limits the spread of engine-specific types.

```text
Physical keypad -> logical actions -> worksheet/editor UI
                                          |
                                   application commands
                                    /              \
                         engine adapter          save/load
                               |                     |
                       selected engine <--- cell traversal

UI, commands, and storage -> platform services -> DMCP5
                                      or -> host test substitute
```

The engine should remain the principal workbook store. Copying every cell into a separate UI model would consume memory and create disagreement between displayed and calculated data. UI metadata can hold cursor position, viewport, column widths, and project formats when the chosen engine does not already provide the needed representation. One component must be authoritative for each property. The [saved architecture](../architecture.md) establishes this separation.

The adapter contract should cover reading a value and formula, setting content, clearing cells, range copy/move, structural edits, recalculation, error reporting, and traversal of populated cells for saving. It also needs a resource/failure contract. “Out of memory” must tell the caller whether the command was rejected, completed, or left the session needing recovery. Avoid a lowest-common-denominator abstraction so broad that every candidate would require hidden reimplementation.

Consider editing `B2` from `3` to `4`. The UI initially holds the new text in a separate editor buffer. Cancel discards that draft. Commit submits a command; the adapter validates and calls the engine. The engine changes the cell and recalculates affected formulas according to its semantics. The UI refreshes the displayed cells and marks the workbook as modified. A later Save serializes committed data. Cursor movement alone need not alter the workbook or write storage.

The command needs a defined failure outcome. A malformed formula can be retained as an engine error if that is the selected product policy, or rejected while preserving the editor text. An allocation failure must not silently lose the previous valid value. We can request strong behavior, but the actual implementation depends on what the engine guarantees and what target tests demonstrate. Full-workbook copies for every edit would be an expensive fallback.

The same application commands and engine should run in a host harness with substitute display/file/input services. That creates useful tests without changing formula semantics. It cannot prove physical key behavior, battery performance, or storage durability on hardware. Switching engines would preserve more UI code under this architecture, but it would still require semantic, persistence, and compatibility work; the adapter is not a promise of cost-free replacement.

<!-- pagebreak -->

## 14 · Memory, performance, and numerical behavior

The most important memory distinction is between permanent program storage and temporary working space. Flash holds code and constants. Static RAM holds always-present writable state. The heap holds dynamically allocated worksheet data and other objects. The stack holds active calls and local working data. A small program image can still require too much heap or stack; a successful link only answers part of the question.

Sparse storage allocates mainly around populated cells, but every cell may require more than its visible number: text, cached value, type, allocator bookkeeping, tree/tile nodes, dependency information, and formatting. Widely scattered cells can cost more than a compact block. A formula's temporary evaluation storage can exceed the permanent cost of the cell that holds it.

A useful planning relationship is:

```text
Safe working budget = measured available memory
                    - platform/UI/storage reserves
                    - worst tested command temporaries
                    - safety margin
```

This is a budgeting method, not an established numerical limit. The SDK exposes total and largest-free-block queries. Fragmentation means a request may fail despite a larger total amount of free memory. Limits should cover formula length, nesting, populated cells, clipboard/range size, undo storage, and expensive calculations. They should be published only after measurement. [Memory service declarations](https://github.com/swissmicros/DMCP5_SDK/blob/master/dmcp/dmcp.h)

Rendering should visit visible cells; it should not recalculate formulas merely because the cursor moved. Bulk edits should avoid repeated full recalculation when the engine permits a batch. Long calculation must either complete within a tested response bound or support safe cooperation/cancellation. Drawing a “busy” message does not make a blocking engine interruptible. Removing features can also save little if the linker still pulls in shared implementation code, so measure the result.

The provisional numerical baseline is IEEE-style binary64: roughly fifteen to seventeen significant decimal digits, with behavior depending on the operation and display conversion. Decimal fractions such as 0.1 are not generally represented exactly. Displaying two decimal places is formatting; it does not make every intermediate calculation exact decimal arithmetic. Changing to another numerical representation later would affect engine compatibility and function implementations.

We must not assume C47 precision, angle modes, or formatting. Sheets47 will use the chosen engine's semantics and explicitly documented product settings. A trigonometric function's units, text-to-number conversion, comparison tolerance, rounding, division errors, and date conventions require defined behavior. A workbook should not change meaning silently when moved from a desktop spreadsheet.

The checked SDK build advertises a single-precision FPU configuration. That does not make the engine's `double` calculations single precision; it means required double arithmetic helpers and performance must be established for the actual toolchain/ABI. [SDK compiler flags](https://github.com/swissmicros/DMCP5_SDK/blob/master/Makefile)

The management implication is to reserve headroom before adding features. A worksheet that opens successfully but cannot afford one edit, a Save buffer, or orderly recovery is already too large for a reliable supported limit.

<!-- pagebreak -->

## 15 · The worksheet the user sees

The required main workspace is a familiar graphical spreadsheet with visible cells, row and column headings, selection, and scrolling. It should identify the active cell, distinguish formulas from displayed results, and show the input mode. A six-key command bar supplies worksheet actions. The following is a layout sketch; the [graphical screen concepts added on 13 November](../design/README.md) show original 400 × 240 drawings informed by Chess47/Nav47 screenshots, with actual cell borders and inverted headings. These remain design proposals.

```text
+--------------------------------------------------+
| BUDGET     B2     NAV       *modified              |
| Selected: 3                                      |
+------+-----------+-----------+-----------+--------+
|      | A         | B         | C         | D      |
| 1    | Item      | Qty       | Price     | Total  |
| 2    | Bolts     | [3]       | 2.50      | 7.50   |
| 3    | Washers   | 4         | 0.75      | 3.00   |
| 4    |           |           |           |        |
+------+-----------+-----------+-----------+--------+
| EDIT | RANGE | FORM | FORMAT | FILE | >>           |
+--------------------------------------------------+
```

The display has 400 × 240 pixels; preserving the 32-pixel menu area leaves 208 pixels for status, selected-cell preview, headings, and grid. The number of visible rows and columns depends on font and width decisions. It should not be promised before a real screen prototype. Narrowing every cell until a desktop-sized worksheet fits would make the calculator harder to use. [Display constants](https://github.com/swissmicros/DMCP5_SDK/blob/master/dmcp/dmcp.h)

Navigation moves the selection and scrolls the viewport only when needed. A Go To command reaches a distant cell without repeated key presses. Page movement and a clear indication of off-screen content should support larger sheets. The full selected value or formula can be inspected in a preview/editor even when the grid shows a shortened representation. A shortened numeric display must be visibly distinguishable from the stored value.

The first menu page is proposed as EDIT, RANGE, FORM, FORMAT, FILE, and a next-page action. RANGE would expose copy, paste, fill, insert, clear, and return. FORM would help insert functions and references. FORMAT would initially offer a small set of number displays and column-width adjustments. Repeated navigation should preserve context rather than reset the cursor or viewport.

Input should have explicit modes. In NAV, keys perform navigation actions according to the verified keymap. In EDIT, they enter or modify content. A formula-reference picker temporarily moves a selection while preserving the draft expression. Digits must not unpredictably alternate between moving and typing. The final physical bindings require tests on DM32 and DM42n and their intended overlays.

The interface should use monochrome contrast, a consistent Back/Cancel action, and short explanations for errors. A selected range needs a visible anchor and extent. Help should explain the action available now, including how to commit or abandon an edit. The goal is a spreadsheet that feels purposeful on a calculator rather than a miniature desktop window.

<!-- pagebreak -->

## 16 · A complete proposed user session

Imagine a user preparing a small parts budget. They load Sheets47 using the supported DMCP5 procedure, choose New, and see cell A1. They enter headings for Item, Quantity, Unit Price, and Total. Text entry uses the application's alpha mode; repeated entry should offer a predictable move to the next cell rather than requiring a menu for every value. This workflow is a proposal to validate with a keypad prototype.

They enter `Bolts`, `3`, and `2.50` in row 2, then select D2. FORM opens a guided expression entry. They choose the quantity cell using the reference picker, insert multiplication, choose the unit-price cell, and commit. With an A1-based engine and presentation, the expression is `=B2*C2`, and the displayed result is 7.50. The UI can show a leading equals sign even if an engine API expects formula text without it; that boundary belongs in the adapter.

For row 3 they enter `Washers`, `4`, and `0.75`. Copying D2 to D3 should adjust the relative references to `=B3*C3`, producing 3.00. The user can then select the two totals and insert a SUM expression, giving 10.50. The exact function catalogue is subject to engine qualification; this example describes the desired initial behavior rather than a verified function set.

Suppose B6 contains a tax rate of 0.12. A copied tax formula might use `=D2*$B$6`, where the dollar signs hold the tax-rate reference fixed. A reference-lock command could cycle relative and absolute forms without typing every symbol. This is especially useful on a small keypad. If mtCellEdit is chosen, its native R1C1 behavior needs a deliberate product decision before promising this exact notation.

The user changes B2 from 3 to 5. D2 becomes 12.50 and the subtotal becomes 15.50. The application highlights the active cell and updates affected visible results. It does not need to animate every dependent cell or rewrite storage after each cursor move. An unsaved indicator remains visible until a Save succeeds.

They choose FILE → Save, name the workbook BUDGET, and receive confirmation only after the proposed save protocol has completed. Turning the calculator off should use the supported power path. On resuming, the application should restore a valid session or offer the most recent valid saved workbook, according to verified lifecycle guarantees. Retaining RAM during ordinary sleep and recovering after reset are different requirements.

Later, they export values as CSV for a computer. The export dialog explains whether it exports results or formula text and whether formats are lost. Their native workbook remains the editable master. Before returning to C47/R47 they save and use the documented platform transition; the normal calculator program must be loaded again. This session connects the engine choice to the actual product: correct copy behavior, easy references, visible state, reliable saves, and predictable movement between applications.

<!-- pagebreak -->

## 17 · Editing rules, functions, and understandable failures

The first release needs a small set of operations that work consistently. A coherent baseline is numeric/text/formula entry, arithmetic, selected common functions, range selection, copy/paste, fill, clearing, structural edits, basic display formats, Save/Open, and simple table exchange. This is a proposed scope for qualification. A function is included only when its semantics, resource use, and error behavior are supported by the chosen engine and tests.

Copy and move must remain distinct. Copying a formula commonly shifts relative references to the new position. Moving cells can require references elsewhere to follow the moved data. Inserting a row can change both positions and references; clearing content leaves the grid structure in place. These differences should be reflected in clear menus rather than hidden behind one ambiguous “delete” command.

The command layer should also update any project-owned formatting metadata when structural edits occur. Otherwise values may move while widths, formats, or selections remain attached to the wrong coordinates. If the engine already owns these properties adequately, use that representation instead of maintaining a duplicate. This is a practical reason to inspect more than the formula evaluator.

| Situation | Proposed user-facing response |
| --- | --- |
| Malformed expression | Identify the problem and preserve the draft for correction |
| Division by zero or invalid reference | Show a recognizable error value with an inspectable explanation |
| Circular dependency | Report it; do not spin indefinitely or silently show an old answer |
| Operation exceeds a measured limit | Explain which limit applies and preserve usable committed work |
| Memory exhaustion | Reject safely or follow a tested recovery path; distinguish from formula errors |
| Unsupported function on load | Preserve recoverable content and report incompatibility explicitly |
| Save failure | Keep the workbook marked unsaved and retain the last valid stored generation |

Not all these guarantees come automatically from an engine. The error vocabulary should preserve upstream distinctions while translating them into concise messages. Replacing an error with zero would produce plausible but wrong totals. Cached results must not be presented as freshly calculated when recalculation is incomplete.

Undo deserves a measured policy. One-step undo or a bounded history may be affordable; unbounded snapshots are inappropriate. Range paste can require a temporary copy, and overlapping source/destination ranges need defined behavior. A large fill or insert must have a predictable completion or rejection rule. Cancellation must be supported by the operation itself, not just by a keyboard shortcut that cannot be serviced while it runs.

Dates, financial conventions, locale-dependent number parsing, dynamic arrays, and multiple sheets should not enter the first release by implication. They can be valuable later but require additional user explanations and tests. An engine's internal support is an option, not an obligation to expose it. A dependable small worksheet is more useful than a large menu whose commands fail unpredictably near device limits.

<!-- pagebreak -->

## 18 · Workbook storage, recovery, and exchange

The executable and workbook must be separate artifacts. `SHEETS47.pg5` provides the application; files in the proposed `/SHEETS47` directory hold user work. A native workbook should preserve formulas, value types, supported formats, and enough metadata to reopen meaningfully. CSV/TSV should be interchange formats, not the sole recoverable representation. This follows the project's [persistence proposal](../architecture.md).

A proposed native format has a recognizable identifier, version, engine/semantics identifier, bounded record lengths, and an integrity check. It records populated cells rather than serializing raw in-memory objects or pointers. Stable stored numeric encoding and documented byte order prevent accidental dependence on one compiler's structure layout. Formula text is authoritative for expressions; cached results, if stored, must be marked as caches rather than substituted for successful recalculation.

**Recoverable Save proposal:** write a new generation while retaining the previous valid one; check all write/close outcomes; validate the completed generation; then select it as current using a mechanism supported by the actual filesystem. On Open, reject incomplete or corrupt generations and offer a known-valid one. Exact names, retention counts, and the selection-record scheme should be chosen after testing SDK behavior.

This approach is a design direction, not a power-loss guarantee. Rename may not be atomic, a close may not imply all data is durable, and two files can share filesystem metadata whose loss affects both. Checksums detect many corruption cases but do not repair missing data. We need target experiments around interrupted writes, near-full storage, and USB disk transitions before making a durability claim.

Load also needs a failure policy. A corrupt file must not erase a usable open worksheet. Staging a second complete workbook is convenient but can double memory demand; validating a bounded stream or requesting a saved checkpoint may be more appropriate. The accepted behavior should be explicit and tested. File lengths, coordinates, formula lengths, and record counts need checks before allocating large buffers.

For computer exchange, start with values-only CSV or TSV and a clear explanation of lost formatting/formulas. Text quoting, embedded separators, newlines, numeric precision, and character support need documented conventions. A later formula-preserving exchange mode must distinguish text that starts with `=` from an actual formula. A desktop spreadsheet may interpret such text differently, so an export should not silently change its meaning.

USB mass-storage access creates an ownership transition: application files should be closed and writes suspended while the computer controls the disk, following the supported platform path. Resume should refresh relevant file state. Installation, workbook transfer, and ordinary calculation are distinct flows.

During development, deleting any existing or generated path requires explicit user approval under AGENTS.md. The eventual application's user-facing retention/overwrite commands need their own deliberate design; this report authorizes and performs no file deletion and proposes no automatic development cleanup.

<!-- pagebreak -->

## 19 · Power, responsiveness, and proving the whole application

A calculator is expected to wait efficiently, turn off predictably, and resume without surprising the user. The application should use platform sleep and power services rather than poll continuously. It must consider normal idle sleep, explicit off, auto-off, USB power changes, reset, and an interrupted save as distinct events. The SDK/demo and the documented Brainbox examples provide lifecycle patterns to study, not a finished Sheets47 policy. [SDK header](https://github.com/swissmicros/DMCP5_SDK/blob/master/dmcp/dmcp.h), [official demo](https://github.com/swissmicros/SDK5demo/blob/master/src/main.c), [local findings](../research/brainbox.md)

The proposed event loop receives a logical input or platform event, performs bounded work, updates the display when needed, and yields to the platform while idle. A long engine call can prevent that loop from seeing keys and power events. Therefore a maximum acceptable calculation response or a genuinely cooperative engine path must be established before expanding the function set.

Draft editor text, committed workbook contents, and saved disk state are three different things. Ordinary sleep may preserve RAM, but reset or power interruption can invalidate it. The UI must never imply that an unsaved workbook is durable simply because the calculator can usually resume. Checkpoints should be deliberate and should avoid turning every cursor move into a flash write.

Testing should be divided by what it proves:

| Evidence level | Questions it can answer | Important limit |
| --- | --- | --- |
| Host tests | Formula results, command semantics, encoding, controlled allocation/file failures | Different allocator, timing, ABI, and filesystem |
| ARM cross-link | Target symbols, section sizes, startup/runtime integration | Does not demonstrate execution or hardware behavior |
| Target instrumentation | Measured heap/stack, timing, runtime failure handling | Requires the actual build and platform configuration |
| DM42n and DM32 trials | Keys, display, sleep, USB, storage, recovery | Each tested firmware/keymap combination needs a record |

Meaningful formula cases include forward and reverse dependency chains, copied relative/absolute references, malformed expressions, circular references, large range reductions, and structural edits. A test that only adds two constants does not qualify an engine. Failure tests should interrupt allocation at successive points, fail writes and closes, and verify that a usable workbook or saved generation remains.

Hardware records should identify the device model, DMCP5 version, keymap/overlay, executable identity, interface requirement, startup memory, and exact observations. Storage interruption and power/resume tests need a controlled procedure and recoverable sample data. They must not involve unattended firmware replacement or personal workbooks.

The product definition of “works” is consequently broader than producing a `.pg5` file. It includes a legible screen, correct calculations, predictable commands, safe resource limits, clear failures, and recoverable user work. Neither a desktop test nor a successful compile alone supports a claim of DM42n/DM32 compatibility.

<!-- pagebreak -->

## 20 · Proposed qualification and delivery decision

Begin with graphical worksheet and interaction design alongside engine qualification. The visible grid experience defines the product; engine feasibility determines how to support it reliably. This report starts no implementation. The [development workflow](../development.md) defines the proposed gates.

**Gate 1: define the graphical experience and candidate.** Review cell-grid, editor, scrolling, and range-selection screens with keypad flows. Pin the official SDK/compiler and a candidate source set, initially Spreader. Record versions, flags, and provenance. Inspect recipes for deletion and downloads. The candidate must enable the reviewed UI and leave resources for it.

**Gate 2: demonstrate spreadsheet behavior on a host.** Use a small harness around the proposed adapter. Exercise empty sheets; 1,000 short text/numeric cells; 100 formulas; clustered and scattered data; dependency chains; reductions; copying; structural edits; invalid expressions; cycles; and forced allocation failures. These are experiment sizes, not promised supported capacities. Record failures as findings rather than weakening the tests to fit the candidate.

**Gate 3: link and measure the target core.** Produce a minimal DMCP5 harness before extensive implementation. Record flash, `.data + .bss`, runtime symbols, peak/steady heap, largest allocation, stack use, and response times. Confirm allocator routing, exceptions, and recovery. Reserve memory for the reviewed graphical worksheet, editor, clipboard, and storage. Separate hardware measurements from host estimates.

**Gate 4: choose or switch.** If Spreader needs major runtime work or fails integrity requirements, stop and test a selected mtCellEdit core against the same cases. Require an explicit decision on recalculation and notation. Reconsider Ixion or SC-IM if both initial options fail. Record why the winner is maintainable, not merely why one demonstration ran.

**Gate 5: build the useful application in slices.** First prove grid/cursor/editor/softkeys and lifecycle; then native Save/Open and recovery; then range editing and bounded undo; then exchange and hardware qualification. A sample budget like page 16 should become an end-to-end acceptance scenario. Publish supported functions, limits, and firmware/keymap combinations only from evidence.

| Risk | Early evidence that reduces it |
| --- | --- |
| Engine runtime consumes the project | Early target link with a graphical UI resource budget |
| Small sheets work but edits fail | Peak-memory and allocation-failure measurements |
| Wrong answers after copy/insert | Reference and dependency regression cases |
| Port becomes a private engine rewrite | Reviewable source closure and small patch set |
| Saves appear successful but lose work | Interrupted-write and full-storage target tests |
| Users cannot enter formulas comfortably | Keypad prototype and reference-picker trials |

A calendar estimate needs both interface design and target feasibility evidence. The reviewable output should pair worksheet/key-flow designs with an engine decision record covering measurements, limitations, and licensing. The product is a familiar graphical spreadsheet that uses the calculator screen fully, with a qualified engine underneath and reliable storage around it.

<!-- pagebreak -->

## 21 · Evidence register: engine sources

**Research date: 12 November 2026.** The report revisits the saved research and checks primary project sources. Branch URLs below are moving references, not pinned dependency revisions. A dependency import must record an immutable identity separately. Retrieved source text was inspected; no engine was built, benchmarked, or installed. Technical judgments are identified in the body as proposals, inferences, or qualification questions.

**Spreader.** The overview establishes its headless C++20 purpose and alpha designation. The public interface establishes the visible operations. Grid and build files support the storage/runtime/dependency discussion. BSD obligations come from the license text.

- [README and build requirements](https://github.com/gershnik/spreader/blob/master/README.md)
- [Sheet API](https://github.com/gershnik/spreader/blob/master/lib/code/inc/spreader/sheet.h)
- [Cell grid and allocation documentation](https://github.com/gershnik/spreader/blob/master/lib/code/inc/spreader/cell-grid.h)
- [Top-level configuration](https://github.com/gershnik/spreader/blob/master/CMakeLists.txt)
- [Helper/test dependency definitions](https://github.com/gershnik/spreader/blob/master/cmake/dependencies.cmake)
- [License](https://github.com/gershnik/spreader/blob/master/LICENSE)

**mtCellEdit.** The author page establishes the reusable C/C++ library, desktop context, ZIP/TSV format, and GPL terms. Implementation details in this report refer specifically to the local 4.0 archive, rather than assuming that a historical or future version behaves identically. The archive identity and inspected paths are listed on page 22.

- [Author's overview](https://www.marktyler.org/ced/)
- [4.0 source archive](https://www.marktyler.org/download/mtcelledit-4.0.tar.xz)

**SC-IM.** Its own overview describes the complete application and features. Interpreter includes and the cell structure expose coupling and storage questions. The root license is the inspected four-condition text; exact selected-file terms still require an import audit.

- [Application overview](https://github.com/andmarti1424/sc-im/blob/main/Readme.md)
- [Interpreter implementation](https://github.com/andmarti1424/sc-im/blob/main/src/interp.c)
- [Cell structures](https://github.com/andmarti1424/sc-im/blob/main/src/sc.h)
- [License](https://github.com/andmarti1424/sc-im/blob/main/LICENSE)

**Other options.** Ixion's README is explicit about capabilities, omissions, and dependencies. Gnumeric's site and source README support the desktop/dependency description. HyperFormula's README and license identify its language, runtime context, and licensing alternatives. LibreOffice is a desktop reference, not a qualified embedded engine.

- [Ixion overview](https://github.com/kohei-us/ixion/blob/master/README.md)
- [Gnumeric project](https://www.gnumeric.org/) and [source requirements](https://github.com/GNOME/gnumeric/blob/master/README)
- [HyperFormula overview](https://github.com/handsontable/hyperformula/blob/master/README.md), [license](https://github.com/handsontable/hyperformula/blob/master/LICENSE.txt), and [licensing guide](https://handsontable.com/docs/hyperformula/guide/licensing.html)
- [LibreOffice licensing](https://www.libreoffice.org/licenses/)
- [Mozilla's MPL-2.0 FAQ](https://www.mozilla.org/en-US/MPL/2.0/FAQ/)

These sources establish available facilities and documented build assumptions. They do not establish comparative DMCP5 speed, memory capacity, allocation safety, or supported firmware. The report deliberately avoids function-count rankings and invented footprint estimates because they would imply evidence that has not been collected.

<!-- pagebreak -->

## 22 · Evidence register: platform, local research, and terminology

**Official platform sources.** The SDK source was inspected through the connected GitHub service; the development manual was checked on the manufacturer's site. Source-level constants and recipes are evidence for the examined interface, not a claim about every installed firmware version.

- [DMCP5 display/platform declarations](https://github.com/swissmicros/DMCP5_SDK/blob/master/dmcp/dmcp.h)
- [Memory layout and discarded library sections](https://github.com/swissmicros/DMCP5_SDK/blob/master/stm32_program.ld)
- [Compiler flags, C++ settings, and package build](https://github.com/swissmicros/DMCP5_SDK/blob/master/Makefile)
- [Metadata, allocator bindings, and return path](https://github.com/swissmicros/DMCP5_SDK/blob/master/dmcp/sys/pgm_syscalls.c)
- [Program startup](https://github.com/swissmicros/DMCP5_SDK/blob/master/dmcp/startup_pgm.s)
- [Program packaging](https://github.com/swissmicros/DMCP5_SDK/blob/master/bin/add_pgm_chsum)
- [SDK license and NOMAS notice](https://github.com/swissmicros/DMCP5_SDK/blob/master/LICENSE)
- [Official demonstration program](https://github.com/swissmicros/SDK5demo/blob/master/src/main.c)
- [DMCP development manual](https://technical.swissmicros.com/dmcp/devel/dmcp_devel_manual/)

**Local evidence.** The [engine study](../research/engines.md), [architecture](../architecture.md), [workflow](../development.md), [Brainbox findings](../research/brainbox.md), [fingerprint manifest](../research/brainbox-manifest.json), and [dependency policy](../../third_party/README.md) supply the saved project context. Brainbox's 2,432-file inventory is not a line-by-line audit. Its local trees are references, not build dependencies or authenticated upstream revisions. This report did not recopy or reinventory them.

The ignored `.local/research/mtcelledit-4.0.tar.xz` archive was read directly, without extraction or execution. Its verified SHA-256 is:

```text
dee2f85798ab538e4e0b0d2c662f2dff5c85f8be3c9fe417c73354b7aadcf7d8
```

Relevant paths under its `mtcelledit-4.0/` root are `libmtcelledit/src/mtcelledit.h`, `ced_eval.h`, `ced_eval.cpp`, `ced_sheet.c`, `ced_book.c`, and `COPYING.txt`. The last four source filenames after `mtcelledit.h` are in the same source directory; `COPYING.txt` is at the archive root. The GPL discussion uses the supplied license's sections 5 and 6. No target benchmark or recalculation experiment is implied by reading these functions.

| Term | Meaning in this report |
| --- | --- |
| Adapter | Project-owned boundary translating commands to one engine |
| Cross-link | Produce a linked calculator-target image on a development computer |
| Dependency closure | Every source/library/runtime piece actually needed to build and run |
| Heap / stack | Dynamic object storage / active-call working storage |
| ABI | Rules by which compiled code passes values and calls other code |
| Runtime | Supporting code needed by language/library facilities during execution |
| Sparse | Storage organized around occupied data rather than the entire coordinate rectangle |
| Qualification | Evidence that a selected configuration meets defined product requirements |

**Repository impact:** this report and its rendering artifacts are documentation. They select no engine, introduce no application code or build target, alter no hardware, and make no compatibility claim. The proposed development gates are the basis for a subsequent implementation decision.
