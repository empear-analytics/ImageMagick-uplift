# M141 - night of 2026-11-20/21, campaign B toward Code Health 10

Branch `campaign/M141`, PR #131. This runs on top of the evening's draw-primitive.c work
(M141-draw-phase2.md). Main was merged in at `0764e2764`, where PLAYBOOK.md conflicted in the
appended recipes and their catalogue rows.

Scores are from cs-mcp 1.5.16 through `tools/ch.py`. The oracle baseline is `8229eff716`.
At every push the whole catalogue ran with a clean `build.sh base <tip>` as the candidate:
15,979 cases, 0 diverged. The reused `build.sh cand` tree diverges only on gaps/c5651ffad8,
the logcfg tie M01 and M90 explained.

## Scores

| File | Before | After | Steps |
| --- | --- | --- | --- |
| draw-primitive.c | 3.67 | 7.22 | E, FB, SM, W, W, D, FB, FI, FB, N, CE, O, W, FI; then the three-header OH chain |
| draw.c | 5.51 | 6.63 | FB, LT, O (ParseMVGClass cleared); E, N, DB, E, D (TraceArcPath cleared, Overall Code Complexity gone); TB (IsValidListChar) |
| constitute.c | 8.81 | 10.0 | CF (Ping/ReadSceneImages), after two new oracle cases for ReadSceneImages |
| constitute-write.c | 9.92 | 10.0 | E, TG |
| policy-private.h | 9.84 | 10.0 | W |
| xml-tree-private.h | 9.39 | 10.0 | W, SD |
| profile-sync.c | 9.38 | 10.0 | OH |
| stream-read.c | 9.68 | 10.0 | OH |
| geometry-meta.c | 8.73 | 9.68 | OH, N, N |
| pixel-export.c, pixel-import.c | 7.32 | 9.68 | OH |
| delegate-invoke.c | 8.28 | 9.68 | OH |
| image-filename.c | 9.38 | 9.68 | OH |
| constitute-pixels.c | 9.2 | 9.68 | OH |
| draw-gradient.c | 8.38 | 9.5 | FB, OH |
| quantum-import.c, quantum-export.c | 8.28 | 9.24, 9.26 | OH |
| matrix.c | 8.81 | 9.09 | E |
| profile.c | 7.41 | 8.19 | SL, D |
| locale.c | 8.91 | 8.96 | Q |
| annotate.c, property.c, constitute-codec.c | 5.83, 7.30, 8.73 | 5.86, 7.34, 8.75 | FB, FI, D |

New headers (Recipe OH):
- From draw-primitive.c, convert 8.22, path 8.23, alpha 8.73, fill 8.97, log 9.46 and render 8.03. Path, fill and render were later carved again (into convert, alpha and log).
- From the other files: pixel-export 7.32, pixel-import 7.32, quantum-import 8.28, quantum-export 8.28, delegate-invoke 9.09, delegate-property 8.81, draw-gradient 9.38, image-filename 9.38, profile-sync 9.68, stream-read 9.68, constitute-pixels 9.5 and geometry-meta 9.19.

Each header scores at least the file's score before its carve (owner, 2026-11-18). They are campaign B files now and stay on the list.

Files left at 9.68 hold only the argument counts of public (MagickExport or MagickPrivate) functions.

## Recipes added or amended
- **SL** (new): Merge Twin Switches Filling Two Struct Locals Through a Pointer and a Struct of Constants. From profile.c's source and target colour-space switches.
- **TG** (new): Guard a Loop's Tail Block with a Continue. Guard level, IDENTICAL.
- **DB** (new): Merge Declaration Blocks of One Type. Guard level, IDENTICAL.
- **TB** (amended): break membership form, with a leading parameter-only test.

## How the OH carves were proven
- Form A carves leave the translation unit's tokens in the same order. The proof is `pp_identical.sh` and `obj_identical.sh` IDENTICAL, or differing only in the file name of moved asserts (as in 5c47fa8e5). stream-read.c also differs in three cold assert paths, which now load their own file-name string.
- Where the run held per-pixel code or OpenMP regions, the gcc-16 OpenMP -O2 `.text` was compared. Both sides were compiled with the omp_compare tree's CFLAGS: HEAD's file against HEAD's headers, and the working tree against its own. Every comparison was identical, so no carve needed timing.
- Header-of-header carves compile the includer against HEAD's headers: obj_identical's `HDRS=1`, and the same extraction added to a copy of `pp_identical.sh`. A `--hdrs` option for `pp_identical.sh` would be worth adding to the tools.
- Carves were chosen to keep clone pairs together. Cuts that would part DrawAlpha/DrawColor*, Replace/Reset*, or pixel-export's and pixel-import's Export/Import* clones were probed and not taken: they would only move the duplication out of the detector's view.

## Traps seen (probed, not committed)
- **draw.c** sits near cs-mcp's duplication size limit. Any step that shrinks it enough brings back the hidden 55-function Code Duplication finding. A stub of TraceMVGPrimitive alone scores 6.49; only TraceMVGPrimitive, RenderMVGContent and IsValidListChar cleared together pass 7.1.
- **matrix.c:** helpers that cleared Overall Code Complexity revealed a Code Duplication finding (9.09 -> 8.81).
- **geometry-meta.c** (before the carve): a helper that cleared Bumpy Road raised Overall Code Complexity (8.73 -> 8.10).
- **resource.c and locale.c** sit at the Lines of Code limit. A Recipe SD split of RelinquishMagickResource (8.97 -> 8.81) and Recipe IP on LoadIncludedLocaleFile (8.96 -> 8.69) both cross it.
- **cache.c and blob.c** are plateaus. Moving a whole hot family (SetPixelCacheNexusPixels' five functions) out of cache.c leaves it at 3.52.

## For the owner
1. **draw-primitive.c to 10.0.** OH's mid-file form would move the stroke code after DrawPrimitive into a header (probe: the file 10.0, the header 7.16).
   - It needs the general reading of "private headers may be included mid-file", flagged in campaign A's PR.
   - It also needs the header's bar to be the chain's starting score (3.92), not the file's 7.22.
2. **The OH carves move debt into headers.** That is within the 2026-11-18 ruling, but please confirm that this use at scale is what you intended.
3. **The draw-primitive.c headers created tonight are new campaign B files.** They are worth renaming or merging if you prefer fewer, larger headers.
