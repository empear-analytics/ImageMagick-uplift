# M138 - 2026-11-17, draw.c findings without new functions

Branched from `campaign/M137` after #123 merged it into `main` (ba6c3318d); this branch's
PR goes to `main`. Scores come from cs-mcp 1.5.16 through `tools/ch.py`.

Campaign B: **unchanged** (draw.c 3.27). draw.c: 65 -> 48 flagged functions, 222 -> 236
functions (four merges, twenty-two helpers), 11,636 -> 11,468 lines. Function findings
(one per function and category) 123 -> 90.

## The function-count lesson

draw.c carries *Number of Functions in a Single Module*, and its weight grows with each
function. Extracting CloneDrawInfo's dash-pattern and gradient-stop blocks (Recipe E, two
helpers) cleared its *Bumpy Road Ahead* and *Complex Method* and still dropped draw.c
3.27 -> 3.16; reverted. In draw.c, and in other files at a module-level limit
(annotate.c, resource.c, xml-tree.c below), a step pays only when it adds no function:
Recipe NG, CN, G, B, MA or LB into one helper serving many copies, or a merge that removes a
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
| 3f826192a | playbook | MT widened | folding form (b): either copy, and `S` may be a `break;` |
| fef8d5575 | MT | TracePathSmoothCubicBezier/SmoothQuadraticBezier | four findings -> two; one function fewer |
| 3647e76d0 | G | FlushFinalEdge | leaves Bumpy Road Ahead |
| 21159a190 | B | IsRenderableClass | leaves Excess Number of Function Arguments |
| f1244915e | B | IsNewSubpathPoint | leaves Excess Number of Function Arguments |
| 985809cfc | B | IsDashSegmentPending | leaves Excess Number of Function Arguments |
| 2cb5acab7 | B | DrawClippingMask | leaves Excess Number of Function Arguments |
| 27cd4bd7d | B | DrawCompositeMask | leaves Excess Number of Function Arguments |
| bd76a842b | B | TraceArcPath | leaves Excess Number of Function Arguments (7) |
| 9b410ebcb | playbook | LB amended | step 7, exit-statement form (a throwing macro left in the caller) |
| 318e07295 ... 58d76cd0b | LB | six path tracers | 14 value reads into GetPathValue; TracePathBezierCurve leaves Large Method; Arc cc 30 -> 20 |
| 36a124bf5 ... 4c844566d | LB | five MVG handlers | 11 list reads into GetMVGListValue; ParseMVGAffine leaves Complex Method (cc 18); PushGradient cc 16 -> 10 |
| 2acc1f110, 6466c4876 | LB | ParseMVGPrimitivePoints, PushGradient | first value read into GetMVGValue; PrimitivePoints leaves Large Method |
| 8439e661e ... 21334be30 | E, D | six path tracers | separator skip into SkipPathSeparators; TracePathLine and AxisLine leave Complex Method, TracePathMove leaves Large Method |
| aef7f0364 | LB | ParseMVGDashPattern | list read into GetMVGListValue; leaves Complex Method |
| ea01cacc0, 377fcb7f1 | E, D | GetStrokeSlope | the two epsilon-slope blocks into GetEpsilonSlope; leaves Bumpy Road and Complex Method |
| 8fce72280, 874610f68 | E, D | TraceMVGPrimitive | eight case tails into GetTracedPrimitiveEnd; cc 51 -> 43, 290 -> 274 lines |
| 035feac90 ... fffec6216 | E, D | AffineEdge | the eight edge clamps into RaiseEdgeStart and LowerEdgeEnd; leaves Bumpy Road (4) and Large Method, cc 17 -> 9 |
| 460d384e0 | MT | ResetAlphaPixels/ResetColorPixels | merged into ResetScanPixels (no findings either side); one function fewer; timed +0.1% |
| f2602e082 | E | CloneDrawInfo | gradient-stop copy into CloneDrawInfoGradientStops; leaves Bumpy Road (the dash-pattern block alone: 3.27 -> 3.14, left) |
| 565d0ae02 | V | ClonePolygonEdgesTLS | per-edge point copy into ClonePolygonEdgePoints, the throw left in the caller; leaves Bumpy Road |
| 26453f1d8 | E | GetWindingNumber | edge-index search into GetWindingEdgeIndex; leaves Bumpy Road (cc 13 -> 11) |
| 79da55aab, b7ef2e5ae | E, D | DrawStrokedPrimitive | the unstroked fill into DrawUnstrokedPolygon (context pass-through); leaves Large Method |
| 8e2513918, 164d9b253 | LB, CN | ParseMVGPushGradient | the radial token skip into SkipMVGListToken, then the radial test and the skip combined; leaves Bumpy Road |
| b81e108fb | V | TraceBezier | the coefficient-range scan into GetBezierQuantum, the throw left in the caller; cc 16 -> 12, bumps 3 -> 2 |
| 84511a6ee, 623ad156b, 04c2aca89 | E | EstimateMVGPrimitiveExtent | bounds, curve count and path-token count into three helpers; no findings left (was Bumpy 3, cc 16, 102 lines) |
| f077afa69 ... 296b8075a | E, D | five trace functions | the closing primitive-type loop into SetTracedPrimitiveType; TraceRoundRectangle 76 -> 70 lines |
| 2f03f7f2d | merge | origin/main (campaign A's W38, #122) | clean; draw.c untouched |

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
- **draw.c DrawAlphaPrimitive/DrawColorPrimitive** (MT folding): four findings -> two in a
  probe, token-checked, but `channel_mask` (set and restored only on the alpha side) is
  *may be uninitialized* under `-Wconditional-uninitialized` (MSVC C4701, an error in
  CI); an initialiser would add a value. Reverted.
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
- Whole catalogue at 985809cfc: 15,961 cases, 5 diverged: the three known ones and the two
  `-hough-lines` cases again, this time the *candidate* timing out under load; alone, 0
  diverged. They take about 9 s alone and pass 30 s under a full run on either side:
  added to `LOAD_SENSITIVE` with the owner's OK (990ecda37).
- Whole catalogue at aef7f0364: 15,961 cases, 4 diverged: the three known ones and
  resize/b41fd3fe32 (write mask + `-scale`, M137's load-sensitive candidate), 0 diverged
  alone three times. Not added to `LOAD_SENSITIVE`: that needs its own OK.
- Whole catalogue at 874610f68, fffec6216 and 26453f1d8: 15,961 cases, 3 diverged, all known.
- After merging main (71f80adb8, campaign A's W38): 15,967 cases, 3 diverged, all known.

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

## Next in draw.c

- **At the threshold** (cc 9, one decision from clean): AffineEdge, ParseMVGClass,
  ParseMVGPrimitivePoints, ClampPolygonBounds (NS fails IDENTICAL), TracePathMove.
- **Shared tails left**: the primitive-type fill loop (`for (i=0; i < coordinates; i++)
  { p->primitive=...; p--; }`) in five trace functions and the point-append block in four
  path tracers take one decision each from functions that stay flagged.
- **The score is not monotonic in findings.** Clearing ParseMVGClass's Complex Method with a
  Recipe LT helper dropped draw.c 3.27 -> 3.17, although the helper alone (unused) kept 3.27 and
  stubbing another cc-9 function kept 3.27; CloneDrawInfo's dash-pattern block alone gave
  3.14, its gradient block alone 3.27. Every step is probed and dropped when the score falls.
