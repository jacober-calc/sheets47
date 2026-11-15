#!/usr/bin/env python3
"""Render the paginated decision report with installed Markdown/Pango/Cairo tools.

Writes HTML and PDF beside the Markdown source. Performs no cleanup or downloads.
Requires markdown-it-py and the system Pango/Cairo shared libraries.
Run from any directory; pass a different source path as the sole optional argument.
"""

from __future__ import annotations

import html
import sys
import ctypes as ct
import ctypes.util
from pathlib import Path

from markdown_it import MarkdownIt


def native(library, name, result, *arguments):
    function = getattr(library, name)
    function.restype, function.argtypes = result, arguments
    return function


# Use the public native APIs directly: the installed Python GI package lacks
# its optional Cairo bridge. No installation or environment changes are needed.
CL = ct.CDLL(ctypes.util.find_library("cairo"))
PL = ct.CDLL(ctypes.util.find_library("pango-1.0"))
PC = ct.CDLL(ctypes.util.find_library("pangocairo-1.0"))
GL = ct.CDLL(ctypes.util.find_library("gobject-2.0"))
PTR, INT, DOUBLE, STRING = ct.c_void_p, ct.c_int, ct.c_double, ct.c_char_p


class Rect(ct.Structure):
    _fields_ = [(name, INT) for name in ("x", "y", "width", "height")]


class Surface:
    def __init__(self, path=None, width=0, height=0):
        if path:
            self.ptr = native(CL, "cairo_pdf_surface_create", PTR, STRING, DOUBLE, DOUBLE)(str(path).encode(), width, height)
        else:
            self.ptr = native(CL, "cairo_recording_surface_create", PTR, INT, PTR)(0x3000, None)

    def finish(self):
        native(CL, "cairo_surface_finish", None, PTR)(self.ptr)
        status = native(CL, "cairo_surface_status", INT, PTR)(self.ptr)
        if status:
            raise RuntimeError(f"Cairo surface error {status}")

    def __del__(self):
        native(CL, "cairo_surface_destroy", None, PTR)(self.ptr)


class Context:
    def __init__(self, surface):
        self.surface = surface
        self.ptr = native(CL, "cairo_create", PTR, PTR)(surface.ptr)

    def __del__(self):
        native(CL, "cairo_destroy", None, PTR)(self.ptr)

    def move_to(self, x, y):
        native(CL, "cairo_move_to", None, PTR, DOUBLE, DOUBLE)(self.ptr, x, y)

    def line_to(self, x, y):
        native(CL, "cairo_line_to", None, PTR, DOUBLE, DOUBLE)(self.ptr, x, y)

    def set_source_rgb(self, r, g, b):
        native(CL, "cairo_set_source_rgb", None, PTR, DOUBLE, DOUBLE, DOUBLE)(self.ptr, r, g, b)

    def set_line_width(self, width):
        native(CL, "cairo_set_line_width", None, PTR, DOUBLE)(self.ptr, width)

    def rectangle(self, x, y, width, height):
        native(CL, "cairo_rectangle", None, PTR, DOUBLE, DOUBLE, DOUBLE, DOUBLE)(self.ptr, x, y, width, height)

    def fill(self):
        native(CL, "cairo_fill", None, PTR)(self.ptr)

    def stroke(self):
        native(CL, "cairo_stroke", None, PTR)(self.ptr)

    def tag_begin(self, name, attributes):
        native(CL, "cairo_tag_begin", None, PTR, STRING, STRING)(self.ptr, name.encode(), attributes.encode())

    def tag_end(self, name):
        native(CL, "cairo_tag_end", None, PTR, STRING)(self.ptr, name.encode())

    def show_page(self):
        status = native(CL, "cairo_status", INT, PTR)(self.ptr)
        if status:
            raise RuntimeError(f"Cairo context error {status}")
        native(CL, "cairo_show_page", None, PTR)(self.ptr)


class Layout:
    def __init__(self, ctx, markup, width, size, mono, bold):
        self.ptr = native(PC, "pango_cairo_create_layout", PTR, PTR)(ctx.ptr)
        context = native(PL, "pango_layout_get_context", PTR, PTR)(self.ptr)
        native(PC, "pango_cairo_context_set_resolution", None, PTR, DOUBLE)(context, 72)
        family = "DejaVu Sans Mono" if mono else "Liberation Serif"
        font = f"{family} {'Bold ' if bold else ''}{size}"
        description = native(PL, "pango_font_description_from_string", PTR, STRING)(font.encode())
        native(PL, "pango_layout_set_font_description", None, PTR, PTR)(self.ptr, description)
        native(PL, "pango_font_description_free", None, PTR)(description)
        native(PL, "pango_layout_set_width", None, PTR, INT)(self.ptr, round(width * 1024))
        native(PL, "pango_layout_set_wrap", None, PTR, INT)(self.ptr, 2)
        native(PL, "pango_layout_set_spacing", None, PTR, INT)(self.ptr, round(size * 0.12 * 1024))
        native(PL, "pango_layout_set_markup", None, PTR, STRING, INT)(self.ptr, markup.encode(), -1)

    def get_extents(self):
        ink, logical = Rect(), Rect()
        native(PL, "pango_layout_get_extents", None, PTR, ct.POINTER(Rect), ct.POINTER(Rect))(self.ptr, ct.byref(ink), ct.byref(logical))
        return ink, logical

    def index_to_pos(self, index):
        rect = Rect()
        native(PL, "pango_layout_index_to_pos", None, PTR, INT, ct.POINTER(Rect))(self.ptr, index, ct.byref(rect))
        return rect

    def get_text(self):
        return native(PL, "pango_layout_get_text", STRING, PTR)(self.ptr).decode()

    def __del__(self):
        native(GL, "g_object_unref", None, PTR)(self.ptr)


ROOT = Path(__file__).resolve().parents[1]
DEFAULT = ROOT / "docs/reports/2026-11-12-engine-and-application-report.md"
WIDTH, HEIGHT = 595.276, 841.89  # A4, PDF points
LEFT, RIGHT, TOP, BOTTOM = 46.0, 46.0, 65.0, 784.0
CONTENT_WIDTH = WIDTH - LEFT - RIGHT
MD = MarkdownIt("commonmark", {"html": False}).enable("table")


def inline_markup(token):
    """Return Pango markup, plain text, and UTF-8 byte ranges for hyperlinks."""
    output, plain, links, opened = [], [], [], []
    position = 0
    for child in token.children or []:
        kind = child.type
        if kind in {"text", "code_inline", "softbreak", "hardbreak"}:
            text = " " if kind == "softbreak" else "\n" if kind == "hardbreak" else child.content
            safe = html.escape(text)
            output.append(f'<span font_family="DejaVu Sans Mono" size="90%">{safe}</span>' if kind == "code_inline" else safe)
            plain.append(text)
            position += len(text.encode("utf-8"))
        elif kind in {"strong_open", "em_open"}:
            output.append("<b>" if kind == "strong_open" else "<i>")
        elif kind in {"strong_close", "em_close"}:
            output.append("</b>" if kind == "strong_close" else "</i>")
        elif kind == "link_open":
            opened.append((position, child.attrGet("href")))
            output.append('<span foreground="#15546d" underline="single">')
        elif kind == "link_close":
            start, target = opened.pop()
            links.append((start, position, target))
            output.append("</span>")
    return "".join(output), "".join(plain), links


def parse_blocks(source):
    tokens, blocks, i, list_depth = MD.parse(source), [], 0, 0
    while i < len(tokens):
        token = tokens[i]
        if token.type == "heading_open":
            blocks.append(("heading", int(token.tag[1:]), inline_markup(tokens[i + 1])))
            i += 3
        elif token.type == "paragraph_open":
            blocks.append(("list" if list_depth else "paragraph", 0, inline_markup(tokens[i + 1])))
            i += 3
        elif token.type in {"bullet_list_open", "ordered_list_open"}:
            list_depth += 1
            i += 1
        elif token.type in {"bullet_list_close", "ordered_list_close"}:
            list_depth -= 1
            i += 1
        elif token.type == "fence":
            blocks.append(("code", 0, (html.escape(token.content.rstrip()), token.content.rstrip(), [])))
            i += 1
        elif token.type == "table_open":
            rows, row = [], []
            i += 1
            while tokens[i].type != "table_close":
                if tokens[i].type == "tr_open":
                    row = []
                elif tokens[i].type == "inline":
                    row.append(inline_markup(tokens[i]))
                elif tokens[i].type == "tr_close":
                    rows.append(row)
                i += 1
            blocks.append(("table", 0, rows))
            i += 1
        else:
            i += 1
    return blocks


def make_layout(ctx, markup, width, size, *, mono=False, bold=False):
    return Layout(ctx, markup, width, size, mono, bold)


def height(layout):
    return layout.get_extents()[1].height / 1024


def show(ctx, layout, x, y, link_ranges=()):
    ctx.move_to(x, y)
    ctx.set_source_rgb(0.10, 0.15, 0.19)
    native(PC, "pango_cairo_show_layout", None, PTR, PTR)(ctx.ptr, layout.ptr)
    for start, end, target in link_ranges:
        rects = {}
        data = layout.get_text().encode("utf-8")
        for index in range(start, end):
            if data[index] & 0xC0 == 0x80:
                continue
            rect = layout.index_to_pos(index)
            rx, ry, rw, rh = (v / 1024 for v in (rect.x, rect.y, rect.width, rect.height))
            if rw < 0:
                rx, rw = rx + rw, -rw
            if ry not in rects:
                rects[ry] = [rx, ry, rx + rw, rh]
            else:
                rects[ry][0] = min(rects[ry][0], rx)
                rects[ry][2] = max(rects[ry][2], rx + rw)
        safe_target = target.replace("\\", "\\\\").replace("'", "\\'")
        for rx, ry, end_x, rh in rects.values():
            if end_x > rx:
                attributes = f"uri='{safe_target}' rect=[{x+rx:.3f} {y+ry:.3f} {end_x-rx:.3f} {rh:.3f}]"
                ctx.tag_begin("Link", attributes)
                ctx.tag_end("Link")


def render_page(ctx, blocks, size, draw=False):
    y = TOP
    for kind, level, payload in blocks:
        if kind == "table":
            count = len(payload[0])
            ratios = {2: [0.31, 0.69], 3: [0.24, 0.39, 0.37], 4: [0.18, 0.24, 0.27, 0.31]}.get(count, [1 / count] * count)
            widths = [CONTENT_WIDTH * ratio for ratio in ratios]
            cell_size = size * (0.85 if count >= 4 else 0.90)
            for row_index, row in enumerate(payload):
                layouts = [make_layout(ctx, data[0], widths[j] - 14, cell_size, bold=row_index == 0) for j, data in enumerate(row)]
                row_height = max(height(layout) for layout in layouts) + 12
                x = LEFT
                if draw:
                    ctx.set_source_rgb(*( (0.88, 0.93, 0.94) if row_index == 0 else (0.965, 0.974, 0.977) ))
                    ctx.rectangle(LEFT, y, CONTENT_WIDTH, row_height)
                    ctx.fill()
                    for j, layout in enumerate(layouts):
                        show(ctx, layout, x + 7, y + 5, row[j][2])
                        x += widths[j]
                    ctx.set_source_rgb(0.77, 0.82, 0.84)
                    ctx.set_line_width(0.4)
                    ctx.move_to(LEFT, y + row_height)
                    ctx.line_to(WIDTH - RIGHT, y + row_height)
                    ctx.stroke()
                y += row_height
            y += 11
            continue

        markup, plain, links = payload
        x, width, gap = LEFT, CONTENT_WIDTH, 7
        font_size, mono, bold = size, False, False
        if kind == "heading":
            font_size, bold, gap = (20 if level == 1 else 17.0), True, 15
        elif kind == "list":
            markup = "•  " + markup
            links = [(a + len('•  '.encode()), b + len('•  '.encode()), target) for a, b, target in links]
            font_size, gap = size * 0.96, 5
        elif kind == "code":
            mono, font_size, gap = True, size * 0.79, 12
            x, width = LEFT + 10, CONTENT_WIDTH - 20
        layout = make_layout(ctx, markup, width, font_size, mono=mono, bold=bold)
        h = height(layout)
        if draw:
            if kind == "code":
                ctx.set_source_rgb(0.95, 0.965, 0.97)
                ctx.rectangle(LEFT, y - 3, CONTENT_WIDTH, h + 10)
                ctx.fill()
            show(ctx, layout, x, y, links)
        y += h + gap
    return y


def create_pdf(pages, path):
    # Preflight every page on an in-memory surface before writing any PDF.
    probe = Context(Surface())
    sizes = []
    blocks_by_page = [parse_blocks(page) for page in pages]
    for number, blocks in enumerate(blocks_by_page, 1):
        selected = None
        for size in [11.2, 11.0, 10.8, 10.6, 10.4, 10.2]:
            end = render_page(probe, blocks, size)
            if end <= BOTTOM:
                selected = (size, end)
                break
        if selected is None:
            raise ValueError(f"Page {number} does not fit at the minimum body font; edit its content.")
        sizes.append(selected)

    surface = Surface(path, WIDTH, HEIGHT)
    metadata = native(CL, "cairo_pdf_surface_set_metadata", None, PTR, INT, STRING)
    metadata(surface.ptr, 0, b"Sheets47: engine options and the calculator application")
    metadata(surface.ptr, 2, "Decision report · 12 November 2026".encode())
    ctx = Context(surface)
    for number, (blocks, (size, end)) in enumerate(zip(blocks_by_page, sizes), 1):
        show(ctx, make_layout(ctx, "SHEETS47  /  ENGINE &amp; APPLICATION DECISION REPORT", CONTENT_WIDTH, 8, bold=True), LEFT, 29)
        ctx.set_source_rgb(0.16, 0.40, 0.47)
        ctx.set_line_width(1.1)
        ctx.move_to(LEFT, 48)
        ctx.line_to(WIDTH - RIGHT, 48)
        ctx.stroke()
        render_page(ctx, blocks, size, draw=True)
        footer = f"12 November 2026   ·   Research and proposal                                       {number:02d} / {len(pages):02d}"
        show(ctx, make_layout(ctx, footer, CONTENT_WIDTH, 8), LEFT, HEIGHT - 30)
        ctx.show_page()
        print(f"Page {number:02d}: body {size:.1f} pt, content ends {end:.1f} pt (limit {BOTTOM:.0f})")
    surface.finish()


def create_html(pages, path):
    sections = []
    for number, page in enumerate(pages, 1):
        sections.append(f'<section class="page"><header>SHEETS47 / ENGINE &amp; APPLICATION DECISION REPORT</header><article>{MD.render(page)}</article><footer>12 November 2026 · Research and proposal <span>{number:02d} / {len(pages):02d}</span></footer></section>')
    template = '''<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>Sheets47: engine options and the calculator application</title>
<style>
*{box-sizing:border-box}body{margin:0;background:#e9eef0;color:#192630;font:11.2pt/1.18 "Liberation Serif","Times New Roman",serif}
.page{background:white;width:210mm;min-height:297mm;margin:18px auto;padding:10mm 16mm 15mm;position:relative;box-shadow:0 2px 12px #0001}
header{font-size:8pt;font-weight:bold;letter-spacing:.06em;color:#285a69;border-bottom:1px solid #285a69;padding-bottom:5mm;margin-bottom:5mm}
footer{font-size:8pt;color:#51636c;border-top:1px solid #dce4e7;margin-top:7mm;padding-top:3mm;position:absolute;bottom:9mm;left:16mm;right:16mm}footer span{float:right}
h1{font-size:20pt;line-height:1.2;margin:0 0 14px}h2{font-size:17pt;line-height:1.2;margin:0 0 16px}p{margin:0 0 10px}
table{width:100%;border-collapse:collapse;font-size:9.3pt;margin:10px 0 13px;table-layout:fixed}th,td{text-align:left;vertical-align:top;padding:7px;border-bottom:1px solid #ccd7dc;overflow-wrap:anywhere}th{background:#e1eef0}td{background:#f6f9fa}
a{color:#15546d;text-decoration-thickness:.6px;overflow-wrap:anywhere}pre{background:#f1f6f7;padding:10px;font-size:8.5pt;line-height:1.3;white-space:pre-wrap}code{font-family:"DejaVu Sans Mono",monospace;font-size:.9em}ul{padding-left:20px;margin:7px 0}li{margin-bottom:5px}
@page{size:A4;margin:0}@media print{body{background:white}.page{margin:0;box-shadow:none;break-after:page;min-height:297mm}.page:last-child{break-after:auto}a{color:#15546d}}
@media screen and (max-width:820px){.page{width:100%;min-height:0;padding:24px;margin:0 0 16px}footer{position:static}pre{overflow:auto}}
</style></head><body>''' + "\n".join(sections) + "</body></html>\n"
    path.write_text(template)


def main():
    source = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else DEFAULT
    text = source.read_text()
    pages = [page.strip() for page in text.split("<!-- pagebreak -->")]
    if len(pages) < 12 or any(not page for page in pages):
        raise ValueError("Expected at least twelve nonempty report pages.")
    create_pdf(pages, source.with_suffix(".pdf"))
    create_html(pages, source.with_suffix(".html"))
    print(f"Wrote {len(pages)} pages in PDF and HTML beside {source.name}")


if __name__ == "__main__":
    main()
