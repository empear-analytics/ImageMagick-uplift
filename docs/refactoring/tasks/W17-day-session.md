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
