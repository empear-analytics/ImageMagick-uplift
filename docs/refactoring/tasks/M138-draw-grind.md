# M138 - 2026-11-17, draw.c findings without new functions

Branched from `campaign/M137` after #123 merged it into `main` (ba6c3318d); this branch's
PR goes to `main`. Scores come from cs-mcp 1.5.16 through `tools/ch.py`.

Campaign B: **unchanged** (draw.c 3.27). draw.c: 65 -> 61 flagged functions, 222 -> 220
functions, 11,636 -> 11,481 lines.

## The function-count lesson

draw.c carries *Number of Functions in a Single Module*, and its weight grows with each
function. Extracting CloneDrawInfo's dash-pattern and gradient-stop blocks (Recipe E, two
helpers) cleared its *Bumpy Road Ahead* and *Complex Method* and still dropped draw.c
3.27 -> 3.16; reverted. In draw.c, and in other files at a module-level limit
(annotate.c, resource.c, xml-tree.c below), a step pays only when it adds no function:
Recipe NG, CN, G, MA into one helper serving many copies, or a merge that removes a
function (Recipe MT's new folding form). Extractions wait for a split (Recipe S) that
lowers the count.

## Commits

| Commit | Recipe | Function | Effect |
| --- | --- | --- | --- |
| 2f1f6d5a7 | NG | DestroyDrawInfo | 13 guards; leaves Complex Method (cc 20 -> 7) |
| c871dcc40 | NG | EstimateMVGPrimitiveExtent | cc 17 -> 16 |
| a3d161b91 | NG | ParseMVGStrokeDasharray | leaves Bumpy Road Ahead |
| 390a2b3fe | NG | RenderMVGContent | cc 40 -> 38 |
| 0f2adabb5 | NG | DrawImagePrimitive | cc 12 -> 10 |
| 23bafc054 | NG | TraceBezier | cc 18 -> 16, bumps 4 -> 3 (error path) |
| d2bf15045 | playbook | MA amended | address form of a role (c) lvalue |
| 0d46f3ca2 | MA | CloneDrawInfo | 11 guarded CloneString copies into CloneDrawInfoString; cc 22 -> 11 |
| 187f99286 | playbook | MT amended | folding form: whole-body twins merge, both functions leave |
| 072242df0 | MT | TracePathHorizontalLine/VerticalLine | two Complex Method -> one; one function fewer |
| 23c3e9fe1 | playbook | MT widened | folding form (b): a one-copy `if (c) S` as `if ((side == B) && (c)) S` |
| 4171c522b | MT | TracePathCubicBezier/QuadraticBezier | four findings -> two; one function fewer |

## Probed and dropped

- **annotate.c AnnotateImage** (Recipe LB step 8, loop body into AnnotateTextLine): 177
  cases 0 diverged, but 5.83 -> 5.53 (the helper is a Large Method and the file crosses
  the function-count threshold).
- **resource.c RelinquishMagickResource** (Recipe SD): clears Large Method, 8.97 -> 8.81
  (crosses Lines of Code in a Single File).
- **constitute-pixels.c SetImageTraitsFromMap** (label peels): 9.20 -> 9.08 / 9.03.
- **draw.c ClampPolygonBounds** (Recipe NS on a nested clamp): object code not IDENTICAL
  (four branch targets move by one instruction); reverted as NS step 5 says.
- **draw.c ParseMVGPopGraphicContext** (Recipe CN): Complex Conditional, 3.27 -> 3.19.
- **draw.c ReplaceAlphaPixels/ReplaceColorPixels** (MT folding): two Bumpy Road -> Bumpy
  Road plus Complex Method (cc 9); no gain.
- **Stub probes** (one function stubbed at a time): cache.c and blob.c do not move for any
  single function (blob.c OpenBlob 2.71 -> 2.77); constitute-codec.c WriteImage 9.35,
  ReadImage 9.25; geometry-meta.c ParseMetaGeometry 9.26; memory.c AcquireMagickMemory
  8.8; module.c GetMagickModulePath 7.27 (all four flagged 9.26).

## Gate at the tip

- Every code commit: `tools/oracle/build.sh cand`, `ci_warnings.sh` and `cxx_warnings.sh`
  on draw.c, `refactor_guard.py` and `--calls`, the oracle on the function's cases, the
  CodeScene pre-commit gate.
- Whole catalogue at 4171c522b against ba6c3318d: 15,961 cases, 3 diverged, all known
  (info/d333f9b818, gaps/2857e5295a, gaps/c5651ffad8).
- **Oracle cache:** two `-hough-lines` matrix-on-disk cases (matrix/f2913328b4,
  matrix/e1095950b7) had a baseline *timeout* cached for ba6c3318d from a loaded run;
  both baselines take about 9 s alone. The two entries were removed from
  `build-oracle/work/cache/953ce2019d7c6b2d-*.json` (backup `.bak-m138`); rerun: 0
  diverged. A cached timeout should probably not be kept.

## Blocked, for the owner

- **constitute.c to 10.0**: merging ReadSceneImages/PingSceneImages (Recipe CF, probe
  8.81 -> 10.0) needs one oracle case that reads `name-%d.png[1-2]` through ReadImages
  (ReadSceneImages: 0 cases). May an agent add it?
- **module.c** (6.63; all four flagged 9.26): lines 69-1672 compile only with
  `MAGICKCORE_MODULES_SUPPORT`; the `cand-mod` proof of M136 still awaits a ruling.
- **OpenMP on this Mac**: no libomp, so SortImagePixels, WriteImages and similar progress
  blocks cannot be checked here.
- **profile.c ProfileImage**: needs the Q8 and non-HDRI oracle builds (question 6).
- **annotate.c Raqm functions** (ContainsRaqmParagraphSeparator 5.83 -> 6.37 alone): Raqm
  is not installed.
