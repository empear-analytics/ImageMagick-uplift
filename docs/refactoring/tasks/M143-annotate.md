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
| annotate-freetype-private.h (new) | - | 8.05 | OH: the FreeType helpers, Raqm block included |
| annotate-freetype-render-private.h (new) | - | 9.18 | OH: the glyph loader and RenderFreetype |
| annotate-render-private.h (new) | - | 8.46 | OH: RenderPostscript and RenderX11 |

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

## Oracle

Whole catalogue at `484300881` (working-tree candidate): 16,005 cases, 1 diverged, the known
`gaps/c5651ffad8` logcfg tie-break. Each step ran its function's cases: RenderType 218,
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
- **Reach.** No case executes `RenderPostscript` (the non-FreeType path). Its carve rests on
  the whole-object IDENTICAL result. Further work on it is guard-level (E, G, P).

## Next

- **annotate-freetype-render-private.h (9.18):** RenderFreetype (cc 17, 149 lines).
- **annotate-render-private.h (8.46):** RenderPostscript is unreached. The argument findings
  are on two statics with 5 parameters each, which Recipe B could bundle.
- **annotate-freetype-private.h (8.05):** RasterizeGlyphRow (cc 12, 72 lines) and
  ComplexTextLayout (6 arguments) are reachable. ComplexRaqmTextLayout and
  ContainsRaqmParagraphSeparator are Raqm-only, which this build does not compile.
