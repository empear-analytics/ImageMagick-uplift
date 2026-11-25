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
| annotate-render-private.h (new) | - | 9.68 | OH: RenderPostscript and RenderX11 (8.46); B (`RenderX11`), E (`LogPostscriptFont`), N (`TransformPostscriptImage`), N (`CompositePostscriptFill`, an OpenMP region), FB, FI. Ceiling: RenderPostscript's own 5 arguments |

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

- **RenderPostscript's fill pass, an OpenMP region, moved by Recipe N.** Only the body under
  the `fill.alpha` test moved; the whole pass as one helper had scored 9.01 in a probe. The
  ruling's write set, read by hand: status is still `shared(status)`; y, x and q are private;
  `sync` and `fill_color` are shared as before (see the data-race note). In gcc-16 -O2 OpenMP
  builds the outlined region has the same 128 instructions; only labels and two offsets into
  the captured-variable block differ. `tools/omp_compare.sh` at 1 and 4 threads on
  `tools/omp_cases/annotate-postscript.txt` (text images above the one-thread limit): 0
  diverged, A/A 0. FB on `transform` and FI on `fill` (both object-IDENTICAL) then took
  RenderPostscript from 75 to under 70 lines: 9.68.

## Oracle

Whole catalogue at `484300881` (working-tree candidate): 16,005 cases, 1 diverged, the known
`gaps/c5651ffad8` logcfg tie-break. Again at the TransformPostscriptImage step, with the five
PostScript-font cases added: 16,010 cases, 1 diverged, the same case. Again after the
xml-tree.c chain: 16,010 cases, 1 diverged, the same case. Each step ran its function's cases: RenderType 218,
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
- **`tools/omp_compare.sh` needs absolute paths.** It runs each case after cd into a temporary
  directory. With relative BASE and CAND paths both sides fail, and the two "not found"
  messages differ by path, so base against candidate reports every run as diverged while an
  A/A run passes.
- **Possible upstream data race (report, not fixed).** In RenderPostscript's fill pass,
  `MagickBooleanType sync` is declared outside the `#pragma omp parallel for` and is not
  private, yet every thread assigns it from SyncCacheViewAuthenticPixels and tests it. The same
  holds for `PixelInfo fill_color`, which every thread fills through GetFillColor.

## Next

- **annotate-freetype-render-private.h (9.21).** RenderFreetype is cc 13 and 143 lines (about
  70 of them declarations). A realistic Z probe moved the glyph loop and its prelude into
  RenderFreetypeGlyphs (twelve inputs in an FB struct, glyph, origin, status and
  last_glyph_id as const targets, the temporaries declared in the helper). It scored 8.99:
  the helper is cc 9 and 87 lines, and RenderFreetype is still 100 lines. Reaching 10 needs a
  stub-probed chain through intermediate drops (the long-flat-chain lane of 2026-11-16), and
  after it ComplexTextLayout's B step. B probed at 9.30 -> 9.59 for its own header but 9.21 ->
  9.20 (one-line block form) or 9.16 (two-line form) for this one.
- **annotate-freetype-private.h (9.30).** RasterizeGlyphRow (cc 12, 72 lines) is per pixel,
  so its steps need callgrind Ir.
- **annotate-raqm-private.h (8.38).** Raqm-only, not compiled here: a plateau unless a
  Raqm build becomes available.

## xml-tree.c to 10.0

| File | Before | After | Steps |
| --- | --- | --- | --- |
| xml-tree.c | 9.09 | 10.0 | OH x3 (mid-file) |
| xml-tree-text-private.h (new) | - | 9.38 | WriteUTF8Bytes to the EntityCheckInfo typedef |
| xml-tree-declarations-private.h (new) | - | 9.09 | ValidateEntities to the entity declarations |
| xml-tree-markup-private.h (new) | - | 9.68 | ParseDoctypeDeclaration to ParseXMLTreeTags |

- **Finding the cut by search.** The parser statics run from GetXMLTreeTag to NewXMLTree.
  No two-header cut kept both headers at 9.09 or above: all 62 function starts in the
  stretch were tried. Every header holding lines 1291-1560 whole with the tail was flagged
  for *Overall Code Complexity*. A three-header search with the first cut at 1291 found
  seven partitions. The one taken (cuts at ValidateEntities and ParseDoctypeDeclaration)
  rises at every step, 9.09 -> 9.09 -> 9.38 -> 10.0, so no step needed `--no-verify`.
- The NewXMLTree banner stood at the top of the first run and moved to directly above
  NewXMLTree. A form-feed line at the run's edge stays in xml-tree.c. The markup run names
  the kept `sentinel`. All three carves were pp- and object-IDENTICAL.
- **What holds the headers below 10.** They carry only file-level ratio findings:
  - *Primitive Obsession*: CodeScene warns at 60% primitive arguments, and counts more as
    primitive than a plain type reading does.
  - *String Heavy Function Arguments*: 52% string arguments in the text header.
  - *Overall Code Complexity*: in the declarations header.
  A probe that rewrote signatures to one struct argument, the widest functions first,
  cleared *Primitive Obsession* in the markup header only at 25 of its 45 functions (20 was
  not enough). Each would be a Recipe B step, and only where the arguments travel together.

## Probed elsewhere, not taken

- xml-tree.c: first probed as not taken (two-header cuts only); taken later, see above.

## Owner answers (2026-11-25)

Before a long unattended run, the owner answered four questions (recorded in AGENTS.md):
- A stub-probed chain may take intermediate drops with `--no-verify`, each disclosed.
- The bar for a header carved out of an already-carved file is the file's score before its
  first carve (draw-primitive.c 3.92).
- The ratio findings (*Primitive Obsession*, *String Heavy Function Arguments*) are worked
  toward 10 with Recipe B bundles.
- `build-oracle/mull-*` may be deleted when disk runs low.

## Afternoon of 2026-11-25: the FreeType headers to 10.0, property.c to 9.68

| File | Before | After | Steps |
| --- | --- | --- | --- |
| annotate-freetype-render-private.h | 9.38 | 10.0 | SN, N (`RenderFreetypeGlyphs`), FI, H, DI, E; then D, O, E to make room for B |
| annotate-freetype-private.h | 9.30 | 10.0 | B (`ComplexTextLayout`), DB, E (`SyncGlyphRow`), N (`CompositeGlyphPixel`), JB (`FetchGlyphPixel`) |
| property.c | 7.6 | 9.68 | OH x9 (mid-file), E (opening block), E (`SetImagePropertyAttribute`) |
| property-iptc-private.h (new) | - | 10.0 | OH |
| property-8bim-private.h (new) | - | 9.38 | OH (Primitive Obsession, String Heavy Function Arguments) |
| property-exif-private.h (new) | - | 9.09 | OH (Overall Code Complexity, Primitive Obsession) |
| property-icc-private.h (new) | - | 10.0 | OH (the whole LCMS block) |
| property-xmp-private.h (new) | - | 10.0 | OH |
| property-clippath-private.h (new) | - | 8.73 | OH (8.41), CS (`TracePSClippathRecord`), CS (`AddSVGClippathKnot`) |
| property-profile-private.h (new) | - | 10.0 | OH |
| property-magick-private.h (new) | - | 10.0 | OH (the GetMagickProperty letter handlers, `#if 0` blocks verbatim) |
| property-interpret-private.h (new) | - | 9.09 | OH (Overall Code Complexity, Primitive Obsession) |

Every property header scores at or above property.c's 7.6 before the chain (the 2026-11-25 bar).

### New recipes and amendments

- **Recipe SN** (sink constant writes to their first reader): SZ for `v=c;` and `v.m=c;`,
  allowed to cross an unbuilt `#if` branch whose text is read, not edited.
- **Recipe H**, guarded form: `#if !defined(M)` / `#define M` / `#endif` moves as one run.
- **Recipe JB** (join a block to the struct fill after it and extract it reading the
  struct): the per-pixel fetch before `RasterizeGlyphRow`'s `GlyphPixel` block.

### Evidence worth knowing

- **One disclosed `--no-verify`** (`6c3d509ad`, Recipe D on the face release): with few
  functions, any one new helper raised *Overall Code Complexity* in the render header
  (10.0 -> 9.38), any two cleared it. Every combination of the chain was scored first;
  the next commit restored 10.0.
- **Per-pixel steps were timed.** No valgrind on this Mac, so the rule's wall-clock form:
  gcc-16 -O2 -fopenmp builds (`build-oracle/omp-t-*`), 16 rotated 900-point `-annotate`
  draws, 41 alternating rounds, both role orders, beside A/A. N (`CompositeGlyphPixel`):
  1.0001 / 1.0013 against A/A 1.0002 (1 thread), 0.9963 / 0.9998 against 0.9966 (4
  threads). JB: 0.9874 / 1.0082 (A/A 0.9977), 0.9944 / 1.0131 (A/A 1.0012). DB and E were
  object-identical under gcc too. omp_compare at 1 and 4 threads: 0 diverged.
- **`DI` on arm64.** `tools/obj_sorted.sh` knows x86 stores only and has no `HDRS=1`; the
  disassembly was compared by hand (one `str wzr, [sp, #0x14c]` moved, sorted lists equal).

### Tooling

- `tools/omp_cases/annotate-freetype.txt` (new), and `tools/omp_compare.sh run` now sets
  `$root` for case lines. **Defect fixed:** `annotate-postscript.txt`'s third case had an
  unquoted `#00800080`, which `eval` read as a comment, so both builds failed the same way
  and the case compared nothing.

### Reported, not fixed

- `Makefile.in` line 439 and 534 (`am__..._SOURCES_DIST`) hold literal `\n` text between
  file names since the W23 merge of 2026-11-14 (`c6c33b048`). It is the dist-only list, so
  builds are unaffected; `make dist` would see a file named `\n`.
- The `#if 0` fragments `property-set-*-private.h` now sit inside the `void`
  `SetImagePropertyAttribute`; their dead text returns `MagickFalse`, which whoever enables
  one must adjust (as UB's Careful note says of stale names).

### Next

- **annotate-render-private.h (9.68).** RenderPostscript's 5 arguments: one caller is the
  `#else` stub of RenderFreetype in annotate.c, compiled only without FreeType. A
  `--without-freetype` configuration (an available build under the 2026-11-12 ruling)
  would let Recipe B take it (the group matches `X11TextTarget`).
- **property.c (9.68).** GetMagickProperty, cc 9: the decisions sit in the LCMS block that
  uses `WarnNoImageReturn`, a macro with a hidden `return`; needs an EX- or RV-like recipe
  that keeps the macro's text.
- **property-clippath-private.h (8.73).** The SVG record loop by CS makes a twin of
  `TracePSClippathRecord` (*Code Duplication*, probed 8.33). A Recipe CF merge (step 3's
  `const void *` inputs for the two knot functions) would take both, then SD/SR on the
  merged record's case groups (cc 10).
- **property-exif / -interpret / -8bim headers.** Ratio findings: Recipe B bundles
  (owner 2026-11-25), and E steps for *Overall Code Complexity*.
- **token-glob.c (8.42), probed, not taken.** Recipe E step 7 on GlobExpression_'s `{` case
  (every path returns) took the function from cc 46 to 32, but the file fell to 6.99: the
  new GlobBraceAlternatives carries Deep, Nested Complexity, two bumps and a Complex
  Conditional of its own. A chain would also need its alternative-try block (an early
  return, a `continue` and three writes) out; probe that end state first.

## Evening of 2026-11-25: every property header at 10.0

| File | Before | After | Steps |
| --- | --- | --- | --- |
| property-clippath-private.h | 8.73 | 10.0 | O (`ReadPSClippathPoints`), E (`StartSVGClippathSubpath`), CS (`TraceSVGClippathRecord`), CF (`TraceClippathRecord`), IC (member form), SD (`TraceClippathLengthRecord`) |
| property-8bim-private.h | 9.38 | 10.0 | BP x4 (`Cursor8BIM`, address form), IP x3, IP step 7 typedef moves x2, IP x2 (`AttributeInfo8BIM`, `Match8BIM`), BP (`Names8BIM`) |
| property-interpret-private.h | 9.09 | 10.0 | DT x3, E x2, P+DT x2 |
| property-exif-private.h | 9.09 | 10.0 | DT x4, O, BP+IP x3 (`EXIFCursor`), RT (`MatchEXIFMarker`), IP, E x5, N x2 (pass-through) |

property.c stays at 9.68 (GetMagickProperty, below). No `--no-verify` this evening: every
chain was probed first and ordered so that no commit dropped.

### New recipe amendments

- **Recipe IC, step 5 (member form):** a local initialised with `p->m` through a `const`
  struct parameter, read once with nothing between, is replaced by `p->m`. It cleared the
  two record wrappers that CF left repeating their bindings.
- **Recipe BP, step 6 (address form):** a group of addresses of one caller's locals is
  filled once, as a `const` declaration, and shared by every call in that block.
- **Recipe RT, step 1 (compared constants):** the copies' constant may be a literal
  compared with a common expression; the step also serves *Overall Code Complexity*
  (`n-3` off the file's total).

### Evidence worth knowing

- **CF's merged function inlines.** `TraceClippathRecord` is `static inline` and gets its
  method as a designator; the property.c object stayed IDENTICAL, so there is no indirect
  call.
- **Primitive Obsession sits at a knife edge in the exif header.** One helper with two
  primitive arguments (RT's `MatchEXIFMarker(exif,length)`, or `SetEXIFImageProperty`)
  brought it back at 9.09. The cursor bundle first (four steps) gave the room; RT and the
  helpers followed without a drop.
- **Overall Code Complexity by count.** Lizard means matched CodeScene again: the exif
  header cleared between 3.93 and 3.86 (lizard), as the 2026-11-23 note says.
- **Recipe D was not used** for the second `magick-property` hint clearing in the interpret
  header: CodeScene reports no duplication there, so D has no finding to act on. One copy
  went by E.
- **Whole catalogue at the evening tip:** 16,010 cases, 2 diverged: the known
  `gaps/c5651ffad8` (logcfg) and `resize/b41fd3fe32` (rose write-mask `-scale 150%`), which
  passed three times alone with `--filter resize/b41fd3fe32 --no-cache`: the write-mask
  load flake M123 recorded for `resize/6ee45e9cfa`. No property code runs in it.
- The oracle runs on `IsNonEmptyImage`, `LogInterpretImageProperties` and
  `GetEXIFProperty` take 10 to 12 minutes each (12,000 to 15,000 cases).

### Next

- **property.c (9.68):** GetMagickProperty cc 9, unchanged (see the afternoon's Next).
- **annotate-render-private.h (9.68):** RenderPostscript, unchanged.
- **token-glob.c, utility.c (7.84), profile.c (8.19), stream.c (7.32), module.c (6.63):**
  the campaign's other low files; not started this evening.

