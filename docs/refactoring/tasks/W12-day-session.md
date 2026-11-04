# W12 - campaign A, day session (2026-11-01)

Not a generated task file. Campaign A (Windows desktop, WSL2), branch `campaign/W12`, cut
from `campaign/W11` (#85) after `main` was merged into it twice (#83, #84). Measured with
`cs-mcp` 1.5.16. The owner marked #85 for merge and said "You can continue with the other
splits you want. that's fine." (2026-11-01).

### Report

| File | Code Health | Commits |
| --- | --- | ---: |
| `statistic.c` | 5.84 → 6.55 (split) → **6.73** | 1 + 8 |
| `statistic-measure.c` (new) | 9.09 → **9.68** | 1 |
| `resize.c` | 7.08 → 7.11 → **8.65** (Recipe PS) | 3 + 3 |
| `enhance.c` (again) | 4.85 → **4.91** | 1 |
| `feature.c` | 5.24 → **5.94** (Recipe PS) | 5 |
| `effect.c` (again) | 4.36 → 4.51 (Recipe PS and others) → **5.15** (Recipe SD, PS) | 9 + 10 |

Ten refactor commits (the split, eight on `statistic.c`, one on `statistic-measure.c`). Every
step ran the oracle on its function's cases (0 diverged); the whole catalogue ran after
each file: 15,900 cases, 0 diverged.

### Splits examined (`tools/split_source.py --out`, both halves scored)

- **`statistic.c`: done** (0129266cc). The measurement family (eight public functions, 35
  functions, 1,211 lines) is `statistic-measure.c`: `statistic.c` 5.84 to 6.55, the new file
  9.09, guards over the pair unchanged (256 literals, 452 call sites), the eight public
  functions the only exported symbols of the new object.
- **`transform.c`**: the trim, consolidate, auto-orient, transform, extent and shave
  families (684 lines) would go to a new file at 9.38, but `transform.c` stays at 5.6: no gain.
- **`quantize.c`**: `GetImageQuantizeError` (181 lines, new file 10.0) is the only family
  the tool accepts (`MaxTreeDepth` and `QCubeInfo` cross every other cut); it leaves
  `quantize.c` at 5.6 and takes three functions out of the 110.
- **`threshold.c`** (`ThresholdsFilename`, `ThresholdImageTag`) and **`feature.c`**
  (`CannyEdgeImageTag`, which `HoughLineImage` uses too): a file-level macro crosses every
  cut that has a useful balance, which Recipe S refuses.
- **`segment.c`, `vision.c`, `fourier.c`, `layer.c`, `resize.c`**: one family holds nearly
  the whole file, or the rest is too small to matter.

### For the owner

- **A ruling that would open `resize.c` and the other `status`-assigning loops:** a recipe
  that moves a whole block of a parallel loop body into a helper taking `MagickBooleanType
  *status` and writing and reading it through the pointer (`*status=...`), instead of T's
  and AS's local copy. The reads and writes then happen at the same points, in the same
  order, to the same memory as inline, so the argument is the one the skip-guard probe
  used; what it needs from you is the OpenMP rule, which the playbook sets at E step 5 (a
  block moves only if every variable it writes is declared inside the loop body) and Q step
  1 (no OpenMP region at all). Measured: `InterpolativeResizeImage`'s whole channel loop as
  a helper gives `resize.c` 8.6 (7.11 now), not committed because it is not legal today;
  `feature.c`'s `MeanShiftImage` and `effect.c`'s `SpreadImage` and `KuwaharaImage` (an
  assign-and-test `status=Interpolate...` with a `break`) meet the same wall. The oracle
  runs single-threaded and cannot see a race, so every such step would be checked by
  reading, as T and AS are.

- **Confirm or revert the `statistic.c` split**, as for `effect.c` and `enhance.c` (#85). The
  Windows build configuration outside this repository still needs `statistic-measure.c`.
- **Close readings:** `statistic.c` commits 3, 4, 5 and 7 pass the progress macro's tag as a
  `const char *` so one `SetProgress` helper serves three functions (T plus D's single
  differing value), and reuse a helper an earlier T commit created (T has no text for a
  second caller); the second `I` step (d60dde784) calls the helper the first created for an
  identical block.
- **Recipe-text gaps found:** a `continue` block mid-body in a parallel loop (`p == NULL` or
  a failed fetch, then `status=MagickFalse;`) fits no recipe: T, LB, OC and W each refuse
  it, and it is what holds `GetImageRange` at 9.68 and `EvaluateImages` at cc 27.

### Recipe PS, step 2 amended: a second shared scalar (6 commits)

The sweep left two files blocked only because their loops also write a second shared scalar.
PS step 2 was amended (43696cf4b, `mandate (2026-10-24)`): besides `status`, at most two
further shared scalars `v` (in `shared(...)` or no data-sharing clause, never in `private`,
`firstprivate`, `lastprivate` or `reduction`), read or written by plain assignment or a plain
increment outside any `atomic` or `critical`, each passed by pointer and written `(*v)`; a
loop with no `status` whose only such variable is a flag passes that flag alone. A plain
increment that is a race stays a race (a new *Careful* bullet). Whole catalogue after the last
commit: 15,900 cases, 0 diverged.

| File and function | Score | Cases |
| --- | --- | ---: |
| `attribute.c` `IdentifyImageGray` (941250151, `type`) | 7.11 → 7.42 | 1,172 |
| `attribute.c` `IdentifyImageMonochrome` (ef16eb71b) | 7.42 → 7.73 | 222 |
| `resize-sample.c` `SampleImage` (10b2103f7, `progress`) | 9.47 → **10.00** | 112 |
| `channel-alpha.c` `SetImageAlphaChannel` OffIfOpaque (acdc42902, `opaque`) | 8.07 → 8.14 | 1,350 |
| `attribute.c` `IsImageOpaque` (5baaec2a8, lone flag `opaque`) | 7.73 → 7.92 | 51 |

**`resize-sample.c` reaches 10.00.** Its loop body writes the shared `progress` with a plain
`progress++`, an upstream data race (W10 report); the helper keeps it as `(*progress)++`
and the commit says it is not fixed, so the file's behaviour, race included, is what it was.
**Close readings:** the amendment also lets a loop with no `status` pass only a flag `v`
(`IdentifyImageMonochrome`, `IsImageOpaque`; T's *other name* paragraph arguably covers
those two, PS was used because the amendment names it); `IdentifyImageGray`'s block reads
`type` in a test before writing it (the amendment allows reading); `acdc42902` reuses
`RemoveAlphaRowInfo` for a different loop; `#define SampleImageTag` moved from
`SampleImage` into the helper.

### The Recipe PS sweep over the other files (12 commits, 11 kept)

PS and its step 7 applied to loops the earlier recipes refused; each commit records the PS
conditions checked by reading (the pragma's `shared(status)`, `status` the only outer variable
written, each `continue` as `return`, no pragma in the moved text). Whole catalogue after
the last: 15,900 cases, 0 diverged.

| File and function | Score | Cases |
| --- | --- | ---: |
| `shear-deskew.c` `RadonTransform` (27c1e00cf) | 8.74 → 8.97, leaves Bumpy Road | 18 |
| `colorspace.c` `TransformsRGBImage` Log loop (fbc8dc272) | 3.15 → 3.21 | 424 |
| `colorspace.c` `sRGBTransformImage` LinearGray loop (22d999ccd) | 3.21 → 3.27 | 1,205 |
| `vision.c` `AngleThreshold` (0cb1ca627) | 5.64 → 5.91, leaves Bumpy Road, Complex Method and Large Method | 4 |
| `enhance-tone.c` `HaldClutImage` (ef7d98919) | 4.17 → 4.24 | 5 |
| `visual-effects.c` `MorphImages` (9723b0eb9) | 3.50 → 3.53 | 3 |
| `channel-alpha.c` `SetImageAlphaChannel` Remove (b598badaa), Shape (ac09858da), Disassociate (bb810647d) | 7.65 → 8.07 | 1,350 |
| `attribute-depth.c` `GetImageDepth` (ac92e4701) | 7.33 → 7.54 | 309 |
| `feature.c` `CannyEdgeImage` gradient loop (353f250a7) | 5.94 → 6.08 | 12 |

No case selects `TransformsRGBImage`'s Log branch, so its step was checked with a
round-trip diff of `-colorspace Log` against the base binary (identical), and the same
spot diff was run for the LinearGray step. **Close readings (ac09858da, bb810647d, ac92e4701):**
the helper is only the `if (status == MagickFalse) continue;` guard, one call and its
`== MagickFalse` test (step 7's small form: the whole body of a small loop): the effect is a
guard moving out of the function, the bump and cc counts fall and the line count stays or
rises by two. Step 7 allows it as written; revert them if you rule that is not what it
means. Probes that did not pay: a second helper of the same shape in `colorspace.c`,
`vision.c` and `channel-alpha.c` (*Code Duplication*), `channel.c` 6.93 → 6.64,
`composite.c` 3.01 → 2.99, `quantize.c`, `shear-rotate.c` 7.63 → 7.17–7.43, `compare.c` flat,
`distort.c`'s column loop (the helper would be about 145 lines); a loop with a progress or
`omp atomic` block cannot move whole (its early `continue;`s skip the block).

**Not covered by PS, not applied:** a second shared scalar besides `status`
(`attribute.c` `IdentifyImageGray` also writes the shared `type`: +0.31 probed;
`IdentifyImageMonochrome` writes only `type`), and `resize-sample.c` `SampleImage` (9.47 →
10.00 probed), whose loop writes the shared `progress` with a non-atomic `progress++`, an
upstream race a helper would keep as it is. **Out of campaign A:** the sweep also found
`matrix.c` `MatrixToImage` (7.45 → 7.76; 23 cases), a file of campaign B; that commit
(21a5350e6) was taken off this branch to a local branch `w12-matrix-c`, not pushed,
because the file belongs to the other campaign; image.c (`SetImageMask`, `SyncImage`) and
annotate.c (`RenderPostscript`) are B's too and were not probed.

### `effect.c` `PreviewImage` and `LocalContrastImage`: three amendments under the mandate

`PreviewImage` (cc 56, 397 lines) went to cc 20 and 182 lines through eight Recipe SD steps
(1cc426fad to 20a08413c, 4.51 to 5.02, 29 cases each), and `LocalContrastImage` (cc 19, 155
lines) to cc 9 and 118 through its two loop bodies as PS helpers (b95941dff, bb6083ab7; 44
cases), 5.02 to 5.15: 4.51 to 5.15 in all. They rest on three amendments, each in its own
commit before the steps that use it, all *Decided by* `mandate (2026-10-24)`, lint clean:

1. **SD step 1** (8e6fde873): a group may hold `break;` inside a selection (14 of the 30
   cases did), as a chain's groups already may; the inner `switch` ends, the helper returns,
   and the outer default's `break` ends the outer `switch`.
2. **SD step 8, new** (8e6fde873): a chain that needs Z's struct declares one `tile` at the
   top of the loop body and fills it once before the `switch` (N step 8's sibling form), and
   each helper passes it on as `&(*v)`; without it step 2 would move the default group's struct
   fill into every later helper.
3. **PS step 7, new** (5bd094ff0, worded in 388c27f6e): a parallel loop's whole body may move
   with its `continue;` statements, each becoming `return;`, as E step 7's whole-body form
   allows: `status` through the pointer, the loop counter by value, the input struct declared
   and filled in the body (so each thread fills its own), no pragma in the body. I read both
   `Run...` helpers: neither holds a pragma, `status` is shared by default (the pragmas name no
   clause), and the thread id is taken in the helper by the same thread. A candidate gap not
   closed: a struct of out-pointers for SD would open the five `PreviewImage` groups that
   stay (the four-argument limit).

Close readings: `label` and `factor` (char arrays written by callee calls) are `char *`
members of the read-only struct, as N's members that point at written buffers are; each SD
helper's default group passes `tile`, `&(*preview_image)` and `&(*gamma)` on; the `Run...`
helpers' OpenMP side was read by eye (the oracle is single-threaded).

### Recipe PS (owner, 2026-11-04): first uses

Recipe PS (Extract Block Through a Pointer to the Loop Status; 97c21e2d8) moves a block of a
parallel loop body into a helper taking `MagickBooleanType *status`, every use written
`(*status)`, so the reads and writes of the shared flag happen at the same points, in the
same order, to the same memory. `status` must be shared (the pragma's `shared(...)` or no
clause) and no pragma moves with the block; steps 1 to 3 are checked by reading.

- **`resize.c` 7.11 → 8.65** (5e05ad4a7, 5e674e81f, e9fc22e39): the column loop of
  `InterpolativeResizeImage` (`InterpolateResizeRow`, a const `InterpolativeResizeInfo`) and
  then the line (offset, row, sync test): the function left Brain Method and Deep, Nested
  Complexity. The first version passed a `PointInfo` by value with `offset.x` unset: gcc is
  quiet, MSVC may report C4701 and the Windows builds treat warnings as errors, so the
  follow-up passes `const double offset_y` and the helper declares its own `offset`.
- **`feature.c` 5.24 → 5.94** (7fd43ea9a, 88d5c4f86 PS; 765e3dc98 LB, aa6418afc N, 1d9222d6a
  V inside and around the helpers PS created): `MeanShiftImage` left Brain Method and Deep
  Nesting; the file crossed CodeScene's 1,000-line mark (1,084 lines of code now) and the score
  still rose.
- **`effect.c` 4.36 → 4.51** (fe0fd65b4 `SpreadImage`, 453254f58 `KuwaharaImage`; V, I, N and
  Hull steps follow), then 5.15 (below).

Close readings: `feature.c` commits 2 and 3 apply LB, N and E inside a helper PS created
(lexically outside any `omp` region, writing the shared flag through the helper's pointer: E
and I let writes through pointers the block already uses stay); `feature.c` commit 1 moves the
function-local `MaxMeanShiftIterations` into the helper with its only user (E step 7's macro
clause; PS does not say it); `effect.c` c1622ac69 gives `UnsharpRow` the loop body's existing
`const UnsharpPixels` in place of a new struct (N step 2 allows that when the caller is a
helper binding the members; here the caller is the loop body); `effect.c` 0c0d3e1ee reuses
the V helper in a second function (V step 6 plus W step 7), and aa7605841 and 7101ebbb5 are I
with two written images, whose failure paths no case reaches. A candidate amendment, not made:
Recipe F for a three-line `static const` table that blocks an extraction (`effect.c`'s
`DespeckleImage`, 4.51 → 4.57 in a probe, whose hull `k` loop needs `X[4]` and `Y[4]`).

### The Recipe D amendment, first uses (one commit, three probes left out)

`enhance.c` f72f8fa8a, form (b): the progress block of `CLAHEImage`'s second row loop equals
`UpdateCLAHEProgress`'s body once `progress` and `status` are `(*progress)` and `(*status)`
(by script, token for token), so it becomes `UpdateCLAHEProgress(image,&progress,&status);`:
4.85 to 4.91, Bumpy Road gone, cc 16 to 14, 109 to 100 lines, 13 cases, 0 diverged. The W11
note that the block "cannot reuse the helper" applied to the loop extracted whole, where
`status` is a plain variable; in place the addresses exist. Not committed: `enhance-tone.c`'s
`GetGrayscaleIntensity` (the Rec709 block equals `EncodeGrayscaleGamma`'s body and the two
sRGB decode blocks are identical: 4.17 to 4.20, but cc 11 and 70 lines remain flagged, so no
finding goes and the amendment's condition fails); `effect-blur.c`'s `AdaptiveSharpenImage`
(`PrepareAdaptiveEdge` reuse: 5.29 to 5.30, no finding removed; its inner loop differs from
`BlurAdaptiveRow` in extra parentheses, equal in value, not the same text). A token-level
scan of seventeen files (vision to resize) found three further whole-body hits, none in a
function where a finding would go.

### `resize.c`: Recipe AS does not apply to `InterpolativeResizeImage`

Three small steps (T on the progress call, E step 7 on the tail, E step 3 on the scale
assignments: 7.08 to 7.11, 104 to 96 lines). AS step 2 fails on every candidate block: the
channel loop, the column loop and the body past the guard leave `status` alone when a loop
runs zero iterations (`GetPixelChannels(image) == 0`) or every channel hits the `continue`,
and the channel loop's `break` leaves the block (step 1); only `offset.x=...;
status=InterpolatePixelChannels(...)` meets step 2, needs a nine-variable struct and probes
7.11 to 7.09. **The lever is a pointer-status form for parallel loops** (below, for the owner).

### The skip-guard recipe, probed and not added

The owner allowed a recipe for the parallel loop's head, `if (status == MagickFalse)
continue;` followed by a fetch and `if (p == NULL) { status=MagickFalse; continue; }`,
which no recipe covers (T, LB, OC and W refuse it). The idea was one helper that reads and
writes the shared `status` through a pointer at the same points, in the same order, and
returns whether to skip (`if (SkipRow(&status,...) != MagickFalse) continue;`); the
ordering argument holds. It does not pay, measured with `tools/ch.py`:

| Probe | Before | After | Finding |
| --- | --- | --- | --- |
| `GetImageRange` (`statistic-measure.c`), 8-argument helper | cc 10, file 9.68 | cc gone, file 9.68 | the helper has *Excess Number of Function Arguments* (8); a 5-argument form is still flagged |
| `ClampImage` (`threshold.c`) | cc 11, file 5.35 | cc 9, file 5.37 | new Excess Arguments (8) |
| all nine row loops of `threshold.c`, one helper | file 5.35 | file 5.41 | every cc drops by 2, bumps stay 2, *Code Duplication* grows from 6 to 8 functions |
| `FlipImage` (`transform.c`), 9-argument helper | cc 12, file 5.60 | cc 9, file 5.52 | the file score drops |
| `ContrastImage`, six sites (`enhance.c`) | file 4.85 | file 4.97 | two new helper findings (one per fetch kind) |

Why: the helper needs the status pointer, the view, the row, the width, the output pointer
and the exception (five arguments at the least, with `x=0` and one row baked in), and
CodeScene flags a function over four; the bumps stay because the second bump of each loop
is its progress tail, not its head. A bundle struct of view, exception and status would get
under four arguments, at one new type per file. No recipe was added.

### Correction made before pushing

- A subagent overwrote three old scratch scripts in the shared scratchpad and deleted copies
  in `~/sp` (not repository files; nothing in git is affected), and later subagents were told
  to keep their scripts in a directory of their own.

- Commit f190a8c9b's message gave "72 -> 69 lines" for `EvaluateImage`, numbers inferred
  from a probe, not measured; `code_health_review` lists the function under *Large Method*
  before the commit and not after. The message was rewritten to say that; the tree is
  unchanged.

## Recipe PC and the second half of the day (2026-11-04)

Recipe PC (aacff05ea, new, `mandate`) moves the head of a parallel loop body (status guard,
fetch with its `continue`, work, sync test) into a `static MagickBooleanType` helper when an
`omp atomic` progress tail has to stay in the caller; each `continue;` becomes
`return(MagickFalse);`, the caller continues on `MagickFalse`. Read-only inputs go in a
`const` struct filled in the body before the call, so the fills now run before the guard.
PS step 8 (38b4f4131) and OC step 6 (875fe9239) are amendments, also under the mandate.
Whole catalogue at each file's last commit: 15,900 cases, 0 diverged.

| File | Score | Commits | Recipes |
| --- | --- | ---: | --- |
| `paint.c` | 6.24 → 6.55 | 2 + PC | PC |
| `threshold.c` | 5.35 → 5.50 | 1 | PC |
| `transform.c` | 5.60 → 5.90 | 3 | PC |
| `segment.c` | 5.70 → 6.44 | 7 | PC, LB, B, PS, D, O, E |
| `vision.c` | 5.91 → 6.98 | 10 + 2 amendments | PS 8, H, OC 6, B, D |
| `fourier.c` | 6.02 → 6.27 | 7 | PC, B |
| `feature.c` | 6.08 → 6.51 | 6 | PC, E, Wa, V, B, Y |
| `compare.c`, `quantize.c` | 2.54, 5.60 | 0 | none legal |

**For the owner.** (1) PC is a new recipe; PS step 8 and OC step 6 are amendments (the agent
suggested a new code might be preferable). (2) Flat steps kept under rule 2: vision.c's
`IntegralImageTag` hoist, `ComputeObjectAngle` (D, whose text says "the score rises") and two
B steps; fourier.c's five B steps; feature.c's `GetCannyNeighbors`. (3) fourier.c 3ba6b4029
and vision.c's B trio rely on rule 1's same-recipe set. (4) segment.c d797fb70e passes a
`MagickStatusType *status` where PC names `MagickBooleanType *`. (5) segment.c ee7667275
(`GetImageDynamicThreshold`, E) is unreached, verified by guard.

**Noticed upstream, not changed.** `BilevelImage` advances progress twice per row;
`SimilarityImage`'s `threshold_trigger` is reset inside the loop so its early exit is dead,
and it writes `progress++` and `status` unprotected; `GetImageDynamicThreshold` leaks on error
exits and passes `0.2f` where `SegmentImage` passes `0.2`; `Classify` returns `MagickTrue`
whatever `status` holds; `ForwardFourierTransformChannel` tests `magnitude_info == NULL`
before relinquishing it; `InverseFourierTransform` calls `LocaleCompare` without a NULL
check; `IntegralImage` tests `p` twice and never `q`; `HoughLineImage` reports progress under
`CannyEdgeImageTag`; `TraceEdges` shifts `edge.x` and `edge.y` while scanning neighbours.
