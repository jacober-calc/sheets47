# sheets47 0.1.0: first-version guide

This is the first functional implementation, for standalone DMCP5 operation on
DM42n and DM32. Its graphical cell grid follows the agreed worksheet concept.
The same commands, renderer, and Spreader backend can be exercised on a host.
Read the validation record before treating either hardware target as tested.

## Program and user files

Program: `sheets47.pg5`. Version: `0.1.0`. Own saved workbook, viewport, and cell
format information live under `/SHEETS47` at the calculator disk root. There is
no separate QSPI resource package and no C47/R47 runtime dependency.

Use the supported DMCP5 program-loading procedure after backing up the device
and retaining the normal calculator program and a known restoration procedure.
Check that the installed firmware satisfies interface 3.17. No tool in this
repository uploads software or changes calculator firmware automatically.
This guide intentionally does not instruct users to bypass loader warnings.

## Worksheet and entry

On startup, the app attempts to open the latest valid saved workbook. With no
save, it opens an empty worksheet. FILE → DEMO loads the sample budget: change
quantity B2 and watch the line total and SUM change. The demo is unsaved until
Save succeeds. Replacing unsaved work requires an explicit on-screen confirmation.

Navigation uses UP/DOWN or numeric 8/2 for rows, and 4/6 for columns. ENTER or 5
starts editing the current cell; F1 EDIT does the same. Numeric keys enter digits
while editing. Typing 8 in worksheet navigation moves; typing 8 in EDIT inserts
8. The physical SWAP and RDN positions also provide left/right editor movement
through their DMCP key codes; actual overlays require hardware verification.

- ENTER commits an edit and recalculates. EXIT/F6 cancels the draft.
- FORM starts a formula with `=`. Ordinary entry accepts numbers or text.
- A leading apostrophe forces text, useful for `00123` or literal formula text.
- EDIT → ABC opens an on-screen letter/symbol palette. UP/DOWN or 8/2 changes
  banks; F1–F6 inserts the displayed character. ENTER returns to the editor.
- REF lets you navigate the grid and insert a cell address with ENTER. Return
  to REF for another endpoint when constructing a range; `:` and `$` are also
  available from the symbol palette.
- FUNC inserts one of the supported function names and an opening parenthesis.
  The editor's `(` and `)` softkeys and arithmetic keys complete the expression.
- GO TO under `>>` accepts a cell address, then returns to the worksheet.

Text is ASCII in this version. The original bitmap renderer displays lowercase
letters using uppercase glyphs while preserving stored case. Long cell contents
are abbreviated in the grid; open the editor to inspect or change the source.
There is no blinking cursor and no continuously updating clock.

## Range and row commands

RANGE starts a selection at the current cell. Move to extend the rectangle.
COPY captures its contents at that moment. Move to the destination, then PASTE
from the range menu. Relative references shift and absolute references remain
fixed according to Spreader. Empty cells in the copied rectangle clear the
corresponding destination. The clipboard is RAM-only.

FILLDN copies the top row's formulas/values down the selected columns. INSROW
inserts above the active row. DELROW under `>>` removes the active row after
confirmation. CLEAR requests confirmation before removing selected cell contents.
The row operation rewrites formulas through Spreader; deleting referenced cells
can leave a visible `#REF!` error. Inserting with an occupied last row is refused.

FORMAT cycles populated selected cells through General, Fixed2, and Scientific.
It changes display, not stored numeric precision. Blank-cell formats and column
resizing are not implemented. Row/column headings and the active selection remain
visible around the grid, with six contextual softkeys at the bottom.

## Functions and limits

This first version deliberately bounds the working set: A1:Z256, at most 256
populated/source cells, input length at most 143 ASCII characters, and parenthesis
nesting at most 12. These are conservative implementation bounds, not measured
hardware capacity promises. Calculated spill output is also checked against the
cell/grid bounds. Whole-row/column ranges, additional sheets, external references,
arrays entered with braces, named ranges, and volatile functions are not exposed.

Supported functions: SUM, AVERAGE, MIN, MAX, COUNT, IF, ROUND, ROUNDUP, ROUNDDOWN.
Arithmetic and power expressions are handled by Spreader. This pinned upstream
revision does not implement ABS or SQRT; the app rejects them explicitly.
Division, invalid syntax, and circular dependencies produce visible engine errors
where supported rather than substituting zero. Calculations use binary64 and
do not inherit C47's numerical settings. Date, financial, Excel/ODS, and CSV/TSV
features are deferred. Undo and workbook-name selection are also deferred.

## Save, power, and recovery

FILE → SAVE writes the current committed workbook and viewport. The asterisk
marks unsaved workbook changes. OPEN, NEW, and DEMO ask before discarding unsaved
work. Version 0.1.0 manages one current workbook; it does not yet offer multiple
named workbooks or an automatic old-generation retention policy.

Each Save creates a new `Bnnnnnnn.S47` generation and verifies its bytes, then
updates one of `HEAD0.S47` and `HEAD1.S47`. The previous head/generation is kept.
Open validates checksums and can fall back to an earlier valid generation.
No application command deletes a file. Generations accumulate: keep backups and
manage space deliberately. Storage failures leave the RAM workbook unsaved.
This protocol has host failure tests, not a hardware power-loss guarantee.

SHIFT+EXIT requests normal power-off. If the committed workbook changed, the app
attempts a Save before sleep. Draft editor text remains in RAM for normal resume,
but is not committed or saved automatically. Reset or loss of RAM can lose a
draft. Cursor-only activity does not trigger an automatic disk write.
The app sleeps while idle and refreshes only changed LCD rows after user-visible
changes, with a full restoration on wake or return from a platform menu.

FILE → SETUP or SHIFT+0 opens the platform settings path after saving dirty work.
Finish or cancel an edit first. All file handles are closed before USB disk or
system menus; no background writes occur while those synchronous menus run.
Leaving for the usual calculator program uses the DMCP platform procedures.
A clean-reset startup skips automatic loading and lets FILE → OPEN restore work.

The physical power, USB, keymap, and filesystem behavior must still be tested on
each supported device/firmware combination. Battery savings are a design goal
supported by sleep/refresh behavior, not a measured battery-life claim.
