#!/usr/bin/env python3
"""Draw original 400x240 worksheet concepts; no upstream assets or cleanup.

Uses installed pycairo and a system font. Produces SVG and monochrome PNGs in
docs/design. The enlarged previews preserve the native image's individual pixels.
This is a design drawing, not application, engine, or calculator emulator code.
"""

from pathlib import Path

import cairo


OUT = Path(__file__).resolve().parents[1] / "docs/design"
WIDTH, HEIGHT = 400, 240
EDGES = [0, 28, 152, 222, 304, 390]
GRID_TOP, ROW_HEIGHT = 64, 21
ROWS = [
    ["Item", "Qty", "Price", "Total"],
    ["Bolts", "3", "2.50", "7.50"],
    ["Washers", "4", "0.75", "3.00"],
    ["Brackets", "2", "4.00", "8.00"],
    ["", "", "", ""],
    ["TOTAL", "", "", "18.50"],
]


def ink(ctx, white=False):
    ctx.set_source_rgb(*((1, 1, 1) if white else (0, 0, 0)))


def box(ctx, x, y, width, height, *, white=False):
    ink(ctx, white)
    ctx.rectangle(x, y, width, height)
    ctx.fill()


def line(ctx, x1, y1, x2, y2, width=1):
    ink(ctx)
    ctx.set_line_width(width)
    ctx.move_to(x1 + 0.5, y1 + 0.5)
    ctx.line_to(x2 + 0.5, y2 + 0.5)
    ctx.stroke()


def text(ctx, label, x, y, *, size=12, bold=False, white=False, align="left"):
    ctx.select_font_face("DejaVu Sans Mono", cairo.FONT_SLANT_NORMAL,
                         cairo.FONT_WEIGHT_BOLD if bold else cairo.FONT_WEIGHT_NORMAL)
    ctx.set_font_size(size)
    ext = ctx.text_extents(label)
    if align == "right":
        x -= ext.x_advance
    elif align == "center":
        x -= ext.x_advance / 2
    ink(ctx, white)
    ctx.move_to(round(x), round(y))
    ctx.show_text(label)


def outline(ctx, x, y, width, height, weight=2):
    box(ctx, x, y, width, weight)
    box(ctx, x, y + height - weight, width, weight)
    box(ctx, x, y, weight, height)
    box(ctx, x + width - weight, y, weight, height)


def triangle(ctx, points):
    ink(ctx)
    ctx.move_to(*points[0])
    for point in points[1:]:
        ctx.line_to(*point)
    ctx.close_path()
    ctx.fill()


def draw(ctx, ranged=False):
    ctx.set_antialias(cairo.ANTIALIAS_NONE)
    font_options = cairo.FontOptions()
    font_options.set_antialias(cairo.ANTIALIAS_NONE)
    font_options.set_hint_style(cairo.HINT_STYLE_FULL)
    ctx.set_font_options(font_options)
    box(ctx, 0, 0, WIDTH, HEIGHT, white=True)

    # Inverted application/workbook title; title and formula are separate rows.
    box(ctx, 0, 0, WIDTH, 19)
    text(ctx, "SHEETS47", 5, 14, size=13, bold=True, white=True)
    text(ctx, "BUDGET *", 394, 14, size=11, white=True, align="right")
    outline(ctx, 1, 22, 47, 20, 1)
    text(ctx, "D2", 24, 36, bold=True, align="center")
    text(ctx, "=B2*C2", 58, 36, size=13)
    line(ctx, 0, 45, 399, 45)

    # Column strip. The active column is reversed; the others stay light.
    box(ctx, EDGES[4] + 1, 46, EDGES[5] - EDGES[4] - 1, 17)
    for column, label in enumerate("ABCD", 1):
        text(ctx, label, (EDGES[column] + EDGES[column + 1]) / 2, 59,
             size=12, bold=True, white=column == 4, align="center")
    line(ctx, 0, 63, 390, 63, 2)

    # Optional range uses a sparse 1-bit pattern, not grayscale shading.
    if ranged:
        for y in range(GRID_TOP + ROW_HEIGHT + 3, GRID_TOP + ROW_HEIGHT * 4 - 2, 4):
            for x in range(EDGES[2] + 3, EDGES[5] - 2, 4):
                box(ctx, x, y, 1, 1)

    for row in range(6):
        top = GRID_TOP + row * ROW_HEIGHT
        if row == 1:
            box(ctx, 0, top, 28, ROW_HEIGHT)
        text(ctx, str(row + 1), 14, top + 15, size=12, bold=row == 1,
             white=row == 1, align="center")
        for column, label in enumerate(ROWS[row], 1):
            left, right = EDGES[column], EDGES[column + 1]
            if row == 0:
                text(ctx, label, (left + right) / 2, top + 15,
                     size=11, bold=True, align="center")
            elif column == 1:
                text(ctx, label, left + 5, top + 15, bold=row == 5)
            else:
                text(ctx, label, right - 5, top + 15, bold=row == 5, align="right")
        line(ctx, 0, top + ROW_HEIGHT - 1, 390, top + ROW_HEIGHT - 1)
    for edge in EDGES:
        line(ctx, edge, 46 if edge != 0 else GRID_TOP, edge, 189)

    # Active cell: double border and a small corner handle.
    outline(ctx, 304, 85, 87, 21, 2)
    box(ctx, 387, 102, 4, 4)
    if ranged:
        outline(ctx, 152, 85, 239, 63, 2)

    # Narrow scroll track; all sample screens use native 1-bit primitives.
    line(ctx, 392, 64, 392, 189)
    triangle(ctx, [(394, 69), (398, 69), (396, 65)])
    triangle(ctx, [(394, 185), (398, 185), (396, 189)])
    box(ctx, 394, 74, 4, 49)

    text(ctx, "B2:D4  9 CELLS" if ranged else "D2  7.50", 3, 202, size=10)
    text(ctx, "RANGE" if ranged else "READY", 221, 202, size=10, align="right")
    triangle(ctx, [(270, 198), (274, 195), (274, 201)])
    triangle(ctx, [(386, 198), (382, 195), (382, 201)])
    line(ctx, 277, 198, 379, 198)
    box(ctx, 277, 196, 46, 5)

    # Six physical softkey positions in the 32-pixel menu region.
    labels = ["COPY", "PASTE", "FILL", "INSERT", "CLEAR", "BACK"] if ranged else ["EDIT", "RANGE", "FORM", "FORMAT", "FILE", ">>"]
    for i, label in enumerate(labels):
        left = 2 + i * 66
        box(ctx, left, 210, 62, 28)
        text(ctx, label, left + 31, 228, size=11, white=True, align="center")


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    for name, ranged in [("worksheet-concept", False), ("range-concept", True)]:
        svg = cairo.SVGSurface(str(OUT / f"{name}.svg"), WIDTH, HEIGHT)
        draw(cairo.Context(svg), ranged)
        svg.finish()
        native = cairo.ImageSurface(cairo.FORMAT_RGB24, WIDTH, HEIGHT)
        draw(cairo.Context(native), ranged)
        native.write_to_png(str(OUT / f"{name}.png"))
        preview = cairo.ImageSurface(cairo.FORMAT_RGB24, WIDTH * 3, HEIGHT * 3)
        ctx = cairo.Context(preview)
        ctx.scale(3, 3)
        ctx.set_source_surface(native)
        ctx.get_source().set_filter(cairo.FILTER_NEAREST)
        ctx.paint()
        preview.write_to_png(str(OUT / f"{name}-3x.png"))
        print(f"Wrote {name}: native SVG, 400x240 PNG, and 3x pixel preview")


if __name__ == "__main__":
    main()
