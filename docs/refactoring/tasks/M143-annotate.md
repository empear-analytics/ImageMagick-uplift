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


## 2026-11-26: Recipe OH chains on stream.c, draw.c, utility.c and profile.c

Sweep at `ed41bddf6` (cs-mcp 1.5.16): MagickCore 267 files, mean 9.66; campaign B had 49 `.c`
files below 10. The four files below took Recipe OH's mid-file form, each chain stub-probed
first (scratch copies under `build-oracle/probe/`).

| File | Before | After | Headers (score) |
| --- | --- | --- | --- |
| stream.c | 7.32 | **10.0** | stream-row (8.03), stream-quantum (7.32), stream-map (8.81), stream-pixels (8.28) |
| draw.c | 6.63 | **9.68** | draw-mvg-prepare (9.09), draw-mvg-trace (7.72), draw-mvg-push (8.54), draw-mvg-attribute (8.03), draw-mvg-keyword (8.28), draw-mvg-render (8.47), draw-path-trace (9.38) |
| utility.c | 7.84 | 8.34 | utility-path (8.81), utility-template (10.0), utility-list (9.68), utility-expand (9.38), utility-base64 (10.0, added later to clear the Primitive Obsession the carves had exposed) |
| profile.c | 8.19 | 8.71 | profile-lcms (9.68) |

Every header scores at or above its file's score before the chain. What is left: draw.c,
DrawClipPath's arguments (`MagickExport`); utility.c, GetExecutionPath, ListFiles and
GetPathTemplate (platform branches), Primitive Obsession and String Heavy Function Arguments;
profile.c, ProfileImage (cc 29, 187 lines, LCMS branch with `ThrowProfileException`, a macro
with a hidden `return` naming four locals).

### How the chains were ordered

- **The pre-commit gate holds a new header to the file's score just before the step**, not to
  the chain's starting score that the 2026-11-25 ruling names. stream.c's first order (map
  writers alone, then the dispatchers) left the dispatcher header at 8.28 against the file's
  8.54 at that step, and the gate refused. The chain was redone so that larger runs leave
  first, while the file is still low, and each header is then **carved again** (its leading
  run into a new header included directly above it; bar = the carved header's score). No
  stream.c, utility.c or profile.c commit skipped the gate.
- **One disclosed `--no-verify`** (`5f840e730`, draw.c's first step, under the 2026-11-25
  ruling): every probed first step dropped draw.c (5.72 to 6.49), as shrinking it brings
  back its hidden Code Duplication. The chosen order drops once (6.63 -> 6.49) and then rises
  at every step: 6.69, 6.69, 8.28, 9.68.
- **Carving more can lower a file.** profile.c: the LCMS block alone gives 8.71; adding the
  sRGB/transform statics or the 8BIM/XMP statics scored 8.19 or 8.65, because ProfileImage's
  Complex and Large Method weigh more in a smaller file. utility.c needed its runs in a set
  order (path, template, list, expand) to avoid an intermediate 7.61.

### Evidence worth knowing

- **Moved asserts change the object only in cold paths.** Each assert in a moved run reports
  the header's name, so the unit gains one string and every `l_.str.N` label renumbers. With
  the labels normalised (`l_.str` and `l_.str.N` alike), stream.c's, profile.c's and
  utility.c's objects were identical, and draw.c's and stream.c's StreamImagePixels differed
  only in `.cold.*` assertion blocks choosing other outlined call sequences (as `48d891a7d`).
- **Header-only carves** (a header's leading run into a new header) were checked by
  preprocessing the `.c` file against HEAD's headers and the working tree's
  (`tools/pp_identical.sh` has no `HDRS=1`; a scratch variant did it) and
  `HDRS=1 tools/obj_identical.sh`.
- `Makefile.in`'s dist list puts some names one per line (`\tMagickCore/draw-core-private.h \`);
  the new header goes on its own line there.
- Oracle runs per step: StreamImagePixels 261, ProfileImage 87, GetPathComponent 16,009 (2
  diverged, the known `gaps/c5651ffad8` logcfg tie-break and the `resize/6ee45e9cfa`
  write-mask flake, 0 alone), ListFiles 43, ExpandFilenames 12,905, DrawImage and
  RenderMVGContent 476, DrawClippingMask 4, DrawCompositeMask 7: all 0 diverged otherwise.

### Gate at the tip

Whole catalogue: 16,010 cases, 1 diverged, the known `gaps/c5651ffad8` logcfg tie-break (as
at every M143 tip); `matrix/f2913328b4` settled (the load-sensitive matrix-on-disk timeout on
the baseline). `tools/cxx_warnings.sh --all`: OK, 267 files. `check_trailers`: 16 refactor
commits, all valid.

### Next

- The new headers below 9: stream-quantum (7.32), stream-row (8.03), stream-pixels (8.28),
  draw-mvg-trace (7.72), draw-mvg-attribute (8.03), draw-mvg-keyword (8.28): mostly Code
  Duplication between per-type or per-keyword siblings, per-pixel in the stream headers (the
  no-slowdown rule applies).
- module.c (6.63) and image-view.c (6.84): function-level findings, mostly in code no case
  reaches (guard-level recipes only).

## 2026-11-26, later: cache.c out of the red, color.c and pixel.c

| File | Before | After | Headers (score) |
| --- | --- | --- | --- |
| cache.c | 3.52 | **6.92** | cache-clone (7.17), cache-image (9.09), cache-virtual (7.71), cache-open (7.80), cache-read (8.33), cache-nexus (8.77), cache-write (8.37) |
| color.c | 9.09 | 9.68 | color-parse (9.38) |
| pixel.c | 8.74 | 9.31 | pixel-interpolate-channel (9.38), pixel-interpolate-channels (9.09), pixel-interpolate-info (9.38) |

cache.c's M141 plateau note held for Recipe S and Form B; the mid-file form reaches the 75
statics between its 18 external definitions, and every step held or raised the file (3.52,
3.52, 3.91, 4.47, 5.18, 5.93, 6.92). Left in cache.c: six Complex Methods in its
`MagickPrivate` functions (GetVirtualPixelCacheNexus, QueueAuthenticPixelCacheNexus,
SyncAuthenticPixelCacheNexus and others) and four functions' arguments.

- **OpenMP regions moved as text** (ClonePixelCacheChannels, ClonePixelCacheRepository,
  SetCacheAlphaChannel): besides the oracle build, the file was compiled with gcc-16 and the
  `omp-cand` tree's flags (`-fopenmp -O2`), HEAD against the working tree; with string
  labels normalised the disassembly was identical at every step. On this Mac, gcc-16 needs
  `PATH=/usr/bin:...` ahead of Anaconda, whose x86_64 `as` breaks the assembler step.
- **Form-feed lines** at a run's edges stay in the `.c` file (cache.c has `^L` pairs between
  sections).
- Unbuilt blocks moved verbatim with their pairs (owner, 2026-11-20): MAGICKCORE_HAVE_LINUX_SENDFILE,
  MAGICKCORE_HAVE_POSIX_FALLOCATE, MAGICKCORE_OPENCL_SUPPORT.
- Oracle per step: ClipPixelCacheNexus 41, ClonePixelCacheRepository 8477, GetImagePixelCache
  15129, OpenPixelCache 15129, SetPixelCacheNexusPixels 14864, all 0 diverged; the other steps
  were object-IDENTICAL in both builds (guard level).

Not taken: blob.c (2.71): OH reaches only 2.77 (its debt is in 86 external functions; M128 and
M130 record why splits are blocked). log.c (8.81): its largest run alone scores 8.54, below the
file; it needs that run cut in two. module.c and image-view.c: unreached or unbuilt code.

Gate at this tip: whole catalogue 16,010 cases, 1 diverged (the known `gaps/c5651ffad8`),
`matrix/f2913328b4` settled; `cxx_warnings --all` OK (267 files); `check_trailers` 27 refactor
commits, all valid.

### log.c

| File | Before | After | Headers (score) |
| --- | --- | --- | --- |
| log.c | 8.81 | 9.68 | log-write (10.0), log-event (9.09), log-load (9.38) |

The writer-and-translator run scored 8.54 as one header; cut at GetModuleBasename it gives
10.0 and 9.09, and the order writers, translators, loader keeps the file at 8.81, 9.38,
9.68. `log-private.h` is listed only under MAGICKCORE_NOINST_HDRS, so the first header went
into MAGICKCORE_BASE_SRCS after `log.h`. The LoadLogCache step's oracle (16,009 cases)
showed only `gaps/c5651ffad8`, the known logcfg tie-break; its object is IDENTICAL.

### Probed, not taken

- identify.c (9.38): its one external function is last; any carve of the leading run scores
  the file 9.28 to 9.30.
- The MVG keyword headers' Code Duplication (draw-mvg-attribute 8.03, -keyword 8.28, -push
  8.54): a helper for the repeated `ParseCommandOption` block (10 handlers) leaves the
  findings almost unchanged (push 8.54 -> 8.81, the others equal). The clones are the
  state-binding prologue and write-back that every Recipe KH handler repeats.
- `FormatSanitizedDelegateOption` (delegate-private.h 9.68): its callers are in `coders/`.
- ProfileImage: Recipe MB refuses its blocks (ThrowProfileException hides a `return`).
- The PR's CodeScene gate counts advisory findings (Primitive Obsession) only in files that
  existed before; several new headers carry it and the gate passes.

### cache.c, function-level steps

| Step | Recipe | cache.c |
| --- | --- | --- |
| SetPixelCacheMethods: four NULL-tested handler copies into SetPixelCacheGetMethods (cc 12 -> 8) | E | 6.92 -> 7.15 |
| SetPixelCacheVirtualMethod: the background case into SetBackgroundVirtualPixels | E | 7.15 -> 7.39 |
| SyncAuthenticPixelCacheNexus: the mask block into ApplyPixelCacheNexusMasks (cc 14 -> 10, Bumpy Road gone, still Complex) | W | 7.39 -> 7.52 |
| QueueAuthenticPixelCacheNexus: the mask argument into HasNexusMaskChannels (cc 10 -> 8) | P | 7.52 -> 7.74 |

The W and P steps touch functions that run once per row; both objects (clang oracle build
and gcc-16 OpenMP -O2) stayed identical, so no timing. The W commit's message first said
SyncAuthenticPixelCacheNexus left Complex Method; it did not (cc 10). The two unpushed
commits were replayed with the corrected message (`--no-verify` on the replay only: the
trees are the ones the gate passed).

Left in cache.c: GetVirtualPixelCacheNexus (cc 35, 149 lines), SyncAuthenticPixelCacheNexus
(cc 10), GetAuthenticOpenCLBuffer (OpenCL, not built here), four `MagickPrivate` signatures'
arguments. Probed for GetVirtualPixelCacheNexus: its colour `switch` as one helper is itself
a Complex Method (cc 11) and lowers the file (7.71); its four constant-fill cases differ in two
values, but Gray's `QuantumRange/2` is not a literal, which Recipe SC requires, and the
helper would change per-row code (timing). With the switch, the metacontent block and the
mask predicate moved, the stub probe reaches 8.02 (cc 22 left).
Gate at this tip: whole catalogue 16,010 cases, 1 diverged (the known `gaps/c5651ffad8`),
`matrix/f2913328b4` settled; `cxx_warnings --all` OK (267 files); `check_trailers` 35
refactor commits, all valid.

### resource.c, and the near-10 files checked

| File | Before | After | Steps |
| --- | --- | --- | --- |
| resource.c | 8.97 | 9.38 | OH (resource-unique-private.h, 10.0), then SD with Recipe Z step 7's targets struct on RelinquishMagickResource (Map, Memory, Time and default groups into RelinquishMagickResourceTail) |

SD alone cleared the Large Method but brought *Lines of Code in a Single File* (8.81), so the
carve went first (8.97 held), then SD (9.38). `tools/switch_split_check.py` confirms the
moved groups token for token but reports the new targets typedef and its `const` fill as a
difference outside the switch: it was written for the unreached form, where the targets
typedef already existed. RelinquishMagickResource is reached (15,132 cases, 0 diverged), so
the oracle is the evidence. Left: *Overall Code Complexity*, held by the external functions
(every remaining OH run probed: 9.38).

Checked and not taken (each would need more than one step or a ruling):
- random.c (9.45): GenerateEntropicChaos' time block holds a `CLOCK_REALTIME_HR` branch that no
  available build compiles.
- policy-path.c (9.53): IsPathContainsSymlink's remaining blocks need five parameters.
- magick.c (9.61): both of GetMagickInfo's blocks hold `MAGICKCORE_BUILD_MODULES` branches; no
  modules build here.
- configure-paths.c (9.18): GetConfigurePaths is made of platform branches.
- token-private.h (9.09): ConvertMacRomanToUTF8, the known clone trap (M141).
- type.c (9.09): *Overall Code Complexity* at lizard mean 4.1; about five small Recipe E
  helpers would bring it under the ~3.9 bar (owner 2026-11-16 allows such steps).
Gate at this tip: whole catalogue 16,010 cases, 1 diverged (the known `gaps/c5651ffad8`),
`matrix/f2913328b4` settled; `cxx_warnings --all` OK (267 files); `check_trailers` 37
refactor commits, all valid.

### cache-open-private.h, and type.c probed

| File | Before | After | Steps |
| --- | --- | --- | --- |
| cache-open-private.h | 7.80 | 7.90 | E (OpenPixelCache's closing debug log into LogDiskPixelCacheOpen, as LogMemoryPixelCacheOpen), W (the geometry-limit check into CheckPixelCacheGeometryLimits; its exception names the helper, 2026-11-12 ruling) |

OpenPixelCache stays a Complex and Large Method (cc 33). Its memory-cache and mapped-disk
blocks fit Recipe RV (each returns a Create*PixelCache call or falls through) but read six
inputs (status, length, cache_info, open_info, the by-value source_info and the result), so
they need RV's struct widening (c); not taken this round.

type.c (9.09) is a trap for *Overall Code Complexity*: the shared IsListedTypeInfo predicate
for GetTypeInfoList and GetTypeList (Recipe P, two sites) crosses *Lines of Code in a Single
File* (8.54); a small OH carve first makes room but removes simple statics, so lizard's mean
goes back to 4.1 (9.09); the large loader run exposes hidden Code Duplication (8.54).
Gate at this tip: whole catalogue 16,010 cases, 1 diverged (the known `gaps/c5651ffad8`),
`matrix/f2913328b4` settled; `cxx_warnings --all` OK (267 files); `check_trailers` 39
refactor commits, all valid.

### blob.c out of the red: a Recipe S chain

| File | Score | Holds |
| --- | --- | --- |
| blob.c | 2.71 -> **8.28** | blob management (Attach, Clone, Destroy, Detach, Duplicate, Map/Unmap, the properties) |
| blob-info-private.h | no score (no function; `noscore_headers.txt`) | the define block and FileInfo, `_BlobInfo`, `_CustomStreamInfo` |
| blob-io-private.h | 10.0 | ThrowBlobException, WriteBlobStream (shared, `static inline`) |
| blob-custom.c | 10.0 | the custom-stream accessors |
| blob-image.c | 4.74 | the image/blob conversions (BlobToImage, ImageToBlob, PingBlob...) |
| blob-read.c | 7.37 | the typed readers |
| blob-write.c | 8.28 | the typed writers |
| blob-query.c | 5.96 | EOFBlob, ErrorBlob, GetBlobSize, IsBlobSeekable, TellBlob, SeekBlob... |
| blob-stream.c | 3.75 | OpenBlob, CloseBlob, ReadBlob, WriteBlob, SetBlobExtent, FileToImage, InjectImageBlob |

Why earlier rounds stopped (M128, M130): the private structures blocked splits, and fixing
functions in place barely moves the score (stub probes: the 7 worst emptied 2.81, 20 emptied
4.8, all 28 emptied 7.05). The 2026-11-16 ruling (typedefs, macros and static helpers shared
through one non-installed private header) lifts the first; the split bar (each product at least
the original's 2.71) makes moving debt out the lever. The first step needed a Recipe OH
amendment (2026-11-26, mandate): blob.c's define block holds `#include <io.h>` under
`#if defined(__OS2__)`.

Every step: whole catalogue 16,010 cases, only the known `gaps/c5651ffad8`; guards --combined
and --calls OK; nm shows each moved symbol in its new object only. blob-stream.c's
-Wimplicit-fallthrough reports under clang++ are upstream's `magick_fallthrough;` cases moved
(16 before in blob.c, 16 after in blob-stream.c, 0 left in blob.c).

Next for this family: blob-stream.c (3.75) can be split again (OpenBlob/CloseBlob from the raw
I/O, sharing SyncBlob and IsWriteBlobMode through a header that only those includers name),
then function-level work on the products; ReadBlob, WriteBlob and the typed readers/writers are
per-byte or per-row paths (timing).

