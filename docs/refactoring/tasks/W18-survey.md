# W18 lane d: survey of campaign A files below 10.0

Read-only survey (no commit, no tracked file edited). Worktree `~/im-w14d`, branch `campaign/W18-d`, fast-forwarded
from d71ca3694 to the merged tip **7ef8ca0b8** (`git merge --ff-only`; no commit made). Scores from `tools/ch.py --json`
(cs-mcp 1.5.16) on that tip, all 84 campaign A files in one batch; 36 are at 10.0, **48 are below**. Raw reviews:
`review_all.json`; per-finding rows: `rows.json`; probes: `probe/`, scripts `stub.py`, `classify.py`, `gen.py`.

Classes used (one row per function and category, or one per file-level category):

- (a) public (`MagickExport`) or `MagickPrivate` function with too many arguments: signature fixed.
- (b) file-level only: Lines of Code, Number of Functions, Overall Code Complexity, Lines of Declarations.
- (c) pinned: the finding sits in an `#if defined`/`#else` branch that the oracle build does not compile (or in the
  `HDRI && FFTW_DELEGATE` region of compare-similarity.c, `#if 0`/`#else` in `ParseKernelArray`).
- (d) an owner question named in the W17/W18 reports (OpenMP shared-table extraction, `#if M` with M defined 0, label
  runs / function-pointer tables / CF, label deletion, `#if 1`/`#else`, wrapper preamble and `GetMagickModule`, nine-target
  I/ST, Recipe H on a function-local macro, the parked fx.c split chain, eleven-function clone cliff in compare-similarity).
- (e) OPEN: non-public function (or a body of a public one), not pinned, not an owner question. Sub-classes:
  **e-flat** a static signature with too many arguments in a file where a public function keeps the category (probed:
  enhance-clahe.c, three statics to 4 arguments: 9.68 -> 9.68; W18 lane d found the same on threshold.c and resize-image.c);
  **e0** probed negative by an earlier lane (clone cliff or slower) and not re-probed; **e1** limited by reach (unreached
  function: only E/G/P); **e2** recipe gap (no recipe admits the block); plain **e** has a fitting recipe.
- IP: lanes a (xwindow.c, opencl.c), b (display.c), c (widget.c) are working on them: findings listed, not classified.

Method for the estimates: `stub.py` copies a file into `probe/`, replaces named function bodies by a body of distinct,
branch-free arithmetic lines (so stubs do not clone each other) and scores the copy; `--pad` keeps the file's line count
(needed only for magnify.c and montage.c, which sit just under 1,000 lines). A stub is a **ceiling**: it costs no helper.
Single-function stubs are flat in the big files (accelerate.c, composite.c, compare-similarity.c all 0.00 per function),
which matches the reports: those files move only when many functions are clean together.

Caution found while probing (not in the reports): in compare.c, appending two trivial cc-1 functions to the unchanged
file makes it newly gain *Overall Code Complexity* (8.92 -> 8.39; one function does not; three give the same 8.39). A real
helper chain for `CompareImages` (4 helpers, modelled below) therefore trips the finding the PR gate refuses.

## Top of the result

Counts (non-IP findings, 338): (a) 60, (b) 37, (c) 40, (d) 91, (e) 53, e-flat 40, e0 9, e1 2, e2 6. In-progress files: 258 findings.

### OPEN candidates, ranked by estimated gain per effort

| # | file / function | finding | fitting recipe(s) | estimate (stub ceiling; model) | effort |
|---|---|---|---|---|---|
| 1 | compare.c `CompareImages` (+ `IsImagesEqual`) | Complex (cc 19), Large (130), Bumpy Road (IsImagesEqual) | `row` fill: FB (7 members, then call); three colour queries: O step 7 / Z step 7 (3 `PixelInfo` out-pointers + 2 inputs = const struct of addresses); the two clone/extent blocks (return constant NULL): W + I/Z; highlight loop: N (input struct) or PC head; `IsImagesEqual` inner x loop with `if (x < columns) break;`: **LT** | 8.92 -> 9.68 ceiling (CompareImages alone 9.61, IsImagesEqual alone 9.00). 4-helper model + LT: 9.09 with new *Overall Code Complexity* (trips: see caution) | 6-8 steps |
| 2 | magnify.c `Scale3X` | Complex (cc 12): eight `if (C) CopyPixels(..a..) else CopyPixels(..4..)` | NS (as on `Eagle3X`), FB frees lines | 8.84 -> 9.09 alone; both Scale3X and Xbr2X (padded) 9.38 | 4-6 steps, but file is at 998 of 1,000 lines: every helper must be paid for by lines freed (FB/FI IDENTICAL steps elsewhere in the file) |
| 3 | animate.c `XMagickCommand` (wide) | Complex Method cc 9 (one over) | CT step 2 (block `static filenames` by pointer) or E/N step 8 on the `OpenCommand` tail (the shared `tile` struct exists) | 4.40 -> 4.49 | 1-2 steps; OpenCommand branch unreached (guard level) |
| 4 | accelerate.c `AccelerateGrayscaleImage` (wide) | Complex Conditional x2 + Complex cc 9 | **P** twice (`(checkAccelerateCondition || checkPixelIntensity)` and the three-trait test; existing `HasUndefinedRGBATraits` tests four traits, so not a D reuse) | 3.19 -> 3.27 | 2 steps |
| 5 | magnify.c `Xbr2X` | Complex cc 9 | NS (W17: 9.07 but crosses 1,000 lines) | 8.84 -> 9.07 alone | 2-3 steps, line budget |
| 6 | segment.c `GetImageDynamicThreshold` | Bumpy Road (2) | unreached: only E/G/P; W17: neither bump fits. Adding two oracle cases (`-segment` dynamic threshold) would admit more recipes | 9.02 -> 9.09 | 2 steps + cases |
| 7 | montage.c `MontageImageList` | Complex cc 10, Large 164 lines | FB/FI/I already used; needs about 94 lines freed first (file ~1 line under 1,000) | 9.11 -> 9.38 | 8+ steps |
| 8 | visual-effects-plasma.c `PlasmaImageProxy` (e0) | Bumpy 4, Complex 21, Large 103 | MT on Bottom/Top, RV on Left/Right (returns differ); W18 probe 8.15 (helpers clone) | 8.50 -> 9.68 | 8+ steps, clone cliff |
| 9 | colorspace-convert.c `ConvertRGBToGeneric` (e0) | Complex cc 20, Large 113 | SD chain (probes 7.78/8.28); TB/KT need constants | 9.08 -> 9.68 | clone cliff |
| 10 | animate.c `XAnimateImages` / `XAnimateBackgroundImage` (e2) | cc 74 and 769 lines; 9 bumps | no recipe admits a function-static whose address also goes to calls; needs a new recipe | 4.40 -> 5.30 (Background) or 6.73 (Images) | 20+ steps |

Also OPEN but with zero single-step gain: accelerate.c `Compute*` (stub: each alone 3.19, nine of 17 4.06, all 17 6.76:
a 17-function chain), composite.c statics (`SaliencyBlendImage`, `SeamlessBlendImage`, `SetVirtualCompositeChannels`,
`CompositeImageChannels`, `CompositeImagePixel`: each alone 3.69), compare-similarity.c `GetSimilarityForMetric` (SD chain,
flat), morphology.c `MorphologyApply` (stub +0.37) and `AcquireFreiChenKernel` (+0.27) (both e0), distort.c `poly_basis_dx`
(slower), and the e-flat static signatures listed under each file.

### Owner questions with the largest payoff (class c and d; stub ceilings, nothing taken)

| item | file / function | ceiling |
|---|---|---|
| edit a block that lies wholly inside a no-`#else` `#if !defined(MAGICKCORE_HDRI_SUPPORT)` branch, verified against `ci-gcc-min` object code like `split_branch_check.py` | attribute-depth.c `SetImageDepth`; attribute-depth-measure.c `GetImageDepth` | 8.16 -> 10.0 each (`depth_map` block: E/N inside the branch, helper in its own `#if !HDRI` guard) |
| same for the FFTW `#if !defined(ENABLE_FFTW_DELEGATE)`/`#else` and `HAVE_COMPLEX_H` pairs | fourier.c three functions | 7.59 -> 9.38 (stub of all three) |
| `#if M` with M defined 0 (`DEBUG_OPT_FRAME`, `DEBUG_ELLIPSE`, `EWA`) | layer.c `OptimizeLayerFrames` 8.25 -> 9.38; resample.c `ResamplePixelColor` + `ScaleResampleFilter` 8.53 -> 9.68 | +1.13, +1.15 |
| OpenMP shared-table extraction (grays-fill loop) | feature-texture.c `GetImageFeatures` | 7.63 -> 9.38 |
| parked evaluator split | fx.c `ExecuteRPN` | 4.61 -> 8.28 |
| Recipe H on the function-local `PushSegmentStack` macro (as the step that unblocks the loop extraction) | paint-floodfill.c `FloodfillPaintImage` | 8.35 -> 9.09 |
| `#if 1`/`#else` + `#if 0` labels | morphology.c `AcquireKernelBuiltIn` | 6.19 -> 7.25 |
| label deletion / `#if 0` | distort.c `SetDistortNoBestFit` + `poly_basis_dx` | 7.70 -> 8.28 |
| label-run helpers / nine-target I/ST / Recipe H on `FreeDivergentResources` | composite.c `CompositeChannelColor` alone 3.69 -> 4.08 | big chains |
| wrapper preamble `GetMagickModule()`; eleven-function clone cliff (RL needs pragma edits) | accelerate.c wrappers; compare-similarity.c `Get*Similarity` | no single-step gain |

### Files with no OPEN finding

True plateaus (only (a) and (b) findings, 16 files): quantize.c, effect-blur.c, effect-filter.c, enhance.c, feature.c,
colorspace-transform.c, statistic.c, transform-region.c, vision-components.c, colorspace-generic-rgb.c,
enhance-level-colors.c, paint.c, resize-image.c, resize-rescale.c, statistic-function.c, visual-effects-stegano.c.
Pinned or owner-held only (no (e), 7 files): fx.c, fourier.c, feature-texture.c, attribute-depth.c,
attribute-depth-measure.c, layer.c, paint-floodfill.c. Only zero-gain statics beyond (a)-(d) (9 files): resample.c
(`ClampUpAxes`), enhance-tone.c (`EqualizeImageRow`), threshold.c (`RangeThresholdValue`, `RangeThresholdChannels`),
visual-effects-pixel.c (`TintPixel`), enhance-clahe.c, histogram.c (`UniqueColorsToImage`), layer-composite.c
(`CompositeCanvas`), statistic-image.c, threshold-adaptive.c (`SumThresholdWindow`).

Files with an OPEN finding (12): compare-similarity.c, accelerate.c, composite.c, animate.c, morphology.c, distort.c,
visual-effects-plasma.c, magnify.c, compare.c, segment.c, colorspace-convert.c, montage.c.
In progress (4): display.c, xwindow.c, widget.c, opencl.c.

# Appendix A: counts and findings

### Counts of findings (a finding = one function in one category, or one file-level category)

| class | findings |
|---|---:|
| (a) public/private signature | 60 |
| (b) file-level | 37 |
| (c) pinned `#if defined`/`#else` | 40 |
| (d) owner question | 91 |
| (e) OPEN | 53 |
| (e-flat) OPEN, zero gain | 40 |
| (e0) OPEN, probed negative | 9 |
| (e1) OPEN, unreached/limited | 2 |
| (e2) OPEN, recipe gap | 6 |
| in progress (lane) | 258 |
| total | 596 |

### Per file (score on merged tip 7ef8ca0b8)

| file | score | a | b | c | d | e | e-flat | e0 | e1 | e2 | IP |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| display.c | 1.97 |  |  |  |  |  |  |  |  |  | 102 |
| xwindow.c | 2.27 |  |  |  |  |  |  |  |  |  | 96 |
| compare-similarity.c | 2.97 | 2 | 2 | 25 | 52 | 2 | 8 |  |  |  |  |
| widget.c | 3.02 |  |  |  |  |  |  |  |  |  | 41 |
| accelerate.c | 3.19 | 5 | 2 |  | 10 | 36 | 13 |  |  |  |  |
| composite.c | 3.69 | 1 | 2 |  | 10 | 7 | 3 |  |  |  |  |
| animate.c | 4.40 | 1 | 2 |  |  | 1 | 2 |  | 1 | 6 |  |
| fx.c | 4.61 | 1 | 2 |  | 2 |  |  |  |  |  |  |
| opencl.c | 5.80 |  |  |  |  |  |  |  |  |  | 19 |
| morphology.c | 6.19 | 2 | 2 | 1 | 3 |  |  | 3 |  |  |  |
| fourier.c | 7.59 |  | 2 | 6 |  |  |  |  |  |  |  |
| feature-texture.c | 7.63 |  | 2 |  | 3 |  |  |  |  |  |  |
| distort.c | 7.70 | 2 | 2 |  | 1 |  |  | 1 |  |  |  |
| attribute-depth-measure.c | 8.16 |  | 1 | 4 |  |  |  |  |  |  |  |
| attribute-depth.c | 8.16 |  | 1 | 4 |  |  |  |  |  |  |  |
| layer.c | 8.25 |  | 1 |  | 3 |  |  |  |  |  |  |
| paint-floodfill.c | 8.35 | 1 | 1 |  | 3 |  |  |  |  |  |  |
| visual-effects-plasma.c | 8.50 | 1 | 1 |  |  |  | 1 | 3 |  |  |  |
| resample.c | 8.53 | 2 |  |  | 4 |  | 1 |  |  |  |  |
| quantize.c | 8.81 | 1 | 1 |  |  |  |  |  |  |  |  |
| magnify.c | 8.84 |  | 1 |  |  | 2 |  |  |  |  |  |
| compare.c | 8.92 | 1 |  |  |  | 3 |  |  |  |  |  |
| segment.c | 9.02 | 2 | 1 |  |  |  |  |  | 1 |  |  |
| colorspace-convert.c | 9.08 | 3 |  |  |  |  |  | 2 |  |  |  |
| effect-blur.c | 9.09 | 3 | 1 |  |  |  |  |  |  |  |  |
| effect-filter.c | 9.09 | 2 | 1 |  |  |  |  |  |  |  |  |
| enhance-tone.c | 9.09 | 2 | 1 |  |  |  | 1 |  |  |  |  |
| enhance.c | 9.09 | 1 | 1 |  |  |  |  |  |  |  |  |
| feature.c | 9.09 | 3 | 1 |  |  |  |  |  |  |  |  |
| threshold.c | 9.09 | 1 | 1 |  |  |  | 2 |  |  |  |  |
| visual-effects-pixel.c | 9.09 | 5 | 1 |  |  |  | 1 |  |  |  |  |
| montage.c | 9.11 |  |  |  |  | 2 |  |  |  |  |  |
| colorspace-transform.c | 9.38 |  | 1 |  |  |  |  |  |  |  |  |
| statistic.c | 9.38 |  | 1 |  |  |  |  |  |  |  |  |
| transform-region.c | 9.38 |  | 1 |  |  |  |  |  |  |  |  |
| vision-components.c | 9.38 |  | 1 |  |  |  |  |  |  |  |  |
| colorspace-generic-rgb.c | 9.68 | 1 |  |  |  |  |  |  |  |  |  |
| enhance-clahe.c | 9.68 | 1 |  |  |  |  | 3 |  |  |  |  |
| enhance-level-colors.c | 9.68 | 1 |  |  |  |  |  |  |  |  |  |
| histogram.c | 9.68 | 1 |  |  |  |  | 1 |  |  |  |  |
| layer-composite.c | 9.68 | 1 |  |  |  |  | 1 |  |  |  |  |
| paint.c | 9.68 | 4 |  |  |  |  |  |  |  |  |  |
| resize-image.c | 9.68 | 1 |  |  |  |  |  |  |  |  |  |
| resize-rescale.c | 9.68 | 4 |  |  |  |  |  |  |  |  |  |
| statistic-function.c | 9.68 | 1 |  |  |  |  |  |  |  |  |  |
| statistic-image.c | 9.68 | 1 |  |  |  |  | 2 |  |  |  |  |
| threshold-adaptive.c | 9.68 | 1 |  |  |  |  | 1 |  |  |  |  |
| visual-effects-stegano.c | 9.68 | 1 |  |  |  |  |  |  |  |  |  |

## Findings by file

### display.c (1.97)

File-level: Overall Code Complexity [IP], Lines of Declarations in a Single File [IP]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| XDisplayImage | PUBLIC | Brain Method (); Bumpy Road (41); Deep Nesting (depth 6); Large Method (1830); Excess Args (args 7) | IP |  |
| XDrawEditImage | static | Brain Method (); Bumpy Road (12); Deep Nesting (depth 6); Large Method (795); Excess Args (args 5) | IP |  |
| XCropImage | static | Bumpy Road (7); Complex Method (cc = 70); Large Method (417); Excess Args (args 6) | IP |  |
| XAnnotateEditImage | static | Bumpy Road (6); Complex Method (cc = 67); Large Method (470); Excess Args (args 5) | IP |  |
| XChopImage | static | Bumpy Road (6); Complex Method (cc = 45); Large Method (303); Excess Args (args 5) | IP |  |
| XCompositeImage | static | Bumpy Road (6); Complex Method (cc = 49); Large Method (333); Excess Args (args 5) | IP |  |
| XOpenImage | static | Bumpy Road (6); Complex Method (cc = 16); Large Method (120) | IP |  |
| XScreenEvent | static | Bumpy Road (6); Complex Method (cc = 40); Large Method (167) | IP |  |
| XROIImage | static | Bumpy Road (5); Complex Method (cc = 56); Large Method (531); Excess Args (args 5) | IP |  |
| XColorEditImage | static | Bumpy Road (4); Complex Method (cc = 43); Large Method (304); Excess Args (args 5) | IP |  |
| XImageWindowCommand | static | Bumpy Road (4); Complex Method (cc = 132); Large Method (342); Excess Args (args 7) | IP |  |
| XMagickCommand | static | Bumpy Road (4); Complex Method (cc = 264); Large Method (1734); Excess Args (args 6) | IP |  |
| XMatteEditImage | static | Bumpy Road (4); Complex Method (cc = 45); Large Method (316); Excess Args (args 5) | IP |  |
| XRotateImage | static | Bumpy Road (4); Complex Method (cc = 44); Large Method (285); Excess Args (args 6) | IP |  |
| XSaveImage | static | Bumpy Road (4); Complex Method (cc = 18); Large Method (113); Excess Args (args 5) | IP |  |
| XTrimImage | static | Bumpy Road (4); Complex Method (cc = 20); Large Method (84); Excess Args (args 5) | IP |  |
| XDisplayBackgroundImage | PUBLIC | Bumpy Road (4); Complex Method (cc = 28); Large Method (186) | IP |  |
| XPasteImage | static | Bumpy Road (3); Complex Method (cc = 38); Large Method (257); Excess Args (args 5) | IP |  |
| XTileImage | static | Bumpy Road (3); Complex Method (cc = 25); Large Method (192); Excess Args (args 6) | IP |  |
| XVisualDirectoryImage | static | Bumpy Road (3); Complex Method (cc = 18); Large Method (131) | IP |  |
| XCropExpose | static | Bumpy Road (2) | IP |  |
| XImageCache | static | Bumpy Road (2); Complex Method (cc = 80); Large Method (190); Excess Args (args 6) | IP |  |
| XPanImage | static | Bumpy Road (2); Complex Method (cc = 20); Large Method (106) | IP |  |
| XMagnifyWindowCommand | static | Complex Method (cc = 38); Large Method (84); Excess Args (args 5) | IP |  |
| XTranslateImage | static | Complex Method (cc = 19); Large Method (73) | IP |  |
| XMagnifyImage | static | Complex Method (cc = 17); Large Method (70) | IP |  |
| XConfigureImage | static | Complex Method (cc = 16); Large Method (112); Excess Args (args 5) | IP |  |
| DisplayImages | PUBLIC | Complex Method (cc = 9) | IP |  |
| XBackgroundImage | static | Excess Args (args 5) | IP |  |
| XMakePanImage | static | Excess Args (args 5) | IP |  |
| XPrintImage | static | Excess Args (args 5) | IP |  |

### xwindow.c (2.27)

File-level: Overall Code Complexity [IP]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| XMakeImage | PRIVATE | Brain Method (); Bumpy Road (19); Deep Nesting (depth 4); Large Method (336); Excess Args (args 7) | IP |  |
| XBestVisualInfo | PRIVATE | Bumpy Road (8); Deep Nesting (depth 4); Complex Method (cc = 41); Large Method (232) | IP |  |
| XGetWindowImage | static | Bumpy Road (7); Deep Nesting (depth 5); Complex Method (cc = 62); Large Method (329) | IP |  |
| XImportImage | PUBLIC | Bumpy Road (7); Complex Method (cc = 31); Large Method (170) | IP |  |
| DestroyXResources | PUBLIC | Bumpy Road (6); Deep Nesting (depth 5); Complex Method (cc = 39); Large Method (183) | IP |  |
| XMakeImageMSBFirst | static | Bumpy Road (6); Complex Method (cc = 30); Large Method (191) | IP |  |
| XMakeStandardColormap | PRIVATE | Bumpy Road (6); Complex Method (cc = 43); Large Method (248); Excess Args (args 7) | IP |  |
| XDitherImage | static | Bumpy Road (5); Complex Method (cc = 13); Large Method (112) | IP |  |
| XDrawImage | PRIVATE | Bumpy Road (5); Complex Method (cc = 43); Large Method (273); Excess Args (args 5) | IP |  |
| XGetPixelInfo | PRIVATE | Bumpy Road (5); Complex Method (cc = 22); Large Method (170); Excess Args (args 6) | IP |  |
| XMakeImageLSBFirst | static | Bumpy Road (5); Complex Method (cc = 28); Large Method (183) | IP |  |
| XMakeMagnifyImage | PRIVATE | Bumpy Road (5); Complex Method (cc = 47); Large Method (288) | IP |  |
| XMakeWindow | PRIVATE | Bumpy Road (5); Complex Method (cc = 27); Large Method (184); Excess Args (args 7) | IP |  |
| XBestIconSize | PRIVATE | Bumpy Road (4); Complex Method (cc = 13); Large Method (73) | IP |  |
| XAnnotateImage | PRIVATE | Bumpy Road (3); Complex Method (cc = 24); Large Method (156); Excess Args (args 5) | IP |  |
| XFreeResources | PRIVATE | Bumpy Road (3); Complex Method (cc = 16); Excess Args (args 7) | IP |  |
| XGetMapInfo | PRIVATE | Bumpy Road (3); Complex Method (cc = 11) | IP |  |
| XRefreshWindow | PRIVATE | Bumpy Road (3); Complex Method (cc = 15); Large Method (71) | IP |  |
| XRenderImage | PRIVATE | Bumpy Road (3); Complex Method (cc = 17); Large Method (156); Excess Args (args 5); Excess Args (args 5) | IP |  |
| XBestPixel | PRIVATE | Bumpy Road (2); Complex Method (cc = 16); Excess Args (args 5) | IP |  |
| XDisplayImageInfo | PRIVATE | Bumpy Road (2); Complex Method (cc = 19); Large Method (109); Excess Args (args 6) | IP |  |
| XGetWindowInfo | PRIVATE | Bumpy Road (2); Complex Method (cc = 13); Large Method (108); Excess Args (args 7) | IP |  |
| XSelectWindow | static | Bumpy Road (2); Complex Method (cc = 20); Large Method (126) | IP |  |
| XGetResourceClass | PUBLIC | Complex Method (cc = 16) | IP |  |
| XInitializeWindows | PRIVATE | Complex Method (cc = 14); Large Method (142) | IP |  |
| FontToList | static | Complex Method (cc = 13) | IP |  |
| XGetResourceDatabase | PUBLIC | Complex Method (cc = 12) | IP |  |
| XUserPreferences | PRIVATE | Complex Method (cc = 9) | IP |  |
| XRemoteCommand | PUBLIC | Complex Method (cc = 9) | IP |  |
| XMakePixmap | static | Complex Method (cc = 9) | IP |  |
| XError | PUBLIC | Complex Method (cc = 9) | IP |  |
| XGetResourceInfo | PUBLIC | Large Method (120) | IP |  |
| XClientMessage | PRIVATE | Excess Args (args 5) | IP |  |
| XConfigureImageColormap | PRIVATE | Excess Args (args 5) | IP |  |
| XMakeCursor | PRIVATE | Excess Args (args 5) | IP |  |

### compare-similarity.c (2.97)

File-level: Lines of Code in a Single File [b], Overall Code Complexity [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| GetAESimilarity | static | Bumpy Road (4); Complex Method (cc = 20); Code Duplication (); Large Method (100) | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| GetDPCSimilarity | static | Bumpy Road (4); Complex Method (cc = 17); Code Duplication (); Large Method (125) | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| GetFUZZSimilarity | static | Bumpy Road (4); Complex Method (cc = 19); Code Duplication (); Large Method (98) | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| GetMEPPSimilarity | static | Bumpy Road (4); Complex Method (cc = 21); Code Duplication (); Large Method (112) | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| GetNCCSimilarity | static | Bumpy Road (4); Complex Method (cc = 17); Code Duplication (); Large Method (127) | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| GetPHASESimilarity | static | Bumpy Road (4); Complex Method (cc = 28); Large Method (180) | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| SimilarityImage | PUBLIC | Bumpy Road (4); Complex Method (cc = 37); Large Method (171); Excess Args (args 7) | a,c | inside `#if defined(HDRI) && defined(FFTW_DELEGATE)` region (lines 2972-4794 and 5152-5201), compiled only with both |
| GetMAESimilarity | static | Bumpy Road (3); Complex Method (cc = 15); Code Duplication (); Large Method (84) | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| GetMSESimilarity | static | Bumpy Road (3); Complex Method (cc = 15); Code Duplication (); Large Method (83) | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| GetPASimilarity | static | Bumpy Road (3); Complex Method (cc = 17); Code Duplication (); Large Method (80) | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| GetPDCSimilarity | static | Bumpy Road (3); Complex Method (cc = 15); Code Duplication (); Large Method (81) | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| GetSSIMSimularity | static | Bumpy Road (3); Complex Method (cc = 20); Large Method (157) | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| DPCSimilarityImage | static | Complex Method (cc = 36); Large Method (165); Excess Args (args 5) | c,e-flat | inside `#if defined(HDRI) && defined(FFTW_DELEGATE)` region (lines 2972-4794 and 5152-5201), compiled only with both |
| NCCSimilarityImage | static | Complex Method (cc = 28); Large Method (127); Excess Args (args 5) | c,e-flat | inside `#if defined(HDRI) && defined(FFTW_DELEGATE)` region (lines 2972-4794 and 5152-5201), compiled only with both |
| PhaseSimilarityImage | static | Complex Method (cc = 25); Large Method (123); Excess Args (args 5) | c,e-flat | inside `#if defined(HDRI) && defined(FFTW_DELEGATE)` region (lines 2972-4794 and 5152-5201), compiled only with both |
| MSESimilarityImage | static | Complex Method (cc = 25); Large Method (117); Excess Args (args 5) | c,e-flat | inside `#if defined(HDRI) && defined(FFTW_DELEGATE)` region (lines 2972-4794 and 5152-5201), compiled only with both |
| GetSimilarityForMetric | static | Complex Method (cc = 18); Large Method (103); Excess Args (args 5) | e,e-flat | dispatch switch / static args; single stub flat (file needs everything) |
| SIMVarianceImage | static | Complex Method (cc = 10); Code Duplication () | c | inside `#if defined(HDRI) && defined(FFTW_DELEGATE)` region (lines 2972-4794 and 5152-5201), compiled only with both |
| SIMUnityImage | static | Complex Method (cc = 10) | c | inside `#if defined(HDRI) && defined(FFTW_DELEGATE)` region (lines 2972-4794 and 5152-5201), compiled only with both |
| SIMSubtractImageMean | static | Complex Method (cc = 10); Code Duplication () | c | inside `#if defined(HDRI) && defined(FFTW_DELEGATE)` region (lines 2972-4794 and 5152-5201), compiled only with both |
| SIMMinimaImage | static | Complex Method (cc = 10) | c | inside `#if defined(HDRI) && defined(FFTW_DELEGATE)` region (lines 2972-4794 and 5152-5201), compiled only with both |
| SIMSquareImage | static | Complex Method (cc = 10) | c | inside `#if defined(HDRI) && defined(FFTW_DELEGATE)` region (lines 2972-4794 and 5152-5201), compiled only with both |
| SIMDivideImage | static | Complex Method (cc = 10); Code Duplication () | c | inside `#if defined(HDRI) && defined(FFTW_DELEGATE)` region (lines 2972-4794 and 5152-5201), compiled only with both |
| AccumulateAEPixel | static | Code Duplication () | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| AccumulateFUZZPixel | static | Code Duplication () | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| AccumulateMAEPixel | static | Code Duplication () | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| AccumulateMEPPPixel | static | Code Duplication () | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| AccumulateMSEPixel | static | Code Duplication () | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| AccumulatePAPixel | static | Code Duplication () | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| AccumulatePDCPixel | static | Code Duplication () | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| GetPSNRSimilarity | static | Code Duplication () | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| GetRMSESimilarity | static | Code Duplication () | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| GetDSSIMSimilarity | static | Code Duplication () | d | eleven Get*Similarity peeled = clone cliff (W17 probe 2.80); RL needs pragma edits / CF function pointers |
| SIMFilterImageNaNs | static | Code Duplication () | c | inside `#if defined(HDRI) && defined(FFTW_DELEGATE)` region (lines 2972-4794 and 5152-5201), compiled only with both |
| SIMMultiplyImage | static | Code Duplication () | c | inside `#if defined(HDRI) && defined(FFTW_DELEGATE)` region (lines 2972-4794 and 5152-5201), compiled only with both |
| SIMSetImageMean | static | Code Duplication () | c | inside `#if defined(HDRI) && defined(FFTW_DELEGATE)` region (lines 2972-4794 and 5152-5201), compiled only with both |
| PSNRSimilarityImage | static | Code Duplication (); Excess Args (args 5) | c,e-flat | inside `#if defined(HDRI) && defined(FFTW_DELEGATE)` region (lines 2972-4794 and 5152-5201), compiled only with both |
| RMSESimilarityImage | static | Code Duplication (); Excess Args (args 5) | c,e-flat | inside `#if defined(HDRI) && defined(FFTW_DELEGATE)` region (lines 2972-4794 and 5152-5201), compiled only with both |
| GetImageDistortion | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |
| GetSimilarityMetric | static | Excess Args (args 6) | e-flat | static signature; category stays while a public one remains |

### widget.c (3.02)

File-level: Number of Functions in a Single Module [IP], Overall Code Complexity [IP]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| XFontBrowserWidget | PRIVATE | Bumpy Road (9); Complex Method (cc = 104); Large Method (792) | IP |  |
| XColorBrowserWidget | PRIVATE | Bumpy Road (8); Complex Method (cc = 108); Large Method (798) | IP |  |
| XFileBrowserWidget | PRIVATE | Bumpy Road (7); Complex Method (cc = 113); Large Method (818) | IP |  |
| XCommandWidget | PRIVATE | Bumpy Road (6); Complex Method (cc = 36); Large Method (308) | IP |  |
| XNoticeWidget | PRIVATE | Bumpy Road (5); Complex Method (cc = 28); Large Method (179) | IP |  |
| XTextViewWidget | PRIVATE | Bumpy Road (5); Complex Method (cc = 66); Large Method (459); Excess Args (args 6) | IP |  |
| XDialogWidget | PRIVATE | Bumpy Road (4); Complex Method (cc = 66); Large Method (379); Excess Args (args 5) | IP |  |
| XListBrowserWidget | PRIVATE | Bumpy Road (4); Complex Method (cc = 87); Large Method (597); Excess Args (args 7) | IP |  |
| XPreferencesWidget | PRIVATE | Bumpy Road (4); Complex Method (cc = 56); Large Method (307) | IP |  |
| XScreenEvent | static | Complex Method (cc = 13) | IP |  |
| XEditText | static | Complex Method (cc = 12); Excess Args (args 5) | IP |  |
| XMenuWidget | PRIVATE | Complex Method (cc = 10); Large Method (96); Excess Args (args 5) | IP |  |
| XConfirmWidget | PRIVATE | Complex Method (cc = 10); Large Method (95) | IP |  |
| XProgressMonitorWidget | PRIVATE | Excess Args (args 5) | IP |  |

### accelerate.c (3.19)

File-level: Lines of Code in a Single File [b], Overall Code Complexity [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| ComputeContrastStretchImage | static | Complex Method (cc = 80); Large Method (328); Excess Args (args 5) | e,e-flat | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| ComputeDespeckleImage | static | Complex Method (cc = 64); Large Method (304) | e | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| ComputeEqualizeImage | static | Complex Method (cc = 55); Large Method (253) | e | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| ComputeMotionBlurImage | static | Complex Method (cc = 51); Large Method (285); Excess Args (args 6) | e,e-flat | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| ComputeLocalContrastImage | static | Complex Method (cc = 48); Large Method (247); Excess Args (args 5) | e,e-flat | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| ComputeBlurImage | static | Complex Method (cc = 36); Large Method (154); Excess Args (args 5) | e,e-flat | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| ComputeRotationalBlurImage | static | Complex Method (cc = 35); Large Method (144) | e | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| ComputeResizeImage | static | Complex Method (cc = 34); Large Method (135); Excess Args (args 6) | e,e-flat | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| ComputeUnsharpMaskImage | static | Complex Method (cc = 33); Large Method (168); Excess Args (args 7) | e,e-flat | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| ComputeWaveletDenoiseImage | static | Complex Method (cc = 31); Large Method (123) | e | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| ComputeUnsharpMaskImageSingle | static | Complex Method (cc = 25); Large Method (112); Excess Args (args 7) | e,e-flat | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| ComputeModulateImage | static | Complex Method (cc = 24); Large Method (133); Excess Args (args 7) | e,e-flat | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| ComputeFunctionImage | static | Complex Method (cc = 22); Large Method (100); Excess Args (args 6) | e,e-flat | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| checkAccelerateCondition | static | Complex Method (cc = 18) | e | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| ComputeContrastImage | static | Complex Method (cc = 17); Large Method (72) | e | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| resizeVerticalFilter | static | Complex Method (cc = 15); Large Method (132); Excess Args (args 15) | e,e-flat | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| resizeHorizontalFilter | static | Complex Method (cc = 15); Large Method (143); Excess Args (args 15) | e,e-flat | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| ComputeGrayscaleImage | static | Complex Method (cc = 12) | e | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| AccelerateGrayscaleImage | PRIVATE | Complex Method (cc = 9); Complex Conditional (2 complex conditional expressions) | e | each single stub flat (3.19); 9 of 17 stubbed 4.06, all 17 6.76: a 17-function chain |
| AccelerateBlurImage | PRIVATE | Code Duplication () | d | ten wrappers clone through the assert/trace preamble (moving it moves GetMagickModule(); owner) |
| AccelerateContrastImage | PRIVATE | Code Duplication () | d | ten wrappers clone through the assert/trace preamble (moving it moves GetMagickModule(); owner) |
| AccelerateContrastStretchImage | PRIVATE | Code Duplication () | d | ten wrappers clone through the assert/trace preamble (moving it moves GetMagickModule(); owner) |
| AccelerateDespeckleImage | PRIVATE | Code Duplication () | d | ten wrappers clone through the assert/trace preamble (moving it moves GetMagickModule(); owner) |
| AccelerateEqualizeImage | PRIVATE | Code Duplication () | d | ten wrappers clone through the assert/trace preamble (moving it moves GetMagickModule(); owner) |
| AccelerateFunctionImage | PRIVATE | Code Duplication (); Excess Args (args 5) | a,d | ten wrappers clone through the assert/trace preamble (moving it moves GetMagickModule(); owner) |
| AccelerateLocalContrastImage | PRIVATE | Code Duplication () | d | ten wrappers clone through the assert/trace preamble (moving it moves GetMagickModule(); owner) |
| AccelerateMotionBlurImage | PRIVATE | Code Duplication (); Excess Args (args 5) | a,d | ten wrappers clone through the assert/trace preamble (moving it moves GetMagickModule(); owner) |
| AccelerateRotationalBlurImage | PRIVATE | Code Duplication () | d | ten wrappers clone through the assert/trace preamble (moving it moves GetMagickModule(); owner) |
| AccelerateWaveletDenoiseImage | PRIVATE | Code Duplication () | d | ten wrappers clone through the assert/trace preamble (moving it moves GetMagickModule(); owner) |
| createKernelInfo | static | Excess Args (args 5) | e-flat | static signature; category stays while a public one remains |
| LaunchHistogramKernel | static | Excess Args (args 8) | e-flat | static signature; category stays while a public one remains |
| AccelerateModulateImage | PRIVATE | Excess Args (args 6) | a | public/MagickPrivate signature |
| AccelerateResizeImage | PRIVATE | Excess Args (args 5) | a | public/MagickPrivate signature |
| AccelerateUnsharpMaskImage | PRIVATE | Excess Args (args 6) | a | public/MagickPrivate signature |

### composite.c (3.69)

File-level: Lines of Code in a Single File [b], Overall Code Complexity [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| CompositeImage | PUBLIC | Bumpy Road (4); Complex Method (cc = 72); Large Method (471); Excess Args (args 7) | a,d | setup switch writes nine caller variables: I/ST with nine targets (owner) |
| CompositeChannelColor | static | Complex Method (cc = 188); Large Method (802) | d | label-run helpers cc 9+ / clone cliff / function-pointer table (W15-W17) |
| CompositeAlphaChannel | static | Complex Method (cc = 41); Large Method (168) | d | label-run helpers cc 9+ / clone cliff / function-pointer table (W15-W17) |
| SetCompositeCoverage | static | Complex Method (cc = 34) | d | label-run helpers cc 9+ / clone cliff / function-pointer table (W15-W17) |
| BlendDivergentImage | static | Complex Method (cc = 20); Large Method (114) | d | `FreeDivergentResources` macro: Recipe H for what it measures (owner) |
| SeamlessBlendImage | static | Complex Method (cc = 19); Large Method (117); Excess Args (args 8) | e,e-flat | single stub flat (3.69); gain only when many functions are clean together |
| SaliencyBlendImage | static | Complex Method (cc = 17); Large Method (113); Excess Args (args 8) | e,e-flat | single stub flat (3.69); gain only when many functions are clean together |
| SetVirtualCompositeChannels | static | Complex Method (cc = 16) | e | single stub flat (3.69); gain only when many functions are clean together |
| CompositeImageChannels | static | Complex Method (cc = 13) | e | single stub flat (3.69); gain only when many functions are clean together |
| CompositeImagePixel | static | Complex Method (cc = 12) | e | single stub flat (3.69); gain only when many functions are clean together |
| IsBlendRowsMissing | static | Excess Args (args 5) | e-flat | static signature; category stays while a public one remains |

### animate.c (4.40)

File-level: Lines of Code in a Single File [b], Overall Code Complexity [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| XAnimateBackgroundImage | PUBLIC | Bumpy Road (9); Complex Method (cc = 57); Large Method (317) | e2 | function-statics whose address also goes to calls + ThrowXWindowFatalException blocks: no recipe admits |
| XAnimateImages | PUBLIC | Bumpy Road (5); Complex Method (cc = 74); Large Method (769); Excess Args (args 6) | a,e2 | setup blocks naming statics/throw macros; cc 74, ~770 lines |
| XSaveImage | static | Bumpy Road (2); Excess Args (args 5) | e-flat,e1 | two bumps hold returns in unreached blocks |
| XMagickCommand | static | Complex Method (cc = 9); Excess Args (args 7) | e,e-flat | cc 9 (one over): OpenCommand case with block `static filenames` + throws |

### fx.c (4.61)

File-level: Number of Functions in a Single Module [b], Overall Code Complexity [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| ExecuteRPN | ? | Complex Method (cc = 221); Large Method (660) | d | parked evaluator split chain (owner) |
| FxEvaluateChannelExpression | ? | Excess Args (args 6) | a | public/MagickPrivate signature |

### opencl.c (5.80)

File-level: Number of Functions in a Single Module [IP]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| LoadOpenCLDeviceBenchmark | static | Bumpy Road (4); Deep Nesting (depth 5); Complex Method (cc = 23); Large Method (83) | IP |  |
| GetOpenCLCacheDirectory | static | Bumpy Road (2); Deep Nesting (depth 4); Complex Method (cc = 11) | IP |  |
| BenchmarkOpenCLDevices | static | Bumpy Road (2) | IP |  |
| IsSameOpenCLDevice | ? | Complex Conditional (5 complex conditional expressions); Code Duplication () | IP |  |
| IsBenchmarkedOpenCLDevice | ? | Complex Conditional (5 complex conditional expressions); Code Duplication () | IP |  |
| GetOpenCLEnabled | PUBLIC | Code Duplication () | IP |  |
| GetOpenCLPlatformString | static | Code Duplication () | IP |  |
| GetOpenCLDeviceString | static | Code Duplication () | IP |  |
| SetOpenCLEnabled | PUBLIC | Code Duplication () | IP |  |
| CompileOpenCLKernel | static | Excess Args (args 5) | IP |  |
| EnqueueOpenCLKernel | PRIVATE | Excess Args (args 10) | IP |  |

### morphology.c (6.19)

File-level: Number of Functions in a Single Module [b], Overall Code Complexity [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| AcquireKernelBuiltIn | PUBLIC | Bumpy Road (4); Complex Method (cc = 56); Large Method (140) | d | open `#if 1`/`#else` and `#if 0` label question (W17) |
| MorphologyApply | PRIVATE | Complex Method (cc = 22); Large Method (101); Excess Args (args 7) | a,e0 | Smooth group + 7-label two-stage run: any helper cc 9+, probe 6.13 < 6.19 |
| AcquireFreiChenKernel | static | Complex Method (cc = 16) | e0 | cases 15-19 shortening clones EdgeBasis (5.91 probe) |
| ParseKernelArray | static | Complex Method (cc = 10) | c | `#if 0`/`#else` inside |
| MorphologyImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |

### fourier.c (7.59)

File-level: Lines of Code in a Single File [b], Overall Code Complexity [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| ForwardFourierTransformImage | PUBLIC | Bumpy Road (2); Complex Method (cc = 14); Large Method (126) | c | bodies in `#if !defined(ENABLE_FFTW_DELEGATE)`/`#else` and `HAVE_COMPLEX_H` pairs (compiled side of an `#if defined`: owner question) |
| InverseFourier | static | Bumpy Road (2) | c | bodies in `#if !defined(ENABLE_FFTW_DELEGATE)`/`#else` and `HAVE_COMPLEX_H` pairs (compiled side of an `#if defined`: owner question) |
| InverseFourierTransformImage | PUBLIC | Complex Method (cc = 16); Large Method (115) | c | bodies in `#if !defined(ENABLE_FFTW_DELEGATE)`/`#else` and `HAVE_COMPLEX_H` pairs (compiled side of an `#if defined`: owner question) |

### feature-texture.c (7.63)

File-level: Lines of Code in a Single File [b], Overall Code Complexity [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| GetImageFeatures | PUBLIC | Bumpy Road (2); Complex Method (cc = 21); Large Method (245) | d | grays-fill loop writing the shared `grays` table (OpenMP shared-table question) + GetMagickModule throw exits |

### distort.c (7.70)

File-level: Number of Functions in a Single Module [b], Overall Code Complexity [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| poly_basis_dx | static | Complex Method (cc = 17) | e0 | hot path; table/chain forms 5-20% slower and clone `poly_basis_fn` |
| SetDistortNoBestFit | static | Complex Method (cc = 9) | d | label deletion / `#if 0` labels (owner question) |
| DistortImage | PUBLIC | Excess Args (args 6) | a | public/MagickPrivate signature |
| SparseColorImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |

### attribute-depth-measure.c (8.16)

File-level: Overall Code Complexity [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| GetImageDepth | PUBLIC | Bumpy Road (4); Deep Nesting (depth 4); Complex Method (cc = 24); Large Method (124) | c | same `#if !HDRI` depth_map block |

### attribute-depth.c (8.16)

File-level: Overall Code Complexity [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| SetImageDepth | PUBLIC | Bumpy Road (3); Deep Nesting (depth 4); Complex Method (cc = 22); Large Method (124) | c | `#if !defined(MAGICKCORE_HDRI_SUPPORT)` branch (depth_map block), not compiled by the oracle |

### layer.c (8.25)

File-level: Lines of Code in a Single File [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| OptimizeLayerFrames | static | Bumpy Road (3); Complex Method (cc = 30); Large Method (215) | d | `#if DEBUG_OPT_FRAME` (`#if M`, M defined 0) blocks + ThrowImageException exits |

### paint-floodfill.c (8.35)

File-level: Overall Code Complexity [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| FloodfillPaintImage | PUBLIC | Bumpy Road (3); Complex Method (cc = 19); Large Method (138); Excess Args (args 7) | a,d | every bump holds the function-local `PushSegmentStack` macro (returns through ThrowBinaryException): Recipe H / throw-macro question |

### visual-effects-plasma.c (8.50)

File-level: Overall Code Complexity [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| PlasmaImageProxy | static | Bumpy Road (4); Complex Method (cc = 21); Large Method (103) | e0 | four edge blocks: helpers clone each other (8.15 probe) |
| SetPlasmaPixelChannels | static | Excess Args (args 6) | e-flat | static signature: fixable by B/M/I but flat while public ones keep the category |
| PlasmaImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |

### resample.c (8.53)

| function | linkage | findings | class | note |
|---|---|---|---|---|
| ResamplePixelColor | PUBLIC | Bumpy Road (2); Complex Method (cc = 11); Large Method (116); Excess Args (args 5) | a,d | `#if DEBUG_ELLIPSE` (`#if M`, M=0) and `#else` code |
| ScaleResampleFilter | PUBLIC | Large Method (95); Excess Args (args 5) | a,d | `#if EWA`/`EWA_CLAMP`/`DEBUG_ELLIPSE` (`#if M`) code |
| ClampUpAxes | static | Excess Args (args 10) | e-flat | static signature: fixable by B/M/I but flat while public ones keep the category |

### quantize.c (8.81)

File-level: Number of Functions in a Single Module [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| KmeansImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |

### magnify.c (8.84)

File-level: Overall Code Complexity [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| Scale3X | static | Complex Method (cc = 12) | e | 998 of 1,000 lines: helpers cross the LoC threshold unless FB-style steps free lines |
| Xbr2X | static | Complex Method (cc = 9) | e | 998 of 1,000 lines: helpers cross the LoC threshold unless FB-style steps free lines |

### compare.c (8.92)

| function | linkage | findings | class | note |
|---|---|---|---|---|
| IsImagesEqual | PUBLIC | Bumpy Road (2) | e |  |
| CompareImages | PUBLIC | Complex Method (cc = 19); Large Method (130); Excess Args (args 5) | a,e | public/MagickPrivate signature |

### segment.c (9.02)

File-level: Lines of Code in a Single File [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| GetImageDynamicThreshold | PUBLIC | Bumpy Road (2); Excess Args (args 5) | a,e1 | unreached function: only E/G/P allowed; neither bump fits; stub +0.07 |
| SegmentImage | PUBLIC | Excess Args (args 6) | a | public/MagickPrivate signature |

### colorspace-convert.c (9.08)

| function | linkage | findings | class | note |
|---|---|---|---|---|
| ConvertRGBToGeneric | PRIVATE | Complex Method (cc = 20); Large Method (113); Excess Args (args 9) | a,e0 | label-run SD chain clones (7.78/8.28 vs 9.08); TB/KT need constant cases |
| ConvertHSLToRGB | PUBLIC | Excess Args (args 6) | a | public/MagickPrivate signature |
| ConvertRGBToHSL | PUBLIC | Excess Args (args 6) | a | public/MagickPrivate signature |

### effect-blur.c (9.09)

File-level: Lines of Code in a Single File [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| BilateralBlurImage | PUBLIC | Excess Args (args 6) | a | public/MagickPrivate signature |
| MotionBlurImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |
| SelectiveBlurImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |

### effect-filter.c (9.09)

File-level: Lines of Code in a Single File [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| ShadeImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |
| UnsharpMaskImage | PUBLIC | Excess Args (args 6) | a | public/MagickPrivate signature |

### enhance-tone.c (9.09)

File-level: Lines of Code in a Single File [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| EqualizeImageRow | static | Excess Args (args 5) | e-flat | static signature: fixable by B/M/I but flat while public ones keep the category |
| LevelImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |
| LevelizeImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |

### enhance.c (9.09)

File-level: Lines of Code in a Single File [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| SigmoidalContrastImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |

### feature.c (9.09)

File-level: Lines of Code in a Single File [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| CannyEdgeImage | PUBLIC | Excess Args (args 6) | a | public/MagickPrivate signature |
| HoughLineImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |
| MeanShiftImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |

### threshold.c (9.09)

File-level: Lines of Code in a Single File [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| RangeThresholdValue | static | Excess Args (args 5) | e-flat | static signature: fixable by B/M/I but flat while public ones keep the category |
| RangeThresholdChannels | static | Excess Args (args 6) | e-flat | static signature: fixable by B/M/I but flat while public ones keep the category |
| RangeThresholdImage | PUBLIC | Excess Args (args 6) | a | public/MagickPrivate signature |

### visual-effects-pixel.c (9.09)

File-level: Lines of Code in a Single File [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|
| PolaroidImage | PUBLIC | Excess Args (args 6) | a | public/MagickPrivate signature |
| ShadowImage | PUBLIC | Excess Args (args 6) | a | public/MagickPrivate signature |
| SketchImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |
| TintPixel | static | Excess Args (args 5) | e-flat | static signature: fixable by B/M/I but flat while public ones keep the category |
| VignetteImage | PUBLIC | Excess Args (args 6) | a | public/MagickPrivate signature |
| WaveImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |

### montage.c (9.11)

| function | linkage | findings | class | note |
|---|---|---|---|---|
| MontageImageList | PUBLIC | Complex Method (cc = 10); Large Method (164) | e | about 1 line under 1,000: needs ~94 lines freed first |

### colorspace-transform.c (9.38)

File-level: Lines of Code in a Single File [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|

### statistic.c (9.38)

File-level: Lines of Code in a Single File [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|

### transform-region.c (9.38)

File-level: Lines of Code in a Single File [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|

### vision-components.c (9.38)

File-level: Lines of Code in a Single File [b]

| function | linkage | findings | class | note |
|---|---|---|---|---|

### colorspace-generic-rgb.c (9.68)

| function | linkage | findings | class | note |
|---|---|---|---|---|
| ConvertGenericToRGB | PRIVATE | Excess Args (args 9) | a | public/MagickPrivate signature |

### enhance-clahe.c (9.68)

| function | linkage | findings | class | note |
|---|---|---|---|---|
| GenerateCLAHEHistogram | static | Excess Args (args 6) | e-flat | static signature: fixable by B/M/I but flat while public ones keep the category |
| InterpolateCLAHE | static | Excess Args (args 8) | e-flat | static signature: fixable by B/M/I but flat while public ones keep the category |
| CLAHE | static | Excess Args (args 6) | e-flat | static signature: fixable by B/M/I but flat while public ones keep the category |
| CLAHEImage | PUBLIC | Excess Args (args 6) | a | public/MagickPrivate signature |

### enhance-level-colors.c (9.68)

| function | linkage | findings | class | note |
|---|---|---|---|---|
| LevelImageColors | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |

### histogram.c (9.68)

| function | linkage | findings | class | note |
|---|---|---|---|---|
| MinMaxStretchImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |
| UniqueColorsToImage | static | Excess Args (args 5) | e-flat | static signature: fixable by B/M/I but flat while public ones keep the category |

### layer-composite.c (9.68)

| function | linkage | findings | class | note |
|---|---|---|---|---|
| CompositeCanvas | static | Excess Args (args 6) | e-flat | static signature: fixable by B/M/I but flat while public ones keep the category |
| CompositeLayers | PUBLIC | Excess Args (args 6) | a | public/MagickPrivate signature |

### paint.c (9.68)

| function | linkage | findings | class | note |
|---|---|---|---|---|
| GradientImage | PUBLIC | Excess Args (args 6) | a | public/MagickPrivate signature |
| OpaquePaintImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |
| TransparentPaintImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |
| TransparentPaintImageChroma | PUBLIC | Excess Args (args 6) | a | public/MagickPrivate signature |

### resize-image.c (9.68)

| function | linkage | findings | class | note |
|---|---|---|---|---|
| ResizeImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |

### resize-rescale.c (9.68)

| function | linkage | findings | class | note |
|---|---|---|---|---|
| InterpolativeResizeImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |
| LiquidRescaleImage | PUBLIC | Excess Args (args 6); Excess Args (args 5) | a | public/MagickPrivate signature |
| ResampleImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |

### statistic-function.c (9.68)

| function | linkage | findings | class | note |
|---|---|---|---|---|
| FunctionImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |

### statistic-image.c (9.68)

| function | linkage | findings | class | note |
|---|---|---|---|---|
| GetStatisticPixel | static | Excess Args (args 8) | e-flat | static signature: fixable by B/M/I but flat while public ones keep the category |
| StatisticImagePixel | static | Excess Args (args 10) | e-flat | static signature: fixable by B/M/I but flat while public ones keep the category |
| StatisticImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |

### threshold-adaptive.c (9.68)

| function | linkage | findings | class | note |
|---|---|---|---|---|
| SumThresholdWindow | static | Excess Args (args 7) | e-flat | static signature: fixable by B/M/I but flat while public ones keep the category |
| AdaptiveThresholdImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |

### visual-effects-stegano.c (9.68)

| function | linkage | findings | class | note |
|---|---|---|---|---|
| StereoAnaglyphImage | PUBLIC | Excess Args (args 5) | a | public/MagickPrivate signature |

# Appendix B: probe log (scores of stubbed copies; base in brackets)

| probe | result |
|---|---|
| compare.c [8.92] CompareImages + IsImagesEqual stubbed | 9.68 (CompareImages alone 9.61, IsImagesEqual alone 9.00) |
| compare.c helper-chain model (MakeCompareDifference, MakeCompareHighlight, GetCompareColors, HighlightCompareRows, FindUnequalColumn) | 9.09 with *Overall Code Complexity* newly flagged; Excess Args only CompareImages (public) |
| compare.c plus two trivial cc-1 functions, unchanged otherwise | 8.39 (OCC flagged; one function: 8.92) |
| magnify.c [8.84] Scale3X / Xbr2X / both (padded) | 9.09 / 9.07 / 9.38 |
| montage.c [9.11] MontageImageList (padded) | 9.38 (Lines of Code reappears from the stub's own lines) |
| animate.c [4.40] XMagickCommand / XSaveImage / XAnimateBackgroundImage / XAnimateImages | 4.49 / 4.43 / 5.30 / 6.73 |
| accelerate.c [3.19] AccelerateGrayscaleImage | 3.27 |
| accelerate.c each of 17 Compute*/resize* functions alone | 3.19 each; five 3.12 (stubs add Code Duplication); nine 4.06; all 17 6.76 |
| composite.c [3.69] each of SaliencyBlendImage, SeamlessBlendImage, BlendDivergentImage, SetVirtualCompositeChannels, CompositeImageChannels, CompositeImagePixel, CompositeAlphaChannel, SetCompositeCoverage | 3.69 each; CompositeChannelColor 4.08; CompositeImage 3.71; pixel path five functions 4.81 |
| compare-similarity.c [2.97] GetSimilarityForMetric, GetSimilarityMetric, DPC/NCC/PHASE/SSIM, SimilarityImage | 2.97 each |
| morphology.c [6.19] MorphologyApply / AcquireFreiChenKernel / ParseKernelArray / AcquireKernelBuiltIn | 6.56 / 6.46 / 6.35 / 7.25 |
| visual-effects-plasma.c [8.50] PlasmaImageProxy | 9.68 |
| colorspace-convert.c [9.08] ConvertRGBToGeneric | 9.68 |
| paint-floodfill.c [8.35] FloodfillPaintImage | 9.09 |
| layer.c [8.25] OptimizeLayerFrames | 9.38 |
| feature-texture.c [7.63] GetImageFeatures | 9.38 |
| distort.c [7.70] poly_basis_dx + SetDistortNoBestFit / poly_basis_dx alone | 8.28 / 8.03 |
| resample.c [8.53] ResamplePixelColor / ScaleResampleFilter / both | 9.17 / 9.00 / 9.68 |
| attribute-depth.c SetImageDepth; attribute-depth-measure.c GetImageDepth | 10.0 each (from 8.16) |
| fourier.c [7.59] Forward / Inverse / InverseFourier / all three | 8.20 / 8.08 / 7.64 / 9.38 |
| fx.c [4.61] ExecuteRPN | 8.28 |
| segment.c [9.02] GetImageDynamicThreshold | 9.09 |
| enhance-clahe.c [9.68] three static signatures cut to 4 arguments, one by one | 9.68, 9.68, 9.68 (flat) |
| enhance-tone.c [9.09] EqualizeImageRow | 9.09 |

Observations that matter to the lanes:

- A static signature fix is flat wherever a public function keeps *Excess Number of Function Arguments*; this covers
  nine files (e-flat). No file in campaign A has only static offenders.
- Stubbed bodies of the huge functions do not add: accelerate.c needs nine of its seventeen Compute* functions clean
  before the score moves at all, composite.c about the whole pixel path.
- `ch.py` occasionally prints `unscorable` for a probe copy; rerunning the same file scores it (seen twice, transient).
- `HasUndefinedRGBATraits` in accelerate.c tests four traits; the `AccelerateGrayscaleImage` test names three, so a
  Recipe D reuse is not available and Recipe P needs a new predicate.
- fx.c's `FxEvaluateChannelExpression` and morphology.c's `MorphologyApply` are `MagickPrivate` (argument counts are (a)).
- Possible upstream, not changed: none found in this read-only pass.
