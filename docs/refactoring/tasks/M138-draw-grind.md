# M138 - 2026-11-17, draw.c findings without new functions

Branched from `campaign/M137` after #123 merged it into `main` (ba6c3318d); this branch's
PR goes to `main`. Scores come from cs-mcp 1.5.16 through `tools/ch.py`.

Campaign B: **unchanged** (draw.c 3.27). draw.c: 65 -> 24 flagged functions, 222 -> 292
functions (four merges, about eighty helpers), 11,636 -> 12,140 lines. Function findings
(one per function and category) 123 -> 49; the summed cc of the Complex Method
functions 908 -> 403.

## The function-count lesson

draw.c carries *Number of Functions in a Single Module*, and its weight grows with each
function. Extracting CloneDrawInfo's dash-pattern and gradient-stop blocks (Recipe E, two
helpers) cleared its *Bumpy Road Ahead* and *Complex Method* and still dropped draw.c
3.27 -> 3.16; reverted. In draw.c, and in other files at a module-level limit
(annotate.c, resource.c, xml-tree.c below), a step pays only when it adds no function:
Recipe NG, CN, G, B, MA or LB into one helper serving many copies, or a merge that removes a
function (Recipe MT's new folding form). Extractions wait for a split (Recipe S) that
lowers the count.

**Revised (fifteenth batch):** in draw.c the drop came from clearing CloneDrawInfo's
*Complex Method* while its *Large Method* stayed, not from the count. Taking both together
(bea801488), and about twenty other extractions that each added a function, held 3.27.
The function count still matters in annotate.c and resource.c.

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
| 7a1752f1e, 8d1c7d074 | V, D | TraceRoundRectangle | the four corner tails into TraceRoundCorner; leaves Large Method |
| f52eb157d | E | ParseMVGDashPattern | the odd-list repetition into RepeatOddDashPattern; no findings left |
| 94df112c2 | E | TracePathSmoothBezierCurve | the control-point reflection into ReflectBezierControlPoint; leaves Large Method, cc 25 -> 20 |
| ee585eefe, ead864ad3 | V, D | TracePathBezierCurve, TracePathSmoothBezierCurve | the point copy, TraceBezier call and advance into TraceBezierPoints; cc 16 -> 13, 20 -> 17 |
| fd48c43c1 | playbook | LB amended | step 7, converted-read form |
| dbafd288e | LB | ParseMVGPushPattern | three cast-wrapped list reads through GetMVGListValue; no findings left (one case reaches it) |
| d9d468c2b, f879a2a24, 58ed7dc9d | E, E, D | DrawColorPrimitive, DrawAlphaPrimitive | the color point case into DrawColorPoint, the fill-to-border target into SetFillToBorderTarget; DrawColorPrimitive no findings left |
| 9e2852dc3 | LB | TracePathArc | two flag-token skips into SkipPathListToken; cc 18 -> 16 |
| e4b035b89 | playbook | NS amended | step 1, offset form `(c ? v : w+v)` |
| bf1cb28fe ... b01ae5ee2 | NS | six path tracers | twelve absolute/relative coordinate selections into GetPathCoordinate (IDENTICAL object code); TracePathMove leaves Complex Method |
| cd10ef1e6 | playbook | MA amended | keyed address form |
| b6df87070, a714e1382, d9d2b5df9 | MA | GetDrawInfo | nine option reads into SetDrawColorOption, SetDrawValueOption, SetDrawStringOption; cc 22 -> 13, 108 -> 94 lines |
| 5bd7c27c6, d3b05f0d5 | V, E | ConvertPathToPolygon | edge-point shrink into ShrinkPolygonEdgePoints (throw left in the caller), bounds widening into WidenEdgeBounds; bumps 4 -> 3, cc 28 -> 25 |
| 1b3d7ffe2 | E | DrawPrimitive | the text case into DrawTextPrimitive, as its sibling cases; cc 14 -> 13, 101 -> 89 lines |
| fb30eaf00 | LB | ParseMVGPrimitivePoints | the point read into GetMVGPoint; no findings left |
| 63cc228b1 | P | AffineEdge | the two out-of-image tests into IsAffineEdgeOutside; no findings left |
| f9f7e02fa | E (two blocks) | DrawImagePrimitive | composite read into ReadCompositeImages, compositing tail into CompositeDrawImage; no findings left (each alone 3.17/3.16) |
| b6bf59389, a94bd1d6f | FI; E (two blocks) | DrawAlphaPrimitive | the reset scan by initialiser; the alpha-channel setup and the point case into EnsureBlendAlphaChannel and DrawAlphaPoint; no findings left |
| 2e6378097, 004681e75, 69e5b567d | FB, LB, E | TracePathArc | shape by initialiser, the separator check into SkipPathListSeparator, the advance into AdvancePrimitiveOffset; leaves Large Method (cc 13 stays) |
| 417b7cd4e, 374142775 | NS, P | GetWindingNumber | direction sign into GetEdgeDirectionSign (IDENTICAL), skip test into IsWindingEdgeSkipped; no findings left |
| aa0a69e2f, 869ecc368 | MA, P | TraceRoundRectangle | the two radius clamps into ClampRoundCornerArc, the degenerate test into IsDegenerateSegment; no findings left |
| 92d9d6ae9, b746b5f71 | P, E | TraceEllipse | zero-radius and closed tests into predicates, the angle wrap into GetEllipseEndAngle; no findings left |
| 0c062eff3 | NS | TracePathBezierCurve | the side's letter and point count into GetBezierAbsoluteLetter and GetBezierPointCount (IDENTICAL); no findings left |
| (playbook), ea02b4b9e | LB | ParseMVGViewbox | four cast-wrapped reads through GetMVGValue/GetMVGListValue (target unread on the exit path, checked through ParseMVGKeyword and RenderMVGContent); no findings left |
| b062cf787 | P | DrawStrokedPrimitive | the closed-path test through IsClosedEllipse (moved above its first user); leaves Complex Method |
| a519f0ad8, 90256d467 | P, CN | ParseMVGPopGraphicContext | the outer clip-mask test into IsNonSVGClipMask, then combined with the inner test (IDENTICAL); no findings left |
| 6cdf7f056 | E | GetRepeatGradientColor | the blending arm into BlendRepeatGradientStops (timed +0.8%); no findings left |
| a198efbc3, 534bf61ca, 2e9b40a88 | NS, P, LB | TracePathSmoothBezierCurve | side selections named (IDENTICAL), three side tests as predicates, the x/y pair into GetPathPoint; no findings left |
| 255f6d4a0, 0f6fe3571, 23433b9e0 | E | DrawStrokePolygon | per-subpath trace and fill into DrawStrokeSubpathPolygon, the round caps into DrawSubpathRoundLinecaps, the stroke clone into CloneStrokeFillInfo; no findings left |
| 15537f632 | B | DrawStrokeSubpathPolygon | its five parameters (a new finding of 255f6d4a0 the gate did not show) bundled into StrokeSubpathPaint, filled once before the loop; no findings left |
| 2d2b74d00, 8412a1647 | P, LB | TracePathArc | the end point through GetPathPoint, the arc's shape reads into GetPathArcShape (the commit says Recipe E; it is LB step 7's exit-statement form); no findings left |
| d6ea8a367 | E | DrawStrokedPrimitive | the wide-stroke arm into DrawWideStrokedPrimitive; no findings left |
| 4b6dff923 | E | DrawPatternPath | the pattern clone setup into ClonePatternDrawInfo; leaves Complex Method (its five public arguments stay) |
| 6cec0c04a | LB | GetMVGMacros | the two-exit token read of both scans into GetMVGMacroToken; cc 19 -> 17 |
| 23b9f122e, 5e8d578dd | LP, E | ConvertPrimitiveToPath | the non-path primitive labels into IsNonPathPrimitive, the tail into TerminatePathInfo; cc 17 -> 11, 96 -> 80 lines |
| 3c7dc9535 | E | DrawPrimitive | the dispatch switch into DrawPrimitiveByType; no findings left |
| 63db10b47, eda78be26 | E | GetDrawInfo | the defaults into SetDrawInfoDefaults, then the image-info attributes and the weight option in one commit; no findings left |
| 6166919af, bea801488 | E | CloneDrawInfo | the text attributes into CloneDrawInfoTextAttributes, then the dash pattern into CloneDrawInfoDashPattern; no findings left |
| 9b74839f6, 9d11cee7b | E, LB | ParseMVGPushGradient | the affine transform into GetGradientBounds, the four segment reads into GetMVGSegment; no findings left |
| a030b4a5f, ff8371e1f | E, V | TraceBezier | the Bernstein sum into GetBezierPoint, the control-point trace into TraceBezierControlPoints; cc 11 -> 9, 97 -> 82 lines, leaves Bumpy Road |
| 9ae7a352f, 442d79366, bda3bb874, (playbook 33131a77e), 9acb019e0 | PS, PS, E, K | DrawAffineImage | the column loop into DrawAffineRowPixels, the whole row body into DrawAffineImageRow (both in the parallel loop, timed in the OpenMP build: within A/A), the corner transform into GetAffineImageExtent, the min/max scan into GetAffineImageBounds (Recipe K's new outside-region form); no findings left |
| c848e000d | Q | DrawGradientImage | the per-pixel spread switch into GetSpreadGradientColor (timed through magick and the driver in the OpenMP build: within A/A at 21 runs); cc 13 -> 9, 140 -> 92 lines, leaves Bumpy Road |
| 62037b202 | revert | ResetAlphaPixels/ResetColorPixels | 460d384e0's merge reverted: in the OpenMP build the merged loop made the alpha reset 10.7% slower |

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
- Case map rebuilt after the merge (2026-11-17, 15,967 cases, 10,008 functions); the old
  one kept as `build-oracle/work/casemap-2026-11-17.json`.
- Whole catalogue at 58ed7dc9d: 3 known and resize/f6be4a23b4 (half write mask, `-resize
  150%`), which then diverged once in three runs alone: the random write-mask resize family
  of ORACLE.md's resize/6ee45e9cfa, in campaign A's code this branch does not touch.
  resize/6ee45e9cfa itself diverged once in a GetDrawInfo gate run (13,204 cases) and agreed
  alone three times.
- Whole catalogue at 1b3d7ffe2, a94bd1d6f, 69e5b567d, ea02b4b9e, 6cdf7f056 and 2e9b40a88: 15,967 cases, 3
  diverged, all known. check_trailers: 108 refactor commits, all valid.
- Whole catalogue at ff8371e1f (fifteenth batch): 15,967 cases, 3 diverged, all known.
- Whole catalogue at 62037b202 (sixteenth batch): 15,967 cases, 3 diverged, all known.
  check_trailers over ba6c3318d..HEAD: 379 refactor commits (128 of them on draw.c, the
  rest from the main merge), all valid.

## Blocked, for the owner

- **constitute.c to 10.0**: merging ReadSceneImages/PingSceneImages (Recipe CF, probe
  8.81 -> 10.0) needs one oracle case that reads `name-%d.png[1-2]` through ReadImages
  (ReadSceneImages: 0 cases). May an agent add it?
- **module.c** (6.63; all four flagged 9.26): lines 69-1672 compile only with
  `MAGICKCORE_MODULES_SUPPORT`; the `cand-mod` proof of M136 still awaits a ruling.
- **OpenMP on this Mac: unblocked (2026-11-18).** Apple clang has no libomp, but Homebrew's
  gcc-16 ships libgomp. `tools/omp_compare.sh build` works when gcc and g++ resolve to
  gcc-16 and g++-16 (symlinks in a scratch directory put first in PATH, plus
  `CXX=g++-16` for Magick++), anaconda's x86 `as`/`ld` are taken out of PATH, and the
  script runs under `arch -arm64`. The result reports `Features: OpenMP` and a 3000x3000
  blur takes 3.33 s at one thread and 1.06 s at four. Nothing was installed; the script
  is unchanged. The OpenMP -O2 timing of the 2026-11-17 ruling can now be done here.
  The two per-pixel steps of this branch timed only in the oracle build were re-timed
  here: 6cdf7f056 (BlendRepeatGradientStops) is within A/A on radial and linear repeat;
  460d384e0 (the ResetScanPixels merge) made the alpha reset 10.7% slower and is
  reverted (62037b202).
- **Branch-wide timing (2026-11-18):** `main`'s draw.c against the tip's in the gcc-16
  OpenMP build, six drawing workloads at 2000x2000 (polygon, shapes, Bezier path, dashes,
  round joins, nonzero fill): omp_compare 0 diverged at 1 and 4 threads; at 4 threads
  all within A/A. At one thread the shapes and path workloads read +0.7% to +1.4%
  (A/A up to +0.7%). A scan of eight points along the branch showed no step, and the
  object code settles it: DrawPolygonPrimitive's two per-pixel OpenMP functions differ
  between main and the tip only in register names and alignment `nop`s, and one
  commit that measured +1.0% (374142775) compiles to the same code as its parent in
  every function. Readings of about 1% at one thread are within this harness's noise.
- **profile.c ProfileImage**: needs the Q8 and non-HDRI oracle builds (question 6).
- **annotate.c Raqm functions** (ContainsRaqmParagraphSeparator 5.83 -> 6.37 alone): Raqm
  is not installed.

## Next in draw.c

- **Probed and left:** Recipe DT on all thirty `? MagickTrue : MagickFalse` predicate returns
  (IDENTICAL object code) leaves draw.c at 3.27 and Overall Code Complexity in place;
  GetFillAlpha's point loop (its helper would be a Complex Method); CloneDrawInfo (its 92
  lines are member copies); ParseMVGClass and DrawBoundingRectangles (extractions of six or
  seven parameters).
- **Label-run predicate:** IsValidListChar (cc 12) is allowed by the owner's ruling of
  2026-11-07.
- **Left for decomposition:** the large functions (TraceStrokePolygon cc 43, TraceMVGPrimitive
  43, TracePath 39, RenderMVGContent 38, DrawPolygonPrimitive 30, ConvertPathToPolygon 25,
  DrawDashPolygon 24, DrawAffineImage 23 and DrawGradientImage 13 with OpenMP loops) hold
  most of the remaining findings; draw.c's score moves only once they go (M137's stub probe).
- **Scoring rule of thumb:** never clear a function's Complex Method while its Large Method
  stays; take both in one commit, or Large Method first. Under that rule, extractions hold
  3.27 (the fifteenth batch added about twenty helpers); the earlier CloneDrawInfo drop
  (above) was this trap, not the function count.
- **Probed and left (fifteenth batch):** GetMVGMacros's inner scan as a Recipe CS helper
  clears GetMVGMacros but the helper is a Complex Method (cc 10), so it was reset;
  GetPolygonBounds, LogPrimitiveInfo and DrawBoundingRectangles have no case; ParseMVGClass
  (one case) needs a two-output split to leave Large Method; ConvertPrimitiveToPath's
  remaining blocks would take five parameters.
- **Gate fix:** the gate script now refuses a step that leaves a new or larger finding on
  any other function, and an oracle run that reached no case.
- **DrawGradientImage's row body (Recipe PS step 7), dropped for speed:** cleared both
  findings, 0 diverged and omp_compare 0, but the repeat spread at one thread measured
  +1.9%, +1.4% and +0.7% (A/A +0.4%, +0.1%, +0.1%); the member form (no local copies)
  +1.5% and +0.5%. Not committed under the 2026-11-17 ruling.
