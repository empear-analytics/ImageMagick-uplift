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

## Lane c, seventh batch: montage.c 7.66 -> 8.56, resample.c 7.42 -> 8.01, layer.c 7.08 -> 7.10 (Opus, 15 commits)

**Review:** de56319d6 (`resample.c`, Recipe D) moves the shared assert-and-log preamble of
`SetResampleFilterInterpolateMethod` and `SetResampleFilterVirtualPixelMethod` into `CheckResampleFilter`; its trace
log now names the helper through `GetMagickModule()` (the oracle normalises it, as the playbook accepts for thrown
warnings), and its subject line says "deduplicate" instead of "simplify <Function>". Suggested amendment (not
written): Recipe Z step 7 for four written variables plus an input struct. Upstream, not changed:
`MontageImageList`'s `ThrowImageException` after a failed `CloneImage` (and at the `AcquireMontageStrings` throw)
reads `image->filename` with `image` NULL, a NULL dereference; the tile loop's progress monitor is never restored;
`tiles_per_page` is assigned twice in a row; `resample.c`'s `#if DEBUG_NO_PIXEL_HIT` branch returns `status`
unassigned.

## Lane d, eighth batch: paint.c 7.24 -> 8.74 with a split (Sonnet, 2 commits + 1 tool fix)

New file `paint-floodfill.c` (8.33): **add it to the Windows build configuration.** Tool fix in its own commit
(e40c9238a, `tools/split_source.py`): a function's end was taken as the first column-0 `}`, which is also the
closing brace of a multi-line macro inside the function; and the include-comment lookup crashed on a one-space
"Include declarations." line. Campaign B should pick it up. Upstream, not changed: `RangeThresholdValue` ends in
`if (pixel > high_black) return 0; else return 0;`; the first `status=MagickTrue;` in `FloodfillPaintImage` is dead.

## Lane a, fifth batch: segment.c 6.46 -> 7.02, transform.c 6.62 -> 7.39 (Opus, 13 commits)

New recipe under the mandate, not yet ruled on: **Recipe RP** (d066388ac, Extract Progress Loop Returning the
Progress): a sequential pass in no parallel region, whose only pragmas are orphaned `omp atomic` constructs on
`progress` moved whole, becomes a function returning `progress`; `status` goes by pointer; the caller writes
`progress=Helper(...,&status,...);`. **Review:** the two tail N steps in `transform.c` (bde214f54..58bcb2cd9) take
the file's existing `ChopRowImages` and `SpliceRow` typedefs as their input structs (same members, types and
order) rather than adding duplicates; a reading of N step 3. `quantize.c` unchanged (no legal split left).

## Lane b, fifth batch: effect.c 6.75 -> 9.09, effect-blur.c 6.01 -> 7.36, enhance.c 7.13 -> 7.49 (Opus, 23 commits)

Two splits: `effect-preview.c` (8.40) and `enhance-clahe.c` (8.71); **add both to the Windows build
configuration.** Tool fix in its own commit (df35436d9): `tools/split_source.py` strips comments before building
the call graph (the word "Contrast" in a comment counted as a call of the static `Contrast`). Upstream, not
changed: `effect-blur.c` defines `MagickSigma` twice, identically.

## Lane b, sixth batch: compare.c and morphology.c cannot be split for a real gain (Opus, 1 tool commit)

Tool fix in its own commit (1596ecbce, `tools/split_source.py`): a column-0 brace inside a function no longer
ends it (the end is where the brace depth returns to zero, ignoring braces in `#else`/`#elif` branches), and
anonymous `typedef struct`s between moved functions are accepted. Across every MagickCore file the function
boundaries found are unchanged except `SimilarityImage`, which the old tool cut short at a column-0 `{` block
inside its HDRI/FFTW `#if`; dry runs the old tool accepted produce byte-identical files. Campaign B should pick
it up. The agent rejected the one split that would lift `compare.c` to 8.92, because it moves the whole problem
into a new file that scores 2.97.

## Lane c, eighth batch: visual-effects.c 7.33 -> 8.54, colorspace-convert.c 7.14 -> 9.08 (Opus, 11 commits)

New file `colorspace-generic-rgb.c` (9.68): **add it to the Windows build configuration.** The oracle caught a bug in
a draft of one SD step (67 cases diverged; `&input` passed where step 8 passes `input`), fixed before the commit.
Upstream, not changed: `MorphImages` passes an uninitialised `status` to `MorphSingleImage`, which only writes it;
`MorphSequenceFrames` leaks the morph list when `SetImageStorageClass` fails.

## Lane c, ninth batch: channel-alpha.c 8.96 -> 10.0 (Opus, 5 commits + 1 playbook)

New recipe under the mandate, not yet ruled on: **Recipe ST** (9d344362e, Split a Selection at its Default,
Copying the Return Tail): the `default:` group of a `switch` moves with the groups next to it, as a run in source
order, into a `static` function of the caller's return type whose body is the switch over those groups followed by
a copy of the function's final `return(R_T);`; the caller's `default:` becomes `return(Helper(...));`. **Review:**
672d2040f is labelled Recipe N step 1 although the moved tail reads only three inputs and needs no input struct.
`threshold.c` and `resample.c` unchanged (no legal split or step).

## Lane d, ninth batch: channel.c 6.93 -> 8.06, paint-floodfill.c 8.33 -> 8.35 (Sonnet, 27 code commits)

Amendments under the mandate, not yet ruled on: **Recipe SR step 5** (79d6a9363: in a chain link, `default:
return(Helper(...));` replaces the if/return/break group when the helper returns only MagickTrue or MagickFalse) and
**Recipe B step 5** (17adead2c: calls in a loop of a function with no OpenMP region of its own may take a struct
filled once before the loop, the form OC step 6 and LB step 4 use). **Temporary drop:** 76a617e62 took
`channel.c` from 6.93 to 6.06 with `--no-verify` as step 1 of a 22-step plan measured end to end (7.81 planned,
8.06 reached), a larger dip than earlier ones. Upstream, not changed: `ChannelFxImage`'s `;` case and setup failure
paths leak (`destination_image=GetLastImageInList(...)` then `return NULL`).

## Lane b, seventh batch: effect-preview.c to 10.0, enhance-clahe.c 8.71 -> 9.68 (Opus, 19 commits)

Amendment under the mandate, not yet ruled on: **Recipe SD step 8** (7e261e567): when the groups one step moves
write five or six caller variables, the helper takes Z step 7's `const Targets *targets` after `tile`.
**Review:** cf17c9d4c (`PreviewImage`'s tile loop, N) moves the two const structs of addresses with their
initialisers into the helper, because every variable they point to moves too; E step 4 was cited, but no sentence
covers this exactly. Upstream, not changed: the Threshold preview runs `BilevelImage` on the thumbnail after the
tile has been cloned from it, so the tile shows the unthresholded image.

## Lane a, sixth batch: composite.c 3.02 -> 3.08 (Opus, 15 commits)

**Review:** step 13 (b6abcf5f0) keeps the cursors `p` and `q` as members of a per-row `BlurRow` struct inside a Z
block, using N step 2's read-then-written clause, while Z step 2 says a written variable is never a member; steps
4-7 use the member form in the four `DisplaceSetup` helpers because the const-local form made the x and y halves of
the Distort centre code a clone pair (N's Careful item sanctions this). Upstream, not changed: `white_luminance` is
set at its declaration and again before use; the Displace path calls `SetGeometryInfo` twice.

## Lane c, tenth batch: colorspace.c 7.14 -> 7.35, enhance.c 7.49 -> 7.72 (Opus, 7 commits)

Amendment under the mandate, not yet ruled on: **Recipe SW steps 2 and 3** (8393d2d77): a group of the moved run
may hold whole parallel loops under N step 1's conditions, and when the tail begins with the fill of an N input
struct, that struct is shared (the fill stays in the caller, the helper takes `const Name *`). The lane declined
two higher-scoring groupings (7.37, 7.39) as clone-detector dodging. `layer.c` unchanged.

## Lane d, tenth batch: segment.c 7.02 -> 7.09 (Sonnet, 3 guard-verified commits)

Upstream, possibly a bug, not changed: in `GetImageDynamicThreshold`, `extrema[i]` is allocated with
`sizeof(**histogram)` (8 bytes per entry) where its element type is `short`; `AcquireSegmentHistograms` uses the
right size. **Option for the owner:** no oracle case reaches `GetImageDynamicThreshold` (no command-line option
calls it); a driver case through `imdriver` would let Recipes I, V, W and PC reach the function, which a stub puts
at about 9.

## Lane a, seventh batch: montage.c 8.56 -> 8.72 with a split (Opus, 4 code commits + 1 tool + 1 playbook)

New file `montage-info.c` (10.0): **add it to the Windows build configuration.** Tool fix in its own commit
(36da89909, `tools/register_source.py`): it also copied `MagickWand/montage.c`'s `Makefile.in` entries under the new
name, which broke configure; it now registers MagickCore entries only. Campaign B should pick it up. Amendment under
the mandate, not yet ruled on: **Recipe I step 2** (551c09acc): a temporary whose declaration in the new function
would leave a variable written but never read (clang's `-Wunused-but-set-variable`, an error in CI) is a written
variable of the step instead. `transform.c` unchanged.

## Lane c, eleventh batch: the honest fx.c rework (Opus, 7 flat commits)

Seven commits take GetOperator, GetProperty, AcquireFxInfoPrivate, DumpTables, GetOperand, DumpRPN and FxImage out
of one category each (flat, rule 2); `fx.c` stays 3.62. **ExecuteRPN, measured and not committed:** a split that
follows the code (20 SD links of adjacent groups in source order, named CopyControl, ConditionalJump, Jump,
LoopFunction, PixelFunction..., Arithmetic, AssignOrValue; one shared input struct) is finding-free and clone-free and
lifts the file to 3.74 (5.34 once the five other big functions are done), but makes `-fx` about **17% slower** per
pixel (median 0.183 s against 0.156 s, 9 runs each); the three control links alone cost 12%. The parked annealed
chain (`w14-fx-annealed`, 5.93) has the same call-chain structure, so it would cost at least as much. **Owner
decision:** accept a slower `-fx` for the score, or leave `ExecuteRPN` whole. Note: `DestroyFxInfoOnFailure` now
has one shared call for four failure returns (call guard -3), stated in 36b1ea6da.
