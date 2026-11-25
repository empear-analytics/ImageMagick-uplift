# M143 - 2026-11-25, campaign B, annotate.c

Branch `campaign/M143`, on `main` after #132. Scores are from cs-mcp 1.5.16 through
`tools/ch.py`. The oracle baseline is `e0c1bbf823`.

## Sweep at the start

MagickCore at `e0c1bbf82`: 267 files, mean 9.63, 2 red (blob.c 2.71, cache.c 3.52, both
recorded plateaus). Of campaign B's 50 files below 10, annotate.c (5.86) was the lowest not
already recorded as a plateau. Earlier rounds had not used Recipe OH on it: the mid-file form
needed the owner's general reading of mid-file includes, given in W50 ("mid-file includes
allowed in any file").

## Scores

| File | Before | After | Steps |
| --- | --- | --- | --- |
| annotate.c | 5.86 | 9.68 | OH x4 (mid-file), Z, E |
| annotate-type-private.h (new) | - | 10.0 | OH (8.90: the type lookup, RenderType, the density parsers), then RV (`RenderTypeByFont`) |
| annotate-freetype-private.h (new) | - | 9.30 | OH: the FreeType helpers, Raqm block included (8.05); OH again: the Raqm block to its own header |
| annotate-raqm-private.h (new) | - | 8.38 | OH: the Raqm text layout (not compiled here) |
| annotate-freetype-render-private.h (new) | - | 9.21 | OH: the glyph loader and RenderFreetype (9.18); E (`LogFreetypeFont`) |
| annotate-render-private.h (new) | - | 9.07 | OH: RenderPostscript and RenderX11 (8.46); B (`RenderX11`), E (`LogPostscriptFont`), N (`TransformPostscriptImage`) |

annotate.c keeps only *Excess Number of Function Arguments* on `FormatMagickCaption`
(`MagickExport`, 6 arguments), its plateau. Each header scores above the file's 5.86 before
the chain (the 2026-11-18 bar). The headers are campaign B files now and stay on the list
(owner 2026-11-21: the headers are to be uplifted to 10 as well).

## How the chain was found

- **Stub probe first.** Everything after `GetTypeMetrics` (the last `MagickExport`) is
  `static`. One header for all of it left annotate.c at 9.06 and the header at 6.01. Three
  headers (type, FreeType, PostScript) gave the same 9.06 with headers 8.90, 6.97 and 8.46.
- **`OutlineMethods` is a kept item.** The FreeType branch holds `static FT_Outline_Funcs
  OutlineMethods`, a non-`const` file-scope object whose address goes to FreeType, so OH
  cannot move it. The branch went into two headers around it, each included inside the
  `#if defined(MAGICKCORE_FREETYPE_DELEGATE)` branch. That also scored better: the helpers
  8.05 and RenderFreetype 9.18, against 6.97 for one header.
- **Every carve was object-identical.** `pp_identical.sh` (13593 preprocessed lines) and
  `obj_identical.sh` (35568 bytes) printed IDENTICAL for all four. The unbuilt
  `MAGICKCORE_RAQM_DELEGATE` blocks moved verbatim (owner 2026-11-20).
- **AnnotateImage by Recipe Z, not LB.** M138 tried LB step 8 on the loop body, and the
  helper became a Large Method. Z step 7 on the whole `for` loop moves the loop's `continue`
  and `break` with it. The nine inputs go in an FB-form struct, and `metrics` and `status` in
  a `const` targets struct. That gave 9.06 to 9.31: AnnotateImage leaves *Complex Method*,
  and the new `AnnotateTextLines` has no finding. An E step on the opening block (asserts
  and trace log, owner 2026-11-13) then took AnnotateImage from 70 to 67 lines: 9.68.

- **RenderType by Recipe RV (b)+(c).** The `draw_info->font` block's four tries each return
  a call's value and otherwise look the font up, writing `type_info`. Six inputs and that
  pointer go in one `const RenderTypeFont`. The header went from 8.90 to 10.0, and
  *Overall Code Complexity* cleared with it.

- **The Raqm block in its own header.** annotate-freetype-private.h opened with the unbuilt
  Raqm block, which no recipe may touch here. Recipe OH's "carved again" form moved it out
  whole: the helper header went to 9.30 and the Raqm header scored 8.38, both above the 8.05
  bar. annotate.c compiled against HEAD's headers (HDRS=1) is object-IDENTICAL, and its
  preprocessed unit differs only in the `__FILE__` path.
- **New oracle cases for RenderPostscript** (`tools: oracle cases for annotate's PostScript
  fonts`). A font name starting with `^` renders through Ghostscript (allowed since
  2026-10-03). Five gap commands, each byte-stable over three runs, put RenderPostscript and
  ReadPostscriptImage at 5 cases and the fill pass (`SetPostscriptAlphaChannels`) at 4. With
  them, RenderPostscript took E and N at the oracle level.
- **RenderX11 by Recipe B.** Its draw_info, offset and metrics (what it hands to
  XRenderImage) became `const X11TextTarget *`. The typedef stands in annotate.c, ahead of
  the forward declaration. The score was flat, as a step of the probed chain to 9.68.

## Oracle

Whole catalogue at `484300881` (working-tree candidate): 16,005 cases, 1 diverged, the known
`gaps/c5651ffad8` logcfg tie-break. Again at the TransformPostscriptImage step, with the five
PostScript-font cases added: 16,010 cases, 1 diverged, the same case. Each step ran its function's cases: RenderType 218,
RenderFreetype 198, RenderX11 20, AnnotateImage 177, all 0 diverged.

## Traps seen

- **An opening-block helper made only of asserts.** Moving all six opening lines would leave
  the helper's `draw_info` used only in `assert`s, an unused parameter under `-DNDEBUG`
  (CI's `-Wextra -Werror`). Only the `image` asserts and the trace log moved. The oracle
  build's compile line with `-DNDEBUG -Wall -Wextra -Wunused-parameter -Wshadow -Werror` is
  clean.
- **`tools/ci_linux_warnings.sh` does not run on the Mac.** It needs campaign A's Linux
  configurations (`configure failed` for every `ci-*` directory, and a bash
  `cache[@]: unbound variable`).
- **ComplexTextLayout by Recipe B costs the other header.** Bundling draw_info, face and
  flags raised annotate-freetype-private.h to 9.59, but the call's block lengthens
  RenderFreetype, still a Large Method, and its header dropped 9.18 to 9.16. The gate refuses
  that, so the step waits until RenderFreetype is below 70 lines. The call is the `else` body
  of a Raqm `#if`, so it needs B's block form, not a fill at the function's top.
- **Possible upstream data race (report, not fixed).** In RenderPostscript's fill pass,
  `MagickBooleanType sync` is declared outside the `#pragma omp parallel for` and is not
  private, yet every thread assigns it from SyncCacheViewAuthenticPixels and tests it.

## Next

- **annotate-render-private.h (9.07, ceiling 9.68).** RenderPostscript (cc 15, 105 lines)
  clears only if its fill pass leaves. The pass holds an OpenMP parallel loop, so the step
  needs the 2026-11-15 checks (by-hand write set, omp_compare at 1 and 4 threads, with a case
  large enough to start threads). A probe of the whole pass as one N helper scored 9.01 (the
  helper cc 9). Move only the body under the `fill.alpha` test, then take P on the
  file-open condition. RenderPostscript's own 5 arguments stay, because the non-FreeType
  RenderFreetype stub calls it in a branch this build does not compile.
- **annotate-freetype-render-private.h (9.21).** RenderFreetype is cc 13 and 143 lines (after
  LogFreetypeFont), about 70 of them declarations. A probe that moved the glyph loop and the debug log scored 9.08
  (still 114 lines, the loop helper cc 9). It needs a longer chain, after which
  ComplexTextLayout's B step can follow.
- **annotate-freetype-private.h (9.30).** RasterizeGlyphRow (cc 12, 72 lines) is per pixel,
  so its steps need callgrind Ir.
- **annotate-raqm-private.h (8.38).** Raqm-only, not compiled here: a plateau unless a
  Raqm build becomes available.

## Probed elsewhere, not taken

- **xml-tree.c (9.09, Number of Functions).** Its parser statics run from after
  GetXMLTreeTag to NewXMLTree (lines 836-3222). Any OH carve of the whole run leaves
  xml-tree.c at 10.0. But lines 1103-1807 (entity and doctype parsing, lizard mean cc about
  4.7) score 8.81 in any header, from *Overall Code Complexity*, *Primitive Obsession* and
  *String Heavy Function Arguments*. That is below the file's 9.09 bar, and runs must start
  after a kept item, so that stretch cannot be skipped. The leading UTF run alone
  (836-1102, header 9.38) leaves xml-tree.c at 9.09. Taking that stretch's complexity down
  first adds functions to a file already flagged for its function count.
