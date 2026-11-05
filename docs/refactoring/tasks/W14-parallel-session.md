# W14 - campaign A, parallel lanes (2026-11-05)

Campaign A (Windows desktop, WSL2). After #87 the owner found the serial lane too slow and approved
running four agents in parallel; the main session runs on Opus, the hard whole-function chains go to
Opus subagents and the mechanical ones to Sonnet. Each lane is a git worktree `~/im-w14<x>` on branch
`campaign/W14-<x>` with its own candidate build and its own oracle baseline; the main session merges each
lane's finished file into `campaign/W14` and runs the whole catalogue on the merged tree.

| Lane | Model | File | Score | Commits |
| --- | --- | --- | --- | ---: |
| a | Opus | `fx.c` | 3.52 -> 3.62 | 8 + 2 playbook |
| b | Opus | `distort.c` | 4.25 -> 5.15 | 19 + 1 playbook |
| c | Sonnet | `visual-effects.c` | 4.23 -> 4.59 | 3 + 1 playbook |
| d | Sonnet | `enhance-tone.c` | 5.07 -> 5.80 | 7 |
| d | Sonnet | `effect-blur.c` | 5.95 -> 6.01 | 1 |

Whole catalogue on the merged `campaign/W14`: 15,900 cases, 0 diverged. (Lane a saw four `-list`
divergences in its own worktree; they came from its baseline being built under another checkout's path,
which the oracle does not normalise, and are absent on the merged tree.)

**New playbook text under the mandate, not yet ruled on.** Recipe BR (Extract Block Leaving Through a
Loop Break or a Return; helper returns whether to return and sets `broke` through a pointer; step 7 passes
an outer `broke` on), Recipe MB (Extract Block Through Copies of Macro-Bound Locals: copy each macro-named
struct local in, run the block with the macro text unchanged, copy it out; the `#define`s stay
byte-identical), and Recipe LB's parallel-loop-body form (a block inside a column loop of a parallel loop
body takes its `break;` as `return(MagickFalse);`, `status` by pointer).

**Owner question.** `distort.c` is at 110 functions: may a step raise *Number of Functions in a Single
Module* (about -0.15) when the chain it starts lifts the file further?

**Lessons.** Code Duplication is a cliff: a file sits just under its threshold, a helper for one of two twin
blocks is not flagged, the second is (-0.4 to -0.5). Lanes must keep private oracle baselines: two lanes
building the same baseline in one shared directory corrupted it.

**Noticed upstream, not changed.** `TranslateExpression`'s early returns inside the loop skip
`pfx->teDepth--`; `ImageStat` tests `channel >= 0` where `ExecuteRPN` tests `channel > 0`;
`DistortResizeImage` ends a statement with a comma operator; the macro name `InitalBounds` is misspelt;
`ImplodeImage` and `SwirlImage` compute `scale` in different forms; `SelectiveBlurImage` leaks `kernel`
when its first `CloneImage` fails.

## Lane c, second file: colorspace.c 4.63 -> 6.22 (Opus, 23 commits)

`ConvertGenericToRGB` SD chain (four runs of up to six labels, input struct) leaves Complex and Large Method;
`sRGBTransformImage` leaves Bumpy Road; `TransformsRGBImage` peeled with E, SD, N, PS step 7. Two amendments
under the mandate, not yet ruled on: **Recipe N step 1** (208177141) and **Recipe Z step 1** (f693afeef) now
admit a block outside every OpenMP region that holds whole parallel loops, moved unchanged, with every
variable the region writes or a clause names a parameter or local of the new function by the same name. The
row notes that W13's `sRGBToGeneric` commit (9845ba7e4) already did this before the text admitted it.

Whole catalogue on the merged tree: 15,900 cases, 1 diverged: `infra/0fd402f13a` (10,300 frames to files)
timed out at 30 s. The baseline binary run as candidate times out the same way at load average 21 (four
lanes running), so it is machine load, not the change.

Rejected: a coefficient-table helper for the OHTA/Rec601/Rec709 loops (more than one differing value for
Recipe D; and on x87 builds a literal may keep long-double precision where an array element does not).
Upstream, not changed: the table path of `TransformsRGBImage` returns `MagickTrue` and ignores its row
loop's `status`.

## Lane a, second file: morphology.c 2.71 -> 3.04 (Opus, 9 commits)

`MorphologyApply` leaves Brain Method and Deep Nesting through an SD chain on its stage switch, Recipe I on
two loops, N on two verbose reports and SV on the difference prints. Amendment under the mandate, not yet
ruled on: **Recipe N step 8** (ae16b9045) treats the statements of one loop body as siblings and lets a later
block that holds earlier siblings' calls share their `const Name *`. Six `#if 0` blocks checked unchanged by
script. Whole catalogue on the merged tree: 15,900 cases, 0 diverged.

## Lane b, second file: compare.c stays 2.97 (Opus, no commits)

Saturated; see its plateau row. The lane found clean shared levers (sync helper, trait predicate, head and
mask predicate) that are flat alone; one would need a Recipe P amendment for operands declared only for the
test.

## Lane d, second batch: enhance.c 5.83 -> 7.13, effect.c 6.17 -> 6.75 (Sonnet, 15 commits)

Amendment under the mandate, not yet ruled on: **Recipe NS step 1** (26fb3d849) lets the expression be the
whole right-hand side of an assignment (`sign=c ? 1 : -1;`) at a single site when the step takes its
function out of *Complex Method*, with signed integer literals as `k1` and `k2`. FI steps proven IDENTICAL by
`obj_identical.sh`. A first verification loop in the lane lost its function name in the WSL quoting and
started two empty-name casemap re-indexes; stopped, scripts used afterwards.

## Lane c, third file: quantize.c 5.60 -> 7.38 (Opus, 9 commits incl. two splits)

Two Recipe S splits under the owner's standing go-ahead for splits: `quantize-info.c` (AcquireQuantizeInfo,
CloneQuantizeInfo, DestroyQuantizeInfo, GetQuantizeInfo; 10.0) and `quantize-error.c` (GetImageQuantizeError;
10.0), registered by `tools/register_source.py`. **The Windows build configuration outside the repository must
add both files.** New recipe under the mandate, not yet ruled on: **Recipe HC** (567b22031, Extract Block
Holding a Named Critical Region): a statement of a parallel loop body that holds whole named `omp critical
(name)` constructs moves, constructs unchanged, into a `static void` helper called from that body alone; an
orphaned critical binds to the enclosing region at run time and a named critical excludes every critical of
that name wherever it stands. **Correction:** commit 401d9e9da says quantize.c went from 2,721 to 2,568
lines; the measured count after that commit is 2,664. Upstream, not changed: `SetGrayscaleImage` overwrites
its colormap loop's `status` with `MagickTrue`; the first, unlocked `colormap_index[intensity] < 0` read is a
double-checked race.

## Lane b, third batch: statistic.c 6.73 -> 8.81 (Opus, 12 commits); feature-texture.c unchanged

`statistic.c` has no function-level finding left. `feature-texture.c` stays at 5.25. **Owner question, now
blocking two files** (`feature-texture.c` and `attribute.c`): may a block inside a parallel loop that writes
only the current iteration's own slot of a shared function-local array or struct (`x.direction[i]`,
`census[i]`) move into a helper that takes that slot by pointer? Each iteration writes a slot no other
iteration touches, so the access pattern is unchanged. Upstream, not changed: both `EvaluateImages` loops
`continue` past a failed row without releasing `p`; in `GetImageFeatures` the Q green term divides by
`density_y[x].direction[i].red`, the co-occurrence failure path leaks, and `p+=...;;` has a doubled semicolon.

## Lane d, third batch: segment.c 6.44 -> 6.46; fourier.c parked

`segment.c`: two commits merged. `fourier.c`: five commits (6.33 -> 6.83) are **not merged**. The first moves
compiled `#if defined(MAGICKCORE_HAVE_COMPLEX_H)` blocks together with their `#else` branches, unchanged; the
playbook's *Preprocessor branches* rule allows only never-compiled `#if 0` text to move, because the oracle
build compiles one branch and cannot check the other. **My lane prompt told the agent these branches could be
refactored as ordinary code; that was my error.** The commits are on the local branch `w14-fourier-ifdef`.
**Owner question:** may a function's `#if defined(X)` / `#else` pair move verbatim and unchanged into a helper
when only one branch is compiled and checked by the oracle (the other compiled only by `pp_identical`-style text
comparison)? If yes, the branch can be merged as is.

## Lane a, third batch: threshold.c 6.82 -> 8.03, transform.c 6.42 -> 6.62 (Opus, 28 commits)

No playbook text. `threshold.c` leaves Complex Method, Large Method and *Overall Code Complexity*; the last
needed a measured plan of 13 leaf helpers whose first twelve steps are flat (rule 2, each message says so).
Upstream, not changed: `OrderedDitherImage` leaks `map` when `SetImageStorageClass` fails; `BlackThresholdImage`
switches gray images with `SetImageColorspace` where White and Range use `TransformImageColorspace`, and its
comment says "White threshold image"; `RangeThresholdValue` has a redundant final else-if returning 0 on both arms.

## Lane c, fourth file: distort.c 5.15 -> 5.77 with two splits (Opus, 8 commits)

Two Recipe S splits: `distort-resize.c` (9.92) and `distort-affine.c` (10.0). **The Windows build configuration
outside the repository must add both files.** `SparseColorImage` left every category except its public
argument count. **Owner question, measured:** `distort.c` is at 110 functions again. A 111th function raises
*Number of Functions in a Single Module* once (-0.17; probes at 111, 115 and 120 functions all score 5.60), and
finishing `DistortImage` is worth about +0.95. May the `DistortImage` chain take the file past 110 functions?
Upstream, not changed: `SparseColorImage`'s clone and storage-class failure exits return without freeing `coeff`.

## Lane d, fourth batch: paint.c 7.02 -> 7.24 (Sonnet, 4 commits); channel.c unchanged

Upstream, not changed: in a `ChannelFxImage` assign operation `destination_channel` takes `i`, the source
channel index, because `i` is not reparsed; `FloodfillPaintImage` has an unused `pixel` local.

## Lane b, fourth batch: vision.c 7.15 -> 9.31, feature.c 7.40 -> 9.09 (Opus, 26 code commits)

Amendments under the mandate, not yet ruled on: **Recipe TW** (06483388e: one function only, when the other copy
is the whole body of an existing static helper, the helper's body becomes the fill and call) and **Recipe N step
1** (d9d90aad9: an orphaned `omp atomic` construct, in no parallel region of the function, may lie inside the
block and move whole and unchanged; it binds to the enclosing region at run time). **Corrections to commit
messages:** in 63a152baa and e43e56658 "cc 9 -> 8" was inferred from the Complex Method finding disappearing, not
read from a tool (read: "leaves Complex Method"); 91302c0fe moves a whole parallel loop under N step 1's form
but carries the trailer `Recipe: E`. Upstream, not changed: `WriteHoughFile` ignores `WriteHoughLines`' status;
`SetCannyEdgeGray` destroys only its local copy of `edge_image` on failure.

## Lane d, fifth batch: three more files at 10.0 (Sonnet, 23 commits)

`decorate-raise.c` 8.87 -> **10.0**, `channel-combine.c` 9.34 -> **10.0**, `quantize-posterize.c` 9.47 -> **10.0**,
`threshold-adaptive.c` 9.13 -> 9.68 and `statistic-image.c` 9.09 -> 9.68 (both held only by a public 5-argument
signature). **Temporary drops:** fifteen commits (the first five of `decorate-raise.c`, `threshold-adaptive.c`
and `channel-combine.c`) lowered their file's score as early steps of a plan that ends higher, and were
committed with `--no-verify` under the owner's ruling of 2026-10-29 ("You can accept a temporary drop. We
focus on the long term goal of CH 10"); every plan ended at or above its starting score. **Correction:**
`decorate-raise.c` messages 1-5 say the plan ends with FI; it ended with PC (FI is illegal there). Upstream, not
changed: `CombineImageRow` leaks its per-channel view on the `p == NULL` continue; `PosterizeImage` overwrites
its row loop's `status` with `QuantizePosterizeImage`'s result.

## Lane a, third file: visual-effects.c 4.59 -> 7.33 with three splits (Opus, 48 commits)

New files: `visual-effects-wavelet.c` (9.55), `visual-effects-plasma.c` (8.50), `visual-effects-stegano.c` (9.09);
**the Windows build configuration must add all three** (the original is not in `Make.com` or
`format_c_api_docs` upstream, so the split-offs are not either). Amendments under the mandate, not yet ruled on:
**Recipe S** (2b2bbb75b: a forward declaration and a file-local typedef travel with the moved functions; a
one-line progress-tag `#define` is defined again verbatim in the new file) and **Recipe V step 6** (98e7a7ee3: a
caller variable the block writes only as `w=F(w);` before a `return(c);` goes by value). **Tool changes, for the
other campaign to pick up:** `tools/split_source.py` (in 2b2bbb75b) handles those cases; `tools/register_source.py`
(inside the split commit 806c5d3fb, not in a commit of its own as AGENTS.md asks) skips `Make.com` when the
original is not listed there. Upstream, not changed: `WaveletDenoiseImage` reports progress under the AddNoise
tag; `MorphImages` leaks `morph_images` when `SetImageStorageClass` fails on a resized frame.

## Lane c, sixth file: shear-rotate.c 7.63 -> 10.0, shear.c 8.03 -> 8.61 (Opus, 16 commits)

Amendments under the mandate, not yet ruled on: **PS step 7** (e0c450691: a sequential loop nested in the
parallel iteration whose whole body is the block), **PC step 1** (582859690: the head of a nested sequential
loop's body when the tail holds an `omp critical`), **HC step 2** (e66a49696: the block may read, test and assign
the shared `status` outside the critical constructs, by pointer), **HC step 3** (61d1601d8: a callee that may take
another lock is allowed because the construct moves whole and unchanged), **LB step 1** (f3711acaa: a PS step 7
helper's body counts as the loop body). **Review:** 9cd200adc is a rule 1 set that mixes six recipes on
`IntegralRotateImage`; the 2026-10-31 amendment admits mixed recipes when *Overall Code Complexity* lowers each
block alone; here the measured chain goes 7.64 -> 7.38 -> ... -> 7.65, so check it against rule 1's text.
*Overall Code Complexity* is a finding of the file from 9cd200adc to 8cddcdd7d and leaves at bb9e8ad41.
**Correction:** 9cd200adc says "271 to 198 lines"; the measured value before that step is 270. Upstream, not
changed: `YShearImage` reports progress against `image->rows` over a width loop; X/YShear skip sync and progress on
rows with zero displacement.

## Lane d, sixth batch: attribute.c and attribute-bounds.c to 10.0 (Sonnet, 10 commits)

New recipe under the mandate, not yet ruled on: **Recipe PD** (d028778f4, Name a Parameter-or-Default Selection):
three or more sites of `(n >= k) ? values[k-1] : d` in a file become calls of one `static inline` helper with the
same ternary; checked by script and by the oracle (the object code differs in branch order). **Review:** be5af5089
(`attribute-bounds.c`) and 2ed22bd52 (`resize.c`) move a function's tail that holds a whole parallel loop under
Recipe E step 7; the text that admits a block carrying whole parallel loops is Recipe N and Z step 1 (amended this
morning), not E: check whether E should be amended to match or these should be relabelled N. The two
`--no-verify` messages in `statistic-function.c` say "of 6" steps; the plan took 5 and its measured end, 9.68, held.

## Lane a, fourth batch: colorspace.c 6.22 -> 7.14 merged; enhance-tone.c splits parked

`colorspace.c`: the `Convert*` family split into `colorspace-convert.c` (7.14; **add it to the Windows build
configuration**) and one E step. **Parked, owner decision:** five `enhance-tone.c` splits take that file to 10.0
(new files: level 8.79, stretch 9.38, equalize 9.92, clut 10.0, gray 10.0), but they need a Recipe S amendment
(2ad9eb859) and a tool option (`split_source.py --copy-static`, 316ec54d6) that copy short shared static helpers
verbatim into each new file, so the same three helpers and one typedef would exist in six files that must change
together. CodeScene measures duplication within a file, so the copies are invisible to it. The agent's
alternative: a `MagickCore/enhance-tone-private.h` with `static inline` helpers included by each file. On the
local branch `w14-tone-splits`.

## Lane b: fx.c chain parked

Lane b took `fx.c` from 3.62 to 5.93 (58 commits, oracle clean), but most of it is an SD chain that splits
`ExecuteRPN`'s operator switch into `ExecuteRpnPart01`..`ExecuteRpnPart32`, with case groupings chosen by a search
for arrangements that CodeScene does not flag as clones (unrelated operators per helper, not adjacent source
runs), and each operator dispatch now walks a chain of up to 32 nested calls in the `-fx` per-pixel interpreter.
Parked on the local branch `w14-fx-annealed`, with a Recipe P amendment and later small steps built on it. **Owner
decision:** rework along the code's own structure, or accept.

## Lane d, seventh batch: distort-resize.c and visual-effects-wavelet.c to 10.0 (Sonnet, 8 commits)

`statistic.c` 8.81 -> 9.38, `visual-effects-stegano.c` 9.09 -> 9.68 (public signature left). **Review:**
e27e601a5 (`distort-resize.c`, Recipe E) also merges the alpha branch's own `if (resize_image == NULL)
return(NULL);` into one check after the if/else and drops that branch's one-line comment; behaviour is the same
(both branches assign `resize_image`, the single check tests it on the same paths), but the merge of two identical
checks is not a listed recipe step. One commit to revert if unwanted. Upstream, not changed: `PlasmaImageProxy`'s
Right block returns MagickFalse on a missing pixel where Left, Bottom and Top return MagickTrue; `*status=Sync...`
overwrites an earlier failure there and in `SetPlasmaMiddlePixel`.
