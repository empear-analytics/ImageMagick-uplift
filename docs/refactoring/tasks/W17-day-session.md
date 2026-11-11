# W17 day session (2026-11-11)

Batch cut from origin/main d38e0c1b3 after #92 (W16) merged. Four lanes in worktrees `~/im-w14a..d` on
`campaign/W17-a..d`; each finished lane merged into `campaign/W17` and gated on the whole catalogue (and
`tools/ci_linux_warnings.sh --campaign-a`) before every push. Scores from cs-mcp 1.5.16. Start (sweep of
d38e0c1b3): campaign A 76 files, mean 8.60, 30 at 10.0.

Lane set-up: lanes b and c carry their unfinished W16 work (`campaign/W16-b` xwindow.c, `campaign/W16-c`
display.c), merged with main; lanes a and d start clean. The lane check (`verify_wt.sh`) now runs
`tools/ci_linux_warnings.sh <file>` on every step.

## Lane a: near-10 files (merged)

| File | Score | Commits |
| --- | --- | --- |
| `effect.c` | 9.09 -> **10.0** | 99aa3f82b S (Despeckle, Kuwahara, LocalContrast, Shade, Spread, UnsharpMask to `effect-filter.c`, 9.09; they share `ReleaseFailedImage`); 276846f80 E `FillEdgeKernel` in EdgeImage, 9.48 -> 10.0 |
| `visual-effects.c` | 9.09 -> **10.0** | 4c22f59d2 S (15 publics to `visual-effects-pixel.c`, 9.09; tied by `SetEffectProgress`, `ReleaseFailedImage`, `IsPixelChannelPairUndefined`, the shadow statics); CharcoalImage stays |
| `colorspace.c` | 9.38 -> **10.0** | 2c5443dbf tool (`split_source.py` moves family-only tables, macros and forward declarations; output for earlier splits byte-identical); 47dcd2cfc Recipe S step 2 amendment; b21287531 S (SetImageColorspace, TransformImageColorspace, `GenericColorspaces`/`YCCMap`, `MaximumLogarithmicColorspace` to `colorspace-transform.c`, 9.38) |

The S amendment (Decided by `owner`, under the 2026-11-10 OK to evolve the recipes) settles W16's open
question on family-only `static const` tables and a numeric macro. **For the owner:** the effect.c and
visual-effects.c splits keep one public function in the original (EdgeImage, CharcoalImage); the original
reaches 10.0 and the new file inherits the old score. They are legal under the 2026-11-07 ruling (new file
scores at least the original) but amount to relabelling most of the file: should they stand?

Plateaus (no commit): enhance.c 9.09 (the legal split leaves the kept file at 8.95: the AutoLevel/Normalize
pair is a Code Duplication finding, and moving one of them alone would be a clone-dodging grouping; it also
needs function-pointer typedefs, which split_source.py refuses); feature.c 9.09 (all three publics take more
than 4 arguments); resize.c 9.09 (`LiquidRescaleImage`, 6 arguments, has an uncompiled `#else` stub);
threshold.c 9.09 (the map family needs `BuiltinMap` inside `#if MAGICKCORE_ZERO_CONFIGURATION_SUPPORT`);
statistic.c 9.38 (one family); resize-image.c 9.38 (one public, 5 arguments); transform-region.c 9.38 (all
nine publics share `CopyPixelChannelsTo`); vision-components.c 9.38 (one public).

Whole catalogue on the lane tip and on the merged tree: 15,900 cases, 0 diverged.

## Lane d: compare.c (no commit)

W17 lane d (2026-11-11): compare.c, no commit, 2.97. Lane b's full peeled end state of all eleven `Get*Similarity` re-scored at 2.80: the peeled skeletons (prologue, pragma, head, named critical, teardown) clone each other and RL cannot merge them without pragma edits; DPC/NCC (4 bumps), PHASE and SSIM stay Large; the rest of the ceiling sits in the `#if defined(MAGICKCORE_HDRI_SUPPORT) && defined(MAGICKCORE_FFTW_DELEGATE)` region (lines 3324-5146), which may not be moved or edited. The only split split_source.py accepts that moves compare.c (whole family: compare.c 8.92, new file 2.97) carries that region: owner question whether a whole compiled `#if defined` block may move unchanged in a Recipe S split. Whole catalogue 15,900 cases, 0 diverged. Possible upstream: FUZZ/MSE/MEPP do not memset `similarity`; DPC/NCC lack the `status == MagickFalse` guard at the top of the row loop.

## Lane b: xwindow.c (wide build) 1.57 -> 1.64 (merged)

The branch carried 21 W16 xwindow.c commits plus playbook amendment 18b5b68d4 and tools commit dc4c85873
(all with valid trailers). W16's commit e3fe04cac had been a temporary drop (1.59 -> 1.57, the 17th new
function) whose plan never recovered it; W17 finished that plan and carried on through `XMakeImageMSBFirst`.

- `XMakeImageLSBFirst` (514 cases, 0 diverged): 8c7da35f8 N flat (IDENTICAL); 5fd57f026 N 1.57 -> 1.58
  (unreached lines: obj_modulo MODULO-IDENTICAL with `XMakeImage` as caller); 9e1923c89 N flat, depth 5 -> 4.
- `XMakeImageMSBFirst` (no reaching case, all `Verified: guard`): six E row helpers flat with object code
  IDENTICAL (55a3ce8a9, e31be1240, 9ffff203d, c55c21f6e, 48cb6cbe2, b3921fda3); 8a4ebbf3c N flat; 639495ed1 N
  1.58 -> 1.60; eight N siblings flat (28aa84ad3, 15933aeda, 0a27813ae, 775918cf2, 33da40051, c65562a73,
  664e6fc29, 507915b88); c9f48ae64 N 1.60 -> 1.64; 8df842ade N flat, depth 5 -> 4. The function went from
  cc 90 to 38, 22 to 8 bumps, 485 to 221 lines; no new helper has a finding, no Code Duplication.
- Recipe and tool text (Decided by `owner`, 2026-11-10 OK): f4d805591 (N, unreached item 3: what
  `sibling_fill_check.py` may list; step 8: a second function may reuse the sibling typedef an earlier
  function declared), d2646c6f5 (N: a declarator that leaves under E step 4 is an allowed move-check line),
  757ff33ad and 2f06d251f (`obj_modulo.sh` ignores trailing alignment padding after a function's last
  instruction; a planted change is still DIFFERENT).
- Whole wide catalogue: 14,145 cases, 1 diverged (`infra/0fd402f13a`, timed out on the baseline; 0 on rerun).
- Left: the XYBitmap branches (a row helper needs six inputs; a whole-branch helper is cc 10), matte helpers
  (flat), and the rest of the file in `XGetWindowImage`, `XMakeMagnifyImage`, `XMakeStandardColormap`,
  `XMakeImage`. Every MSB case helper differs from its LSB twin only in the row function: a CF candidate once
  function pointers are ruled on.
- Possible upstream: `XMakeImageLSBFirst` composites the checkerboard with `MagickTrue` where MSB uses
  `MagickFalse`; the immutable test is inverted between the two; MSB's multi-byte colour-mapped comment says
  "8 bit".

## Lane c: display.c (wide build) 1.02 -> 1.12 (merged)

The branch carried 10 W16 display.c commits and a Recipe P amendment; `WIDE=1` verify passes on every function
they touched (XTileImage has no reaching case and was already `Verified: guard`). With 14 Brain Methods the
score reacts only to a function leaving Brain Method (about +0.014) or losing a nesting level (about +0.002).

New: e90cca81f G `XVisualDirectoryImage` (unreached, IDENTICAL); 5d5cb5b71 D `XCropImage` (three identical
help switches to `XShowCropHelp`); 30de7ba92 E `XColorEditImage`; d6ecc4e6d E `XMatteEditImage`; f4d43a402 CN
and 22fc2fbf0 E `XDisplayImage` (317 cases; the moved `-update` lines are unreached per lcov, object code
MODULO-IDENTICAL); ef28cd47f, 9762225d2, 9547d88d0, e78b7c3fa E x4 `XImageWindowCommand` (leaves Brain Method);
2893053a2 and 108edf0ca E `XChopImage` (leaves Brain Method); 1caefcc6f N and ab630cbb4 D `XPasteImage`
(reuses `XTrackInfoWidget`; leaves Brain Method). Flat steps name the step that recovers them.

Whole wide catalogue: 14,145 cases, 1 diverged (`infra/0fd402f13a`, times out on the baseline).

Plateau: the remaining Brain Methods hang on blocks writing `static` locals (`XImageCache` `undo_image`,
`XCompositeImage` `compose`/`factor`/`blend`, `XRotateImage` and `XDrawEditImage` `static color_name` blocks
leaving their case by `break`), which O, I and E step 4 refuse. **For the owner:** an LB amendment letting a
`switch` serve as its loop would open the `color_name` blocks. Possible upstream: XCropImage's help titles
read "Image Cropg" and "Image Cutg".

## Lane d: mid-band files (merged)

| File | Score | Commits |
| --- | --- | --- |
| `montage.c` | 8.72 -> 9.11 | 1d822244a playbook (new Recipe FB); c6f101a4f FB `MontageImageList` (`bounds_info`, `page_info`, `placement`), 8.72 -> 8.86, IDENTICAL; 849992f7b playbook (Z and Y step 5); d3a4a6774 Z `WrapMontageRow`, 8.88; d28f25469 playbook (I step 8); a61b8a086 I `CopyMontageTiles`, 9.03; 228d7ff3c playbook (FI and FB step 2); 7abacea9f FI and 71424e5bc FB `PlaceMontageTile`, flat, IDENTICAL (they free the lines the next step needs: without them the file crosses 1,000 lines, 8.55 in a probe); e0f4fe8e0 I `AdvanceMontagePage`, 9.11, leaves Bumpy Road. 49 cases, 0 diverged, each MontageImageList step. |
| `magnify.c` | 8.62 -> 8.84 | fd2985f35 FB `Fish2XBottomRightPixel` (`ladder`), flat, IDENTICAL, frees 6 lines; 73bbb958b NS `Eagle3X` (`Eagle3XTopLeftSource`), 8.84, IDENTICAL (no timing needed: object code unchanged) |

New recipe text (Decided by `owner`, 2026-11-10 OK): **Recipe FB** (a run of member assignments filling a
struct used only by one call becomes a `const` struct initialised in a compound statement around the call;
steps 1-3 as FI's, values may be written before the run; `obj_identical.sh` IDENTICAL), Z and Y step 5 (the
fill may take FB's form), I step 8 (MagickCore's throw macros may be the `M` that stays in the caller, so
`GetMagickModule()` still names the caller), FI and FB step 2 (reads through a pointer-to-`const` parameter).

Plateaus: montage.c (MontageImageList cc 10 and Large Method 164 lines with about one line left under 1,000;
lowering cc costs about 13 lines); magnify.c (NS on Xbr2X reaches 9.07 but crosses 1,000 lines; the other
fills change object code); segment.c 9.02 (both bumps of the unreached `GetImageDynamicThreshold` give only
9.09); colorspace-convert.c 9.08 (the mirrored SD chain clones through shared binding lines, 8.28);
quantize.c 8.74 (125 functions, split_source.py refuses every family); resample.c 8.53 (remaining bumps in
`#if FILTER_LUT`/`DEBUG_*` code); visual-effects-plasma.c 8.50 (edge-block helpers clone each other);
paint-floodfill.c 8.35 (every bump holds the `PushSegmentStack` macro).

Possible upstream: montage.c `ThrowImageException` after a failed `CloneImage` dereferences a NULL `image`; the
`progress_monitor` saved in the tile loop is never restored; `tiles_per_page` is computed twice. **For the
owner** (from W14): segment.c's `ThrowClusterAllocationFailed`, moved under E step 6, now names the helper as
the exception's function in the unreached `GetImageDynamicThreshold`.

**Gate on the merge (ed4dff2aa):** 15,900 cases, 1 diverged, `infra/0fd402f13a` (`-size 1x1 xc:gray -duplicate
10299 +adjoin f%d.pgm`) timing out at the oracle's 30 s limit on the candidate, and again on two reruns, with
the load average at 18-28 while lanes b and c ran. Run by hand at the same load, both binaries finish and write
all 10,300 files: baseline 57.9 s, candidate 27.0 s, exit 0 each. The case is file-system bound and touches no
file this merge changed; recorded as a load timeout.

## Lane a, second round: layer, distort, attribute-depth, feature-texture, fourier (merged up to d5c4d00d9)

| File | Score | Commits |
| --- | --- | --- |
| `layer.c` | 7.98 -> 8.00 | 3ea4c337f E `AcquireFrameBounds`, 32ff96881 E `AcquireFrameDisposals` (OptimizeLayerFrames) |
| `feature-texture.c` | 7.53 -> 7.60 | c26c07ff4 PC `FetchFeatureGrayRow`; f14e9a20f D (a) `RelinquishFeatureGrays`; 805763e23 D (a) `DestroyFeatureMatrix`; 5649f016f E sibling run `ZeroFeatureDensities` (GetImageFeatures) |
| `fourier.c` | 6.52 -> 7.59 | ee98b70cc, ef3969b4c, 720ba43d2 E (ForwardFourierTransform, no findings left), 7d2507db9 E (InverseFourier leaves Large Method), ec40c537c E step 7 and 2c33273fd D (a) (ComplexImages); 21772299f playbook and a74ffbef6 tool (Recipe S step 2: a trailing run of conditional delegate includes that holds every `#define` of the include block is left out of the new file when no line of the family names one of its macros); d5c4d00d9 S `ComplexImages` and 13 statics to `fourier-complex.c` (9.50, about a quarter of the file; the new file has no preprocessor branch) |

`distort.c` 7.60, no commit (`poly_basis_dx` on the hot path, measured 5-20% slower in table/chain forms;
`SetDistortNoBestFit` label deletion is an owner question; the tool refuses any split because
`GenerateCoefficients` is shared). Plateaus: layer.c (`#if DEBUG_OPT_FRAME` blocks and the
`ThrowImageException` validation), feature-texture.c (the grays column loop writing the shared table, an owner
question; one public function), fourier.c (the transforms sit in the `#else` of `#if !defined(ENABLE_FFTW_DELEGATE)`).

**Held back, for the owner: the attribute-depth.c split** (492c52f3b, branch `w17-attribute-depth-split`,
7.56 -> 8.16, new `attribute-depth-measure.c` 8.16). It moves `GetImageDepth` whole, including its
`#if !defined(MAGICKCORE_HDRI_SUPPORT)` branch, verbatim. The oracle's HDRI build does not compile that branch;
AGENTS.md forbids touching such code, with only `#if 0` excepted. The non-HDRI CI configurations do compile it,
and there the 27 functions the new file defines disassemble the same as in the original (1,367 lines; the only
differences are the `__LINE__` immediates, shifted by 6, `__FILE__`/constant-pool references and one
rip-relative offset). **The same question covers effect.c's split (99aa3f82b, merged above):** it moved three
`#if defined(MAGICKCORE_OPENCL_SUPPORT)` branches verbatim. In the wide build (OpenCL on), the moved functions
disassemble the same (3,434 lines; only `__LINE__` immediates and constant-pool label numbers differ). Should a
split move a compiled-elsewhere branch verbatim when the build that compiles it shows the same instructions? If
not, effect.c's split is reverted and the attribute-depth.c split dropped.

Possible upstream: feature-texture.c `GetImageFeatures` leaks `density_x`, `density_xy`, `density_y`, `Q` and
`sum` when `FillFeatureCooccurrence` fails; its parallel grays fill writes the shared `grays[]` from several
threads (a race, every write storing the same value).

## Owner ruling (2026-11-11): splits may move a compiled-elsewhere branch whole

The owner answered "Yes, allow it" to the question above. Recorded in f9589c422: AGENTS.md's second
exception, the playbook's *Preprocessor branches* section and a Recipe S row; new
`tools/split_branch_check.py` compiles the original at the split's parent and the new file with a build's flags
(`cand-wide`, `ci-gcc-min`, `ci-gcc-dlg`) and prints `IDENTICAL-MODULO-LAYOUT` only when every function the new
file defines differs at most in `__LINE__` immediates, `.rodata` references, constant-pool label numbers and
rip-relative offsets (a planted one-constant change prints DIFFERENT). effect.c's split stands (wide build: 40
`__LINE__` immediates, 76 labels). The attribute-depth.c split is merged (e38bd8ebd): `attribute-depth.c`
7.56 -> 8.16, `attribute-depth-measure.c` 8.16; `split_branch_check.py` in ci-gcc-min and ci-gcc-dlg:
IDENTICAL-MODULO-LAYOUT (6 `.rodata` references, 5 `__LINE__` immediates, 1 rip-relative offset). Gate:
15,900 cases, 0 diverged (`infra/0fd402f13a` load timeout, passed on rerun).

## Lane c, second round: animate.c 2.49 -> 3.35, opencl.c 5.03 -> 5.27 (wide build, merged)

animate.c, 25 refactor commits. `XAnimateBackgroundImage`: 87cc80834 E (flat), 513e342ff LB 2.59 (leaves Deep
Nesting). `XAnimateImages` (30 cases): every statement above depth 3 went into a helper; raising steps 36941f57c
E 2.65 (after 4afb47b2c D, flat), 1e0d6d9ba N 2.71, 030d72d4c N 2.77, 01421bbf1 P 2.90, 6a4402a7c D 3.35 (reuses
`XFocusBackdropImage`; leaves Brain Method and Deep Nesting); flat plan steps e35ba63ee CN, c7272670c,
62c30529c, d5567bf9f, 7d682080e E, 51f38b5a4, 858de5eed, 82d8d50d3, dddf26448, 24012b30b Z, 749f09c70,
a24867657 N, d02830403 H (pp check against the wide build: IDENTICAL), f1f0cda1b LB, 66112b45f, 58d44db8f,
e1ffaebdf I. **Correction:** e35ba63ee (CN) gave `XAnimateImages` a Complex Conditional finding at a flat score
and its message does not say so; 01421bbf1 (P) removes it and says so. 4afb47b2c (D) is the one step with
DIFFERENT object code (`FindColormapDifference` no longer inlined into `XAnimateBackgroundImage`); it rests on a
text check and the oracle.

opencl.c: c3ff375d9 E `RecordProfileData` 5.22 (leaves Large Method); 2e14e50ba N `LoadOpenCLDevices` 5.27
(leaves Bumpy Road, 253 cases). Rejected: BenchmarkOpenCLDevices' score loop (5.11, a Code Duplication twin of
`EnableSameOpenCLDevices`). Stopped early to save context; `LoadOpenCLDevices`' platform loop is next.

Recipe text (Decided by `owner`, 2026-11-10 OK): ef17bfab2 Z step 7 (five or six caller variables, or two to
four whose pointers would bring the call to five or more arguments, go by one `const` struct of addresses);
64ab6430e and 793736b19 N step 2 (a caller array used only as `v[e]` reads or passed whole to a pointer
parameter may join the group as a pointer member).

Whole wide catalogue on the lane: 14,145 cases, 1 diverged (`infra/0fd402f13a`, times out on the baseline).

For the owner: `tools/pp_identical.sh` uses the narrow build, so it proves nothing for wide-build files (lane c
ran a copy pointed at `cand-wide`); `tools/ci_warnings.sh` fails on opencl.c before any change
(`-Wdeclaration-after-statement` in the `#else` of `StopAccelerateTimer`, which no recipe may edit).
Possible upstream: animate.c's scene loops pick `TrueColorType` when `alpha_trait == BlendPixelTrait`, while
`XAnimateBackgroundImage`'s later `display_image` call uses `!=` for the same choice.

## Lane b, second round: xwindow.c 1.64 -> 1.83 (wide build, merged)

`XGetWindowImage` (173 cases): aac32e9bf E, 8ca036aab E, f74f6a315 N (`XDirectRowSource`), flat; 9c843791f N
1.70 (nesting 7 -> 6); 79aa878a5 E flat; 601701e77 N (struct passed on, N step 8) 1.73 (nesting 6 -> 5).
`XMakeStandardColormap` (514 cases): 22071e2f2, 3ac1d2e44, be905a450 E, 100ce6216 E (`Verified: guard`, lines
unreached), a929bbc62 N, 2c920507c N, 1a4740387 E (flat plan steps), 8a2273ae3 D (two identical grayscale blocks
to `XApplyGrayVisual`) 1.77, bumps 17 -> 9. 2b3d14d2a E `XBestFont` 1.80 (leaves Deep Nesting; 366 cases);
d032daa75 O `XImportImage` 1.83 (leaves Deep Nesting); flat steps that each leave a category: 804bcb0fd E
`XBestFont`, 18570fea7 CN `XWindowByName` (IDENTICAL), d4f1840fd E `XFreeStandardColormap`, 8f04789b8 E
`XWindowByProperty` (unreached, `Verified: guard`). No new helper finding, no Code Duplication, no new recipe text.
Whole wide catalogue: 14,145 cases, 1 diverged (`infra/0fd402f13a` timeout, 0 on rerun).

Note on the check: `verify_wt.sh` prints VERIFY FAILED on xwindow.c even at the starting tip, from two
`ci_warnings.sh` diagnostics already there (`map_list` maybe uninitialised in `XBestVisualInfo`; a sign-compare on
`index < image->colors`, now inside `XCountRowDiversity`); the lane passed a step only with no new warning and the
guard, call guard, ci_linux_warnings and oracle all passing.

Plateau: `XMakeMagnifyImage` (nesting 8 in the LSB/MSB bitmap loops writing `byte`, `q_bit`, `q` together: only Q
fits, not allowed on unreached lines; the twins would clone); `XMakeImage`, `DestroyXResources`,
`XBestVisualInfo` keep their depth inside `#if defined(MAGICKCORE_HAVE_SHARED_MEMORY)`/`#if !defined(PRE_R4_ICCCM)`
code; `XGetWindowImage` probes to depth 4 are flat; `XDitherImage` (function-static tables and 2D maps).
Possible upstream: `XMakeMagnifyImage`'s bitmap cases advance `q+=scanline_pad` inside the column loop rather than
once per row; `XMakeStandardColormap` never sets `pad` in the copied `XColor`s.

## Lane d, second round: morphology.c 4.48 -> 6.19, fx.c 4.61 no commit (merged)

`MorphologyPrimitiveDirect` (88 cases; no finding left): 7a785f91e D (b) progress tails reuse
`ReportMorphologyProgress` 4.50; 2a915969a E `GetDirectKernelOffset` 4.53; 6291d7ed9 N
`ForwardDirectChannelValue` 4.73; a0c385339 N `ReverseDirectChannelValue` 4.82; 4dd1230d9 playbook (MT
amendment); 2d205ba09 MT nested pair (channel bodies and pixel loops in one commit, as amended step 6) 5.27;
fd1299174 MT loop heads `MorphologyDirectScanRow` 5.35; 0ceb06f54 MT orphaned atomic `MorphologyDirectPass` 5.60.
Timing (medians of 11 alternating runs against the baseline, Distance Euclidean:6 and Voronoi Manhattan:5 on
1500x1500): 6291d7ed9 +1.8% / -3.1%, a0c385339 -0.5% / +0.8%, 2d205ba09 Voronoi -4.3% / -3.4% (the channel merge
alone, not inlined, measured +4.1% to +5.9% and was not taken alone); the two per-row steps stayed within 3% at
load averages of 9-29 (noisy). No indirect call in the object file.

`MorphologyApply` (2,609 cases; once per kernel iteration, not timed): 7f73e686d E 5.63, c7b618dad E 5.66,
44c83dcb2 Y 5.75, b2e81dc07 N 5.84, ca45ccdca Z 5.89, fb2cdb14b FI step 8 (flat, IDENTICAL), e06a06624 I 6.01,
9ebccb019 N 6.10, faad7b3d9 SD 6.19.

MT amendment (4dd1230d9, Decided by `owner` under the 2026-11-10 OK): step 1 admits copies differing by a
statement in one copy only (outside any parallel region), an orphaned `#pragma omp atomic` at the same place in
both (moved whole, unchanged), and a sequential loop's body head; step 4 writes the one-copy statement as
`if (side == NameB) S` and a loop head in PC step 3's form; step 6 lets an inner pair whose out-of-line call
slows the pixel path merge in one commit with the outer pair that holds both calls, quoting both timings.

Plateau: `MorphologyApply` cc 22, 101 lines (the Smooth group, the seven-label two-stage run and `default`: any
helper holding them is cc 9+, 6.13 in a probe); `AcquireKernelBuiltIn` (the open `#if 1`/`#else` question and its
`#if 0` labels); `AcquireFreiChenKernel` (shortening cases 15-19 clones EdgeBasis, 5.91 in a probe);
`ParseKernelArray` (`#if 0`/`#else`). fx.c: everything but `ExecuteRPN` is clean; splitting the evaluator is the
parked chain (owner), and split_source.py refuses any split (FxImage pulls in the parser through
`AcquireFxInfoPrivate`). Lane catalogue: 15,900 cases, 0 diverged. Possible upstream: `ScaleKernelInfo`'s
`minimum = 1` (W16); `(void) (void) FormatLocaleFile` (cosmetic, moved verbatim into `ApplyDirectMorphology`).

## Lane d, third round: re-probing splits under the ruling (merged)

- **resize.c 9.09 -> 10.0** (new `resize-rescale.c` 9.68). 2fc3b7d5a tool (`split_source.py`: a function
  defined in both branches of an `#if`/`#else` calls what either definition calls; a file-level `#if` opened in
  the moved region moves with its `#endif`; two false file-scope-variable refusals fixed; regression over every
  public function of 15 files as a root: 74 of 78 byte-identical, the 4 that differ all resize.c). f2d312574
  playbook (Recipe S, *Preprocessor branches*: a whole file-level `#if defined(...)`/`#else`/`#endif` holding
  whole functions of the family moves with its directives when its `#endif` follows the family's last function,
  nothing in it edited, every side compiled by an available build and passing `split_branch_check.py`).
  b1a297769 S: InterpolativeResizeImage, LiquidRescaleImage, MinifyImage, ResampleImage, ThumbnailImage (30
  functions, 864 lines, the contiguous tail, about a third of the file). The LQR block moves whole:
  `split_branch_check.py` IDENTICAL-MODULO-LAYOUT in `ci-gcc-dlg` (delegate side; 39 functions, 53 `.rodata`
  references, 28 `__LINE__` immediates) and `ci-gcc-min` (stub side; 31 functions, 59, 27). No oracle case runs
  LiquidRescaleImage, so its delegate side rests on that check; combined guards 634 literals and 449 call sites
  unchanged; oracle 59, 4, 13 and 132 cases on the other moved functions, 0 diverged.
- **resize-image.c 9.38 -> 9.68**: d51efe94c RV step 6(a) `TryApplyResizeFilterPass` (ResizeImage leaves Complex
  Method, 911 cases); c4313cb1e B `VerticalFilterPass` (7 -> 4 arguments, flat, leaves a category, 896 cases).
  Ceiling: the public ResizeImage has 5 arguments.
- Plateaus: threshold.c 9.09 (shared row helpers; `BuiltinMap`'s other side under
  `#if MAGICKCORE_ZERO_CONFIGURATION_SUPPORT` is compiled by no build), montage.c 9.11 (no natural family frees
  the ~94 lines the Large Method fix needs), magnify.c 8.84 (one public reaches every static), statistic.c 9.38,
  segment.c 9.02, quantize.c 8.74 (one family each; `MaxTreeDepth` refuses CompressImageColormap).

## Lane a, third round: widget.c 2.26 -> 2.43, accelerate.c 2.90 -> 3.04 (wide build, merged)

widget.c, 20 commits on `XFontBrowserWidget` (leaves Deep Nesting, Nested Complexity and Brain Method): a24167ba3
CN (IDENTICAL), 321cf9384 CT; D reuses of the ListBrowser helpers (c3ef5289c, b1494bfdd, d744352ca typedef move,
b35837ded, c8a141a80, 0e71fef43, 4d4e97556; reuse_check plus obj_modulo MODULO-IDENTICAL or IDENTICAL); 389eca142 E
2.30; 7bd957c11 Z (depth 5 -> 4); 1c0639f25, f848f1ed5, 437a47347 E; 6e6703f57 SD, 30ceb2bd0 E; f2627c4c9 N;
11e91aef0 N 2.38; c4608c1cc Z; ac301b0b6 LB step 8 2.43 (IDENTICAL). Not a plateau: XColorBrowserWidget and
XFileBrowserWidget have the same shape (about +0.16 each with the new helpers). accelerate.c: 1e3cfaccd E
`ReleaseGrayscaleResources` 2.97 (Contrast and Grayscale leave Code Duplication); 06fd2f469 N
`SetResizeVerticalFilterArgs` 3.04 (the resize twins leave Code Duplication). Plateau: the 10 wrappers clone
through the assert/trace preamble (moving it moves `GetMagickModule()`); Equalize's helper would be cc 11; bumps
holding `OpenCLThrowMagickException(GetMagickModule())`. Recipe text (Decided by `owner`): 66f51afee D (forward
declaration and typedef move; input-struct bindings), 2e626175f SD step 1 (a `switch` with no `default:`),
20a94bbe7 N step 2 (an array passed only to calls). Tools: fa1130f3d `case_tail_check.py`, 0ccab1646 and
0a24f1314 `reuse_check.py` (planted errors caught). Note: `ci_warnings.sh` reports a pre-existing `3999999999`
constant warning in accelerate.c's LocalContrast at every step. Whole wide catalogue: 14,145 cases, 0 diverged.
Possible upstream: widget.c Font's reply press reads a stale `i` when `XLoadQueryFont` returns NULL (kept); Font's
SelectionRequest leaves `notify.send_event` unset and does not check `reply_info.highlight`, unlike ListBrowser's.

## Lane c, third round: compare.c split, opencl.c 5.27 -> 5.47 (merged)

compare.c's one conditional, `#if defined(MAGICKCORE_HDRI_SUPPORT) && defined(MAGICKCORE_FFTW_DELEGATE)`, has no
`#else`; the oracle's `cand` and `cand-wide` (HDRI and FFTW) compile it, `ci-gcc-min` and `ci-gcc-dlg` compile the
file without it. acf515ccb tool: `split_branch_check.py` resolves clang's string-literal labels (`.L.str.N`) to
their bytes and counts the compiled source path as `__FILE__` (a planted literal change still prints DIFFERENT;
the earlier checks still pass). 7b97e311b S: GetImageDistortion, GetImageDistortions, SetImageColorMetric and
SimilarityImage move to `compare-similarity.c` with 77 statics and 18 typedefs (81 functions, 5,195 lines; no
smaller family is accepted, the statics are shared). **compare.c 2.97 -> 8.92, compare-similarity.c 2.97.**
`split_branch_check.py`: `cand` IDENTICAL-MODULO-LAYOUT (26 functions; 22 `__LINE__`, 247 constant-pool labels,
103 string-literal labels), `ci-gcc-min` and `ci-gcc-dlg` IDENTICAL-MODULO-LAYOUT (77 functions; 45 `.rodata`,
22 `__LINE__`, 12 rip-relative). Combined guard OK (702 literals, 1,195 calls); oracle SimilarityImage 10,
GetImageDistortion 165, GetImageDistortions 62, SetImageColorMetric 62, CompareImages 165 cases, 0 diverged.
**For the owner:** this split relabels most of the file (the effect.c/visual-effects.c question, at its largest).

opencl.c (wide build): cf1adf8f6 OC step 6 `LoadOpenCLPlatformDevices` 5.33 (cc 17 -> 11); 470124b95 E
`AcquireOpenCLCacheDirectory` 5.38 (leaves Large Method; logs stay in the caller for `GetMagickModule()`);
ded3ea345 P `IsDoctypeQuote` 5.40; fed781bc9 P `IsDoctypeEnd` 5.41; 6609ed18e P `IsOpenCLDeviceSkipped` 5.43;
8ff5648a9 playbook (Recipe W step 8: a `void` caller's `return;` exits, Decided by `owner`); b7dd7edfc W step 8
`AcquireOpenCLContexts` 5.47 (LoadOpenCLDevices leaves Large Method). verify_wt.sh prints VERIFY FAILED on every
opencl.c step from the pre-existing `StopAccelerateTimer` warning (line 257, unchanged). Plateau: the DOCTYPE scan
(lane a's LB break-form question), the uncompiled `#if WINDOWS` block in GetOpenCLCacheDirectory, five-argument
helpers, the CF/function-pointer question. Whole catalogues on the lane: 15,900 and 14,145 cases, 1 diverged each
(`infra/0fd402f13a` load timeout).

## Lane b, third round: composite.c 3.66 -> 3.69 (merged)

91e783d56 N step 1 `CopyTextureImage` (TextureImage leaves Complex Method, 9 cases); 6dce66669 E `GetCompositeClamp`
and b877bf869 PC `CompositeOverRowHead` (CompositeOverImage leaves Complex and Large Method, 567 cases);
SetVirtualCompositeChannels (21 cases): 9082cb1f2 B `VirtualCompositeInputs` (7 -> 3 arguments), 45ed667ab SD,
5254a4b74 SD step 6 (cc 24 -> 17), 49f60de32 E step 7 `GetVirtualCanvasPixel` 3.69 (Bumpy Road now only in
CompositeImage). All flat steps but the last; no helper finding, no Code Duplication. Timing: wall clock too noisy
(load about 11), so callgrind instruction counts against d38e0c1b3: fewer everywhere (Over -1.1%; In -0.16%, Clear
-0.14%, Dissolve -0.12%, Multiply -0.26% on the virtual-pixel paths).

Plateau 3.69 under the current rulings (padded-stub probes): the gain comes only when many functions are clean
together (CompositeImage with Saliency, Seamless, Divergent, SetVirtual, Over, Texture, Pixel, Channels: 4.16).
**For the owner:** (1) a Copy/Intensity merge (CF cannot: different row-callee types and fills), about +0.1 to
+0.15; (2) letting I or ST carry nine targets, or a block tail, so CompositeImage's setup switch (writes nine
caller variables) can leave, about +0.25; (3) hoisting `FreeDivergentResources` with Recipe H for what it measures
alone (CodeScene counts the macro's eight `if`s in BlendDivergentImage). split_source.py refuses both public roots
(shared `SyncCompositeRow`). Ready but zero gain now: an LP amendment for two `break;` groups plus TB on a
nine-label `IsVirtualCanvasOperator`. Possible upstream: `BlendDivergentImage` leaks `dx_divergent` when the
`dy_divergent` convolve fails; `SaliencyBlendImage` leaks `divergent_image` on the residual-clone and kernel
failure paths; both blend loops `break` when cloning the residual fails and then `DestroyImage(NULL)` asserts;
CompositeOverImage sets `status`/`progress` twice and fills two unread `PixelInfo`s.

## Lane d, fourth round: the split files (merged)

- **fourier-complex.c 9.50 -> 10.0**: c5b5995c8 H (`ComplexImageTag` moved above ComplexImages, pp_identical
  IDENTICAL; flat, making room for the next step); 4380838ec N step 1 with E step 7 (everything from the cache-view
  acquisitions to `return(FinishComplexImages(...))` into `ApplyComplexImageRows`, the whole parallel loop and its
  pragma unchanged; 12 inputs filled in member order before the call; ComplexImages leaves Large Method; 18 cases,
  once per image, no timing needed). A first probe that left the return behind left `complex_images` named only by
  the pragma (-Wunused-variable without OpenMP), so the return moved with the tail.
- visual-effects-pixel.c 9.09, three flat B steps each leaving Excess Number of Function Arguments: 5d9a9dccd
  `AddNoisePixelChannels` 7 -> 4 (42 cases; +0.6% median of 11 alternating runs, an earlier 9 gave +2.0%),
  ed402c3f1 `MorphPixelChannels` 6 -> 3 (3 cases; +0.2%), eafe821b1 `AnnotatePolaroidCaption` 6 -> 4 (5 cases).
  B on `TintPixel` rejected (clones `SepiaToneImageRow`, 8.28).
- Plateaus: resize-rescale.c 9.68 (public argument counts only), colorspace-transform.c 9.38 (Lines of Code only;
  one family), effect-filter.c 9.09 (Lines of Code and public argument counts; shared statics refuse every split),
  attribute-depth.c and attribute-depth-measure.c 8.16 (every finding inside the `#if !defined(HDRI)` branch),
  feature.c 9.09 (three natural algorithm families share `SetFeatureProgress`; **for the owner:** a split needs the
  helper copied into each file or a ruling), enhance.c 9.09.

## Lane a, fourth round: widget.c 2.43 -> 2.80 (wide build, merged)

XColorBrowserWidget leaves Deep Nesting and Brain Method (depth 6 -> 3, bumps 25 -> 8, cc 165 -> 115): a82e8a01d
CN 2.49, 507efd07c D 2.54, 059c35d5b Z 2.64, 6424787b2 LB 2.70; flat plan steps 6836dd083 (typedef and declaration
move, IDENTICAL), D reuses f9b5ba877, 2b65ca45b, 3e6e7a6c3, c3db7a481, 1a2cebac4, ba6e198ad, ba9692d47, 62ab50637,
247a84e70, 9efaa0ef9, e4889fb94, fe6f50339, d7178b9b7 (SD into `MoveFontBrowserSliderByPage`), E c97580604,
a1b65539b, dce428d20 (reuse_check OK; obj_modulo MODULO-IDENTICAL or IDENTICAL; 14 cases). XFileBrowserWidget:
535f8f62e CN 2.76 (IDENTICAL, 21 cases). XConfirmWidget: 86f70f06c CN (IDENTICAL), f72e5afb4 D (a)
`ReleaseConfirmChoice` 2.80 (leaves Deep Nesting). Whole wide catalogue: 14,145 cases, 0 diverged.

Evidence notes: f9b5ba877 and fe6f50339 print DIFFERENT under plain obj_modulo because XFontBrowserWidget stops
inlining the now-shared helper; 9f2858989 adds `NOINLINE=` to obj_modulo.sh (keeps a named helper out of line on
both sides): fe6f50339 MODULO-IDENTICAL, f9b5ba877 one `lea` apart (the static `glob_pattern`'s address now passed
as an argument); both rest on the oracle with XFontBrowserWidget's 12 cases rerun. 059c35d5b and 6424787b2 were
checked by a rewrite diff (the moved text is rewritten as `(*v)`). Recipe text (Decided by `owner`): a630a7db2 D
(*case tail*, *returned local*, *split selection*), tools c61d11ec1 and 26cb14426 (planted errors caught).
**Message slips (not amended):** 6836dd083 and a82e8a01d quote 945 LoC from the morning review (941 measured after
CN), against the rule that every number is measured for its commit; 059c35d5b says the jump test remains at depth
4 (only the list update did). Not a plateau: XFileBrowserWidget needs about ten new helpers; XPreferencesWidget's
probe scored 2.79 (not taken). Possible upstream: XColorBrowserWidget's ButtonRelease ends with a redundant
`if (MatteIsActive(...) == MagickFalse) break; break;`; Font's SelectionRequest does not set `notify.send_event`.
