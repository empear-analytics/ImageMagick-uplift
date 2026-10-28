# M101 - Recipe B after uncapped mutation runs

Not a generated task file. PR #54 (M100) was merged at `04ccce3dc` on the afternoon of
2026-10-27; this round continues on `campaign/M101`, stacked on that merge (`50e8137d9`,
also the oracle's baseline). Measured with `cs-mcp` 1.5.13.

### Report (2026-10-27, Mac, evening)

**quantum-import.c 1.41 → 1.73, quantum-export.c 1.57 → 1.87, morphology.c 2.04 → 2.20,
distort.c 2.29 → 2.49, configure.c 6.01 → 6.19, transform.c 2.65 → 2.71, shear.c 2.18 →
2.72**, one Recipe B per commit. Most remove *Brain Method* with *Excess Number of
Function Arguments*; configure.c, transform.c and shear.c lose their last argument
finding.

| Function | Bundle | Evidence | Code Health |
|---|---|---|---|
| ImportRGBQuantum | `QuantumPixelSpan` (M99) | oracle, ready (95%, uncapped `q100`) | 1.41 → 1.48 |
| ImportIndexQuantum | the same | oracle, ready (95%) | 1.48 → 1.55 |
| ImportGrayQuantum | the same | oracle, careful (80%) | 1.55 → 1.63 |
| ExportGrayQuantum | export's `QuantumPixelSpan` (M99) | oracle, careful (85%) | 1.57 → 1.65 |
| ExportRGBQuantum | the same | oracle, ready (92%) | 1.65 → 1.75 |
| ExportRGBAQuantum | the same | oracle, ready (100%) | 1.75 → 1.87 |
| MorphologyPrimitive | method, kernel, bias (`MorphologyPass`, new) | oracle, careful (79%, ERDC fresh capped run) | 2.04 → 2.20 |
| GenerateCoefficients | number_arguments, arguments, number_values (`CoefficientInput`, new) | oracle, careful (86%, uncapped `q100`) | 2.29 → 2.49 |
| ImportCbYCrYQuantum | `QuantumPixelSpan` | oracle, ready (100%, uncapped `n100`) | 1.63 → 1.73 |
| configure.c AddConfigureKey | cache, path (`ConfigureKeyTarget`, new; the literal `"[built-in]"` at all five calls) | oracle, ready (2/2, `n100`), 9 cases | 6.01 → 6.19, the file's last argument finding |
| transform.c CopyImageRegion | columns, rows, sx, sy, dx, dy (`CopyRegion`, new; one per call, amendment below) | oracle, ready (19/19, `n100`), 26 cases | 2.65 → 2.71, the file's last argument finding |
| shear.c XShearImage | width, height, x_offset, y_offset (`ShearRegion`, new; one per call) | oracle, careful (88%, `n100`), 43 cases | 2.18 → 2.45 |
| shear.c YShearImage | the same struct | oracle, careful (78%), 43 cases | 2.45 → 2.67 |
| shear.c CropToFitImage | x_shear, y_shear, width, height, rotate (`CropToFitRegion`, new) | oracle, careful (89%), 43 cases | 2.67 → 2.72, the file's last argument finding |

| annotate.c RenderType | image, draw_info (`RenderTarget`, new) | oracle, careful (80%, `n101`), 218 cases | 2.33 → 2.53 |

| annotate.c RenderFreetype | image, draw_info, encoding (`FreetypeTarget`, new; one per call) | oracle, careful (79%, `n101`), 198 cases; the no-FreeType stub compiled separately | 2.53 → 2.77 |

### The owner's decision: no mutation run before a step (evening)

The owner stopped the per-step mutation runs: the harness was built under mutation
guidance and is finished, and the 3SX and Jellyfin campaigns did not gate steps this
way. *Two levels of evidence* now reads: a function some case executes
(`casemap.py`) is oracle-verified for every recipe; the guard level stays for code no
case reaches. AGENTS.md and CLAUDE.md say the same (`5a8c61007`). Steps from here on cite
case reach and the oracle run.

| File | Function | Recipe | Evidence | Code Health |
|---|---|---|---|---|
| policy.c | LoadPolicyCache (`PolicyLoadTarget`) | B | 15,903 cases; zero-configuration branch compiled separately | 3.85 → 4.39 |
| color.c | LoadColorCache (`ColorLoadTarget`) | B | 1,391 cases | 3.16 → 3.49 |
| matrix.c | GaussJordanElimination's macros | H | IDENTICAL preprocessed text | 6.76 → 6.84 |
| matrix.c | UnscrambleColumns, SwapMatrixRows, SwapVectorEntries, EliminateColumn | E (4 commits) | 202 cases | 6.84 → 7.17 |
| matrix.c | FindPivot (sets row, column, max) | O | 202 cases | 7.17 → 7.27 |
| matrix.c | GetMatrixRange, GetMatrixScale (MatrixToImage) | O (2 commits) | 22 cases | 7.27 → 7.45 |
| color.c | SetDeviceColorspace, SetICCColorspace (QueryColorFunction) | O (2 commits) | 143 cases, device- and icc-color among them | 3.49 → 3.55 |
| constitute.c | SetInterpretedProperty (ReadImage's caption/comment/label) | D | 14,969 cases | 4.45 → 4.48 |
| constitute.c | EncodeImageWithInfo (WriteImage's two encode blocks) | D | 13,088 cases | 4.48 → 4.52 |
| constitute.c | FindFallbackEncoder, SyncEndianFromMagickInfo (WriteImage) | O, E | 13,088 cases, 251 on the fallback path | 4.52 → 4.65 |
| fx.c | ExecuteRPN's six HSL-qualifier tests → existing IsHslChannelQualifier | P (several sites) | 767 cases | 1.93 → 2.15 |
| fx.c | TranslateExpression's two user-symbol tests → IsUnassignedUserSymbolRead | P (several sites) | 821 cases | 2.15 → 2.28, no Complex Conditional left |
| image.c | GetFilenameOption (InterpretImageFilename's %[name] lookup) | E | 12,656 cases | 2.20 → 2.24 |
| image.c | CopyPixelChannels (CopyImagePixels' channel loop, inside OpenMP) | E | 54 cases; Brain Method gone | 2.24 → 2.49 |
| threshold.c | RangeThresholdValue (RangeThresholdImage's ladder) | E | 13 cases | 2.08 → 2.12 |
| threshold.c | SumThresholdWindow (AdaptiveThresholdImage's first window) | K | 12 cases | 2.12 → 2.16 |
| effect.c | UnsharpPixelChannels (UnsharpMaskImage's channel loop) | E | 53 cases; Brain Method gone | 1.40 → 1.48 |
| effect.c | GetSurfaceShade, ShadePixelChannels (ShadeImage) | E (2 commits) | 17 cases; Brain Method gone | 1.48 → 1.59 |
| paint.c | SetUpdatableFillChannels (OpaquePaintImage) | E | 36 cases; Brain Method gone | 3.33 → 3.82 |
| paint.c | SetMaskedFillChannels (FloodfillPaintImage) | E | 41 cases | 3.82 → 3.87 |
| paint.c | OilPaintImage's macros | H | IDENTICAL preprocessed text (M47's blocker) | flat at 3.87 |
| paint.c | GetMostFrequentOffset, SetOilPaintChannels (OilPaintImage) | E (2 commits) | 15 cases; Brain Method and Deep Nesting gone | 3.87 → 4.14 |
| feature.c | GetHoughLine, GetHoughMaxima (HoughLineImage) | E (2 commits) | 19 cases; nesting 7 → 4 | 1.94 → 2.01 |
| feature.c | GetCannyGradient (CannyEdgeImage) | K | 12 cases; Brain Method gone | 2.01 → 2.21 |
| quantize.c | ReduceKmeansPixels (KmeansImage) | E | 22 cases | 2.09 → 2.14 |
| visual-effects.c | AnnotatePolaroidCaption (PolaroidImage) | O | 5 cases with captions; Brain Method gone | 2.41 → 2.63 |

The pattern of the night: a public function's Brain Method often sits in one per-pixel
channel loop inside its parallel row loop that writes only through `q`. Moved out by
Recipe E (allowed inside OpenMP for a block that writes only the iteration's own data),
the function loses Brain Method and Deep Nesting, and the file gains 0.1 to 0.5.
Refused: the blur loops in effect.c (MotionBlur, SelectiveBlur, Bilateral) and
MeanShiftImage, which write the shared `status` and accumulators in the same block
(neither K nor T covers that); StatisticImage, whose accumulation writes five values;
AdaptiveThresholdImage's sliding window (extracted, flat, reverted). Also flat and reverted: HoughLineImage's vote loop, KmeansImage's centroid update and per-pixel accumulation (its depth-5 nesting is the nearest-centroid search, which writes `k` and `min_distance` inside the parallel loop), cache.c's constant virtual pixel, SparseColorImage's final scaling (its depth-6 cases need eight or nine arguments), compare.c's SimilarityImage channel loop (the file's findings sit in its FFTW-only region). Not attempted: IsEquivalentImage (the match test writes six variables), ProfileImage (LCMS `#if` code).

Most of these were left by earlier tasks (M17, M21, M38, M48) only because the
recipe needed the oracle level and the function's mutation score was below the gate.

**New cases: SetMagickSecurityPolicy.** No case reached it (only the API calls it); a new
`imdriver` command, `secpolicy XML [NAME...]`, and six cases (a coder rule, resource
values, both, malformed XML, includes of a provided and a missing file) now do
(`56a0ad2ab`). The case
map was rebuilt (15,906 cases).

**Not done.** MorphologyApply: its method switch sets five variables and its per-stage
switch four (Edge also swaps `curr_image` and `save_image`), so Recipe O helpers would
take seven to nine arguments. distort.c's best-fit bounds: each case writes `min` and
`max` through macros defined mid-function that name the caller's locals. property.c and
colorspace.c tables: Recipe F left the score flat (above).

### Morning (2026-10-28): the owner's three answers

The owner answered the night's questions: "1 and 2 are ok, go ahead! you can try 3."

**1. pixel.c's five fixed-map extractions in one commit** (rule 1 amended, `fda56c183`):
`99af5d46d`, pixel.c 1.73 → 1.80.

**2. Recipe S for the largest files.** Three splits, each verbatim, each static moving
with its only caller:

| Commit | Family | Original | New file |
|---|---|---|---|
| `fe5a33124` | `ExportImagePixels`, `ExportPixelBuffer`, 56 statics | pixel.c 1.80 → 1.89 | pixel-export.c 3.11 |
| `f8fc17c71` | `ImportImagePixels`, `ImportPixelBuffer`, 58 statics | pixel.c 1.89 → 2.36 | pixel-import.c 3.12 |
| `6343c01e0` | `ResizeImage`, `ContributionInfo`, 11 statics | resize.c 2.89 → 3.55 | resize-image.c 6.29 |

Each passed the combined guards over the pair and the oracle on every case that reaches
the moved public function (135, 129 and 911 cases, 0 diverged); the builds are clean and
nm shows each public function exported from its new object. As for magnify.c, the
pre-commit gate's rule that a new file score 10.0 was skipped (the gate's check of the
original passed) and the trailer check was run by hand; Recipe S now says so (`bcc48ab84`).
The Windows build configuration outside this repository still needs the three files.
`tools/register_source.py` lists a new file in the four build lists;
`tools/split_source.py` finds and moves a family and reproduces the resize.c split byte
for byte.

The other approved files do not split cleanly, by the tool's rules:
- draw.c (`DrawPrimitive` reaches two statics other functions call), morphology.c,
  cache.c and fx.c: the large families share statics, which would have to widen.
- compare.c: the metric functions serve both `CompareImages` and `SimilarityImage`.
- blob.c (`OpenBlob`, macro `_O_BINARY`) and property.c (`GetMagickProperty`, macro
  `cmsUInt32Number`): a file-level macro would be needed on both sides.
- quantum-import.c: one public function is the whole file.
- enhance.c: `ModulateImage`, `ContrastStretchImage` and `EqualizeImage` split cleanly,
  but enhance.c moves by 0.07 at most, so none was made.

Outside the approved list, statistic.c's `StatisticImage` family splits cleanly
(2.29 → 2.61 measured on the dry run, before the two steps below), and so would
colorspace.c's, but for a macro. They wait for the owner.

**The split costs the per-file mean.** A split-off family keeps its findings, so the new
file is usually red: MagickCore's mean is 4.96 at `bcc48ab84` against 4.97 at `a055db34e`,
with red 44 → 46, although no file went down. Weighted by lines, it rose from 3.149 to
3.200. The splits pay off for the file split, not for the per-file mean.

**3. Shared status plus accumulators: Recipe J** (`3f7696c06`, Recipes T and K together:
the helper returns the block's status and takes the accumulators by pointer). Its
instance is vision.c: the four moment-threshold functions share a first moment pass that
writes `status=MagickFalse` and the object's own `M00`, `M10` and `M01`. J extracted it from
`MajorAxisThreshold` (`2de73eed9`, 2.35 → 2.45), and Recipe D put the same call in
`MinorAxisThreshold`, `EccentricityThreshold` and `AngleThreshold` (`e943876c7`,
`548e9bff5`, `444ddfbd2`): 2.35 → 2.59. Amended for array accumulators (`312716201`), J then
moved PerimeterThreshold's pattern-counting row loop to its own helper (`369afb5c9`), and
PerimeterThreshold has no findings left: **vision.c 2.35 → 2.66**. CircularityThreshold
holds the same loop, but the call there measures 2.62, so it was not made. Each step reached its function's
cases (2, 2, 2, 4 and 2, all verbose connected-components output), 0 diverged. The
second moment pass, also in all four, starts with `if (status == MagickFalse) continue;`:
it reads the shared status, which neither T nor J allows, and stays.

The first try, `MotionBlurImage`, did not pay. Its two kernel loops write
`status=MagickFalse` and one or two of the channel's sums, but the helpers need nine to
eleven arguments. Measured, not committed:

| Extraction | effect.c (1.59) | MotionBlurImage's Brain Method |
|---|---|---|
| plain kernel loop (`pixel` by pointer, 9 arguments) | 1.61 | stays |
| blended kernel loop (`pixel`, `gamma`, 11 arguments) | 1.57 | stays |
| both | 1.57 (the two helpers duplicate each other) | stays |
| Recipe T on the whole column loop | 1.58 | moves to the helper |

The other targets named last night do not write `status` in their accumulating blocks:
`StatisticImage` took Recipe E twice instead (`517fe1865`, flat, then `7c283e05c`,
2.29 → 2.32); `SelectiveBlurImage`, `BilateralBlurImage` and `MeanShiftImage` have
Recipe K's shape, but their blocks also advance three or more pointers.

**Later in the morning.** threshold.c's `AdaptiveThresholdImage`: Recipe K on its per-pixel
channel loop, whose accumulators are the row's own `channel_bias` and `channel_sum` arrays
(`7c51b9302`, **threshold.c 2.38 → 2.65**, its Brain Method gone; 12 cases, 0 diverged). The same
move on `SelectiveBlurImage` measured 1.59 → 1.60 with the Brain Method moving into the
helper, and K on its two window sums after it stayed at 1.60: not committed. color.c's
`IsEquivalentImage` and image.c's `InterpretImageFilename` were read and left: their
blocks write two or more outer variables. Also measured and left: `BilateralBlurImage`'s per-pixel channel loop
(effect.c 1.59 → 1.51, a second Brain Method in the helper). `MeanShiftImage` was read and
left: its block assigns `status` from a call, which neither T nor J allows.

Whole catalogue at `7c51b9302`: 15,906 cases, 1 diverged (`gaps/c5651ffad8`), as at `911987a09`. Earlier, at
`f8fc17c71`, 3 diverged: `gaps/c5651ffad8` (the known
artifact) and the two hough matrix-on-disk cases, which timed out under load and passed
when rerun alone.

### Midday (2026-10-28): splits beyond the approved list

The owner answered the three remaining questions ("of course" to each): statistic.c may
be split, more splits are wanted although each new file starts red or yellow and lowers
the per-file mean, and a split commits past the pre-commit gate's new-file rule
(`eb4ed9640`). A scan dry-ran every clean family of 150 lines or more in each red file
and scored both sides; the splits made are those that raise the original by about 0.25
or more:

| Commit | Family moved | Original | New file |
|---|---|---|---|
| `154982200` | `StatisticImage`, PixelList types, 14 statics | statistic.c 2.32 → 2.61 | statistic-image.c 6.56 |
| `9b578080f` | `GetImageFeatures`, `FeatureStatistics`, 5 statics | feature.c 2.30 → 3.90 | feature-texture.c 5.18 |
| `5ed3581d2` | `DeskewImage` and its statics | shear.c 2.72 → 4.68 | shear-deskew.c 6.80 |
| `a6646ddf1` | `GetGeometry`, `ParseGeometry` and their statics | geometry.c 3.45 → 6.90 | geometry-parse.c 4.49 |
| `f77ec9ad8` | `ScaleImage` and its statics | resize.c 3.55 → 4.52 | resize-scale.c 6.66 |
| `bc7ea9777` | `GetImageDepth`, `SetImageDepth` and their statics | attribute.c 2.97 → 3.87 | attribute-depth.c 6.37 |
| `4ef25efe7` | `GetImageMoments` | statistic.c 2.61 → 2.97 | statistic-moments.c 8.00 |
| `94444f901` | `AutoThresholdImage` and its statics | threshold.c 2.65 → 3.05 | threshold-auto.c 7.42 |
| `1e1ccff65` | `MergeImageLayers` | layer.c 3.94 → 4.27 | layer-merge.c 9.08 |
| `a6fb42c3f` | `CropImageToTiles` and its statics | transform.c 2.71 → 2.97 | transform-tiles.c 8.17 |

Each commit records the moved statics, the combined guards (literals and call sites
unchanged over the pair), the exported symbol and the oracle on every case reaching the
moved public functions, 0 diverged (`GetGeometry` 12,952 and `ParseGeometry` 14,721
cases; `SetImageDepth` 10,840). Families were paired only where they belong together
(the two geometry parsers, the depth getter and setter): pairing raises the original
more but leaves the new file lower.

Tried and left:
- color.c's `QueryColorCompliance` (3.55 → 4.35 dry): it calls `GetColorCompliance`,
  which is `MagickExport` but declared in no header, so the new file would not compile
  without a new declaration. `tools/split_source.py` does not detect this; the build did.
- fourier.c's `ComplexImages` (3.27 → 3.65 dry): the include block carries the FFTW
  macro definitions (`cabs`, `carg`, `cimag`, `creal`), which the copied header would
  define in both files; the literal guard flagged them.
- threshold.c's `OrderedDitherImage` (3.05 → 3.43 dry): it uses `struct _ThresholdMap`,
  which threshold.c defines and keeps for `GetThresholdMap` and its readers.

The three left were accepted by `tools/split_source.py` and failed at the build or the
guard; the tool now refuses each kind with its reason (`2c8c839f6`).

**Second round.** A rescan of the changed files found families that now raise their file
by 0.2 or more:

| Commit | Family moved | Original | New file |
|---|---|---|---|
| `b7df7b86a` | `IntegralRotateImage` | shear.c 4.68 → 6.46 | shear-rotate.c 7.05 |
| `d112476b0` | `GetImageBoundingBox`, `CensusInfo` and its statics | attribute.c 3.87 → 5.19 | attribute-bounds.c 7.51 |
| `a1bb1314f` | `GetImageMinimumBoundingBox`, `CaliperInfo` and its statics | attribute.c 5.19 → 5.64 | attribute-hull.c 8.49 |
| `59ba5fb1a` | `FunctionImage` and its statics | statistic.c 2.97 → 3.23 | statistic-function.c 8.70 |
| `a2413bce4` | `PosterizeImage` and its statics | quantize.c 2.36 → 2.58 | quantize-posterize.c 8.18 |
| `3055af531` | `InterpretImageFilename` and its statics | image.c 2.49 → 2.71 | image-filename.c 7.32 |
| `7503478c6` | `SampleImage` and its statics | resize.c 4.52 → 4.78 | resize-sample.c 9.10 |
| `caa3018a8` | `ParseMetaGeometry` and its statics | geometry.c 6.90 → 7.40 | geometry-meta.c 8.73 |
| `2d052c5c1` | `AdaptiveThresholdImage` and its statics | threshold.c 3.05 → 3.28 | threshold-adaptive.c 8.76 |

Oracle on each family's cases, 0 diverged (`InterpretImageFilename` 12,656 cases,
`ParseMetaGeometry` 1,587). One batch failed for a build reason, not a code one: a step
reset by `git reset --hard` restored threshold.c within the same second its object had
been built from the split version, so make kept the stale object and the next five
builds failed to link `OrderedDitherImage`. Touching the restored file fixed it.

A third scan of the same files finds no family above 0.2 except in files left above
(image.c's are 0.06 at most). The families that split cleanly but move their file by
less than 0.2 (enhance.c's ten, effect.c's six, image.c's five, the X11 files) were not
made.

**Yellow files.** The same scan over the 47 MagickCore files scoring 4 to 9.5 found six
with a family worth 0.2 or more:

| Commit | Family moved | Original | New file |
|---|---|---|---|
| `f31d03fa6` | `SetImageAlphaChannel` and its statics | channel.c 4.04 → 5.13 | channel-alpha.c 6.96 |
| `c629d08d0` | `CombineImages` | channel.c 5.13 → 6.93 | channel-combine.c 8.22 |
| `e4193e021` | `RaiseImage` and its static | decorate.c 6.74 → 7.73 | decorate-raise.c 8.67 |
| `aee929162` | `GlobExpression` and its statics | token.c 5.02 → 6.49 | token-glob.c 7.59 |
| `5d2502f8e` | `GetNextToken` and its statics | token.c 6.49 → 7.82 | token-next.c 7.99 |
| `f697c21ca` | `GetConfigurePaths` | configure.c 6.19 → 6.47 | configure-paths.c 8.95 |

`GetConfigurePaths` and `GetNextToken` reach 15,903 cases and `GlobExpression` 228; in each
the only divergence was `gaps/c5651ffad8`, the known artifact, which diverges on the
unchanged tree too (rerun at `e4193e021`). Left: resource.c's `SetMagickResourceLimit`
(5.02 → 5.94 dry), which uses the file-scope `resource_info` that resource.c keeps (the
tool now refuses such a family), and module.c's `InvokeDynamicImageFilter` (6.46 → 6.94
dry), which no case reaches.

### Sweep (2026-10-28, 00:20): where the night ends

MagickCore against the morning's sweep: **mean 4.86 → 4.97; red 46 → 44, yellow 38 → 40,
optimal 3 → 4**; 31 files up, none down. Largest moves: splay-tree.c 4.91 → 5.94, paint.c
3.33 → 4.14, version.c 9.28 → 10.00, matrix.c 6.76 → 7.45, pixel.c 1.10 → 1.73, shear.c
2.18 → 2.72, policy.c 3.85 → 4.39, quantum-import.c 1.41 → 1.88, quantum-export.c 1.57 →
2.04, annotate.c 2.33 → 2.77. Brain Methods in the built MagickCore files: 44 → about 25.
Whole catalogue at `a055db34e`: 15,906 cases, 1 diverged (`gaps/c5651ffad8`, the known
Mac artifact).

**What is left needs a decision or a new recipe, not more of the same.**
- pixel.c's five fixed-map extractions: one commit for all five (1.73 → 1.80), the open
  question carried over from #54.
- Recipe S (split file) for the largest files, which needs the owner's go-ahead per file.
- Blocks inside parallel loops that write the shared `status` together with private
  accumulators (effect.c's blur family, MeanShiftImage, vision.c's moments, StatisticImage):
  a Recipe T and K combination would cover them, but each such block reads eight or more
  values of row context, so the helpers would carry Excess Arguments.
- Duplicated per-format blocks (quantum-import.c, enhance.c's row helpers, the
  TransparentPaint pair) differ in an expression, not a value, so Recipe D does not reach
  them, and merging them left the score flat where tried.

### Sweep (2026-10-27, 22:35)

MagickCore with `cs-mcp` 1.5.13, against the morning's sweep at `dbeddd910` (before
M100's pixel.c and draw.c steps, which this table includes): **mean 4.86 → 4.95; red 46
→ 44 files, yellow 38 → 40, optimal 3 → 4** (version.c). 27 files moved, none down:
paint.c 3.33 → 4.14 and policy.c 3.85 → 4.39 leave the red band; matrix.c 6.76 → 7.45;
pixel.c 1.10 → 1.73; shear.c 2.18 → 2.72; annotate.c 2.33 → 2.77; color.c 3.16 → 3.55;
fx.c 1.93 → 2.28; quantum-import.c 1.41 → 1.73; quantum-export.c 1.57 → 1.87; image.c 2.20 →
2.49; feature.c 1.94 → 2.21; visual-effects.c 2.41 → 2.63; distort.c 2.29 → 2.49;
constitute.c 4.45 → 4.65; effect.c 1.40 → 1.59; configure.c 6.01 → 6.19; morphology.c 2.04 →
2.20; draw.c 1.79 → 1.90; composite.c 1.92 → 2.02; threshold.c 2.08 → 2.16; signature.c
8.54 → 8.62; transform.c 2.65 → 2.71; quantize.c 2.09 → 2.14; geometry.c 3.40 → 3.44;
delegate.c 2.60 → 2.61.

Later in the night also: composite.c CopySourceChannels and SetVirtualCompositeChannels
(CompositeImage, E ×2, 1.92 → 2.02), geometry.c ParseGeometryXOffset (GetGeometry, O; the
matching y helper was refused: CodeScene counts the pair as duplication, 3.44 → 3.36),
delegate.c WaitForDelegateInput (InvokeDelegate, E, 2.60 → 2.61).

### Late night: the quantum entry points

| File | Step | Recipe | Evidence | Code Health |
|---|---|---|---|---|
| quantum-import.c | DisassociateQuantumAlpha (ImportQuantumPixels) | E | 13,937 cases (the commit says 13,921, the case count before tonight's new cases); Brain Method, Deep Nesting and Bumpy Road gone | 1.73 → 1.88 |
| quantum-export.c | AssociateQuantumAlpha (ExportQuantumPixels) | E | 12,782 cases; Brain Method and Deep Nesting gone | 1.87 → 2.03 |
| quantum-export.c | SwapRedGreenQuantum, the two identical CbYCr swaps | D | 12,782 cases; Bumpy Road gone | 2.03 → 2.04 |
| delegate.c | WaitForDelegateInput (InvokeDelegate) | E | 12 cases | 2.60 → 2.61 |

| quantize.c | FindNearestKmeansColor, then AccumulateKmeansPixel (KmeansImage) | O (amended), E | 22 cases; flat, then Brain Method and Deep Nesting gone | 2.14 → 2.36 |
| feature.c | GetCannyNeighbors (CannyEdgeImage's suppression) | O (amended) | 12 cases | 2.21 → 2.23 |

| threshold.c | RangeThresholdChannels (RangeThresholdImage's per-pixel work around the earlier ladder helper) | E | 13 cases; Brain Method gone | 2.16 → 2.38 |
| feature.c | WriteHoughHeader, VoteHoughLines (HoughLineImage) | E (2 commits) | 19 cases | 2.23 → 2.30 |
| splay-tree.c | CompareNodeKey reused for the root-key comparison in six functions (M53's leftover, which waited for the oracle level) | D (6 commits) | 12 to 15,549 cases each | 4.91 → 5.12 |
| splay-tree.c | RelinquishNodeContents (value and key release; three copies in RelinquishSplayTreeNodes, then AddValueToSplayTree, DeleteNodeByValueFromSplayTree, DeleteNodeFromSplayTree) | E, D (4 commits) | 12 to 15,549 cases each | 5.12 → 5.94 |
| stream.c | AcquireStreamRegion, the buffer sizing that ends GetVirtualPixelStream and QueueAuthenticPixelsStream | E (step 7), D | 13 and 306 cases | 3.10 → 3.13 |
| geometry.c | SetPageGeometryDefaults (ParseGravityGeometry) | E | 579 cases | 3.44 → 3.45 |
| layer.c | FindDifferingPixel, the four edge scans of CompareImagesBounds | E | 67 cases | 3.79 → 3.94 |
| vision.c | CountPerimeterPattern, the 2x2 pattern count PerimeterThreshold and CircularityThreshold share | D | 4 cases; Deep Nesting gone in both | 2.10 → 2.35 |

Whole catalogue at `f08adb1b0`: 15,906 cases, 1 diverged (`gaps/c5651ffad8`, the known Mac artifact). Brain Methods in MagickCore (built files): 27 left, from 44 in the morning.

Found with a scan for identical runs of four or five lines within a file (the repeated blocks behind Code Duplication); the splay-tree, stream.c and layer.c steps came from it. Flat and reverted: quantum-import.c's half-float RGB triple across nine functions; geometry.c's reuse of SetPageGeometryDefaults in ParsePageGeometry (the commit `ae29733d9` names that reuse as its next step; it was tried and left flat, so not kept).

**Recipe O amendment (`bf8fe8636`):** inside a parallel loop body, a selection may move
when every variable it writes is declared in that body, it writes nothing declared
outside it (`status` included) and it has no pragma, which is Recipe K's condition.
KuwaharaImage's quadrant variance and offset (flat together), AdaptiveBlurImage's accumulation (eight arguments) and the vision.c moments loops (they write the shared `status` with up to six accumulators) were left. DrawGradientImage's repeat offset was tried under it and left flat: its depth-6 nesting
also sits in each spread's stop lookup.

Reverted: DistortImage's -verbose printers (BilinearForward flat, Polynomial 2.49 → 2.47);
the import side's single CbYCr swap (flat).

### Open: an intermittent divergence on write-mask cases

`resize/6ee45e9cfa` and `resize/6efddf0c0a` (rose under a write mask, `-scale`, written
as 32-bit floating-point MIFF) diverged three times today in full oracle runs: during the
ImportIndexQuantum and ImportGrayQuantum steps and once at 21:08, when the case diverged
in 4 of 5 runs on its own, with a peak error of 3.4e38 (FLT_MAX, a value no pixel had).
Minutes later it diverged in none of: 16 runs against the baseline, 16 of the candidate
against itself and 16 of the baseline against itself under a load average of 27, nor in
full uncached runs of all 13,088 WriteImage cases (baseline against itself, and the
candidate against the baseline). The baseline never diverged in any run. The pattern
fits a read of uninitialised memory whose content depends on the run's surroundings, but
it could not be reproduced to bisect. If it reappears, keep the candidate's output
(`build-oracle/work/runs/cand/`) and bisect quantum-import.c and quantum-export.c first:
the case reads and writes MIFF through both, and both changed today.

**It is in the unchanged code (2026-10-28, midday).** The whole catalogue at `4acdcfc46`
had `resize/6ee45e9cfa` diverge again, and alone it diverged in 3 of 10 uncached runs. A
candidate built from the baseline's own commit, `de54d27df`, diverged from the baseline
binary in 11 of 20 runs; `a6646ddf1` in 0 of 20 and `f77ec9ad8` (`ScaleImage` moved) in 6
of 20. The same source differs from itself between builds and runs, so the refactoring
does not cause it: the case reads something the code does not set, as suspected above.
Where is still open; a divergence on this case alone is not evidence against a step.

### ERDC (night)

The fresh reports after profile.c are unusable: transform, quantize, montage and cache
show 0 survivors with most kills "exit status 0 → 1, out.miff missing", and constitute
is partly affected; the earlier files look normal. The run was stopped. Those five
reports are quarantined on the Mac (`build-oracle/work/quarantine/`); nothing used them.
With the owner's decision above, night3 is not restarted.

**Recipe F - Hoist Function-Local Static Data (new).** A `static` local whose
initialiser is data and which nothing writes moves to file scope above its function.
Proof: `tools/obj_same_code.sh` (new), which compares the machine code and the data as
sets, since the compiler reorders the string pool when a literal moves (byte-identical
objects, Recipe L's proof, fail on that alone). It printed SAME CODE for the steps below
and DIFFERENT for a trial with one delegate name changed.

| File | Function | Data moved | Evidence | Code Health |
|---|---|---|---|---|
| version.c | GetMagickDelegates | the `#if`-assembled delegates string (124 lines) | oracle (9 cases), SAME CODE | 9.28 → **10.00** |
| signature.c | TransformSignature | the 64 SHA-256 round constants | oracle (1,867 cases), SAME CODE | 8.54 → 8.62 |

Tried and reverted (score flat): property.c GetEXIFProperty's tag table (304 lines; the
function stays Large at about 400 lines; the tool also flagged 4 bytes of alignment
padding) and colorspace.c TransformsRGBImage's `YCCMap` (236 lines, the function stays
at about 700). F pays only where the data is most of the function.

**Recipe B amendment: member accesses and casts.** Step 4 refused any initialiser that
reads a member (`shear.x`, `image->rows`) or casts, which left shear.c and transform.c
out of reach though every one of their argument findings is on a static function. Such
an initialiser is now allowed in a struct declared directly before the one call that
uses it, with no statement between: nothing can run between the two, so the struct holds
exactly what the call would have read, and the expressions have no side effect, so
reading them first changes nothing. The calls that need it each get their own struct
(`first_pass`, `second_pass`, `third_pass`, `crop_region`, `first_region` to
`fourth_region`).

**New cases: ShearRotateImage.** No case reached ShearRotateImage, where three of the
changed calls are: `-rotate` is RotateImage, a distortion, and nothing in the library
calls it. A new `imdriver` command (`shearrotate IMAGE DEGREES OUT`) and sixteen cases
(eight angles across the quadrants, 45 and 180 degrees, negative angles; rose and
rose_alpha) now reach it. Commit `e3c0292c2`, before the shear steps; the case map was
rebuilt (15,900 cases).

**The 200-case cap, not the harness, made these functions "not ready".** M99's capped
runs gave the six quantum functions 18-51% adjusted and GenerateCoefficients 74%. The
uncapped `q100` run (`--max-cases 4000`) puts every one at careful or ready. A mutant
that survives only because the cases that would kill it lie beyond the cap is a
*capped* survivor, which the gate counts against the function. When a capped run says
"not ready" for a function reached by more than 200 cases, rerun it uncapped before
recording it as weakly protected.

MorphologyPrimitive's one call is in MorphologyApply's kernel loop, after an `if`
chain that sets `primitive` and `this_kernel`; the struct is declared just before the
call. MorphologyApply keeps Brain Method and seven arguments: it is `MagickPrivate`,
called from other files. GenerateCoefficients keeps its `DistortMethod *method`
out-pointer as a parameter (step 4 refuses `&method`); its two callers each declare
their own `CoefficientInput`. GetGeometry, PolaroidImage and GetImageDynamicThreshold
carry both findings too, but are public API.

**Load-induced divergences.** While `q100` ran, two full oracle runs each had one
`write-mask -scale` case diverge (`resize/6efddf0c0a`, `resize/6ee45e9cfa`, output
differs); alone they passed 3/3 and 5/5, on both the changed and unchanged code. A
third run had the *baseline* of two `compare -subimage-search` cases time out, and the
cache kept those timeouts, so they diverged on unchanged code until rerun with
`--no-cache`. Each affected step was reverted by the script and redone on a quiet
machine; every commit cites a clean run.

Full catalogue at `e77a54c39`: 15,884 cases, 1 diverged, gaps/c5651ffad8 (the known Mac
artifact).

### Harness notes

- **ERDC had not pushed since 2026-10-12.** `night3.sh` died silently on 2026-10-26 in
  morphology.c, most likely by the container's 64 GB limit with 28 jobs. Restarted on
  2026-10-27 with 16 jobs and a 4 GB per-process cap. Set for the whole script, the
  cap made the Mull build fail (`posix_spawn failed: Cannot allocate memory`), so it
  now wraps only the mutation step. The first five fresh reports (morphology,
  geometry, enhance, visual-effects, segment) arrived at 17:39; MorphologyPrimitive
  above uses them.

### Open question for the owner (carried over from #54)

pixel.c stops at 1.73: each of the five remaining fixed-map Recipe E extractions,
taken alone, dips the file to 1.71 (*Lines of Code in a Single File* threshold), and the
pre-commit hook refuses it; all five together reach 1.80. May they go in as one
commit? See [M100](M100-pixel-buffer.md#where-pixelc-stops-at-173).
