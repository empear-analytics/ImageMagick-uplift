# W15 lane a: the next Campaign A files

Campaign A, branch `campaign/W15` (after the composite.c and morphology.c work, 903bc5749),
2026-11-07. Harness: Claude Code (Claude Opus 5.5), WSL2 Ubuntu. Code Health from `cs-mcp`
1.5.16 through `tools/ch.py` (the committed baseline records 1.5.12; every before and after
score below is from 1.5.16, and the fresh sweep `codehealth_sweep.py --only MagickCore/` at
903bc5749 reproduced the playbook's recorded scores for these files).

**Result: 54 refactor commits in nine files, no playbook text.** Every commit passed the
build, `tools/ci_warnings.sh`, `refactor_guard.py` (literals `OK`; `--calls` moving only the
new helper, `+2`, or `+3` for a predicate at two sites), the oracle on the cases that reach the
function and the pre-commit CodeScene gate. Whole catalogue at the end: 15,900 cases, 0
diverged (run twice: after visual-effects.c and after quantize.c).

| File | Before | After | Commits |
| --- | ---: | ---: | ---: |
| `shear.c` | 8.61 | 9.17 | 5 |
| `effect-blur.c` | 7.73 | 8.22 | 1 |
| `layer.c` | 7.10 | 7.55 | 2 |
| `resample.c` | 8.01 | 8.53 | 4 |
| `resize-image.c` | 7.82 | 8.32 | 5 |
| `channel.c` | 8.06 | 8.58 | 7 |
| `visual-effects.c` | 8.54 | 9.09 | 16 |
| `transform.c` | 7.39 | 7.85 | 13 |
| `quantize.c` | 7.38 | 7.80 | 1 (a rule 1 set of 12) |
| `feature-texture.c` | 5.25 | 5.25 | 0 (measured) |

## The lever: Overall Code Complexity

None of the lowest files had a function-level step left that the recorded plateaus and
today's rulings did not already rule out (feature-texture.c below; enhance-tone.c and
colorspace.c were taken by lane b, distort.c is lane b's). What was left in many mid-table
files is the file-level finding *Overall Code Complexity*, CodeScene's flag on the mean
complexity of a file's functions. A probe adding n trivial cc-1 functions to a scratch copy
says how many new functions clear it and what that is worth: effect-blur.c 1 (+0.49),
layer.c 2 (+0.45), resample.c 4 (+0.52), resize-image.c 5 (+0.50), shear.c 5 (+0.56),
channel.c 6 (+0.51), transform.c 13 to 16 (+0.46), visual-effects.c 16 (+0.55),
quantize.c 12 (+0.23, with the function-count finding), visual-effects-plasma.c 4 (+0.55),
paint-floodfill.c 8 (+0.53), attribute-depth.c 14 (+0.48), fourier.c 20 (+0.39). Any
extraction moves a block's decisions into a function of its own and adds one function, so it
counts like a cc-1 function; a predicate (P) counts for less, since its `?:` is counted too.

For each file the chain was built in full on a scratch copy first (real code, not stubs) and
committed only when the whole chain cleared the finding with no new finding and no *Code
Duplication*. The steps before the last are flat; each is kept under rule 2 as a step of the
measured plan against *Overall Code Complexity*, and its message names the plan and the next
step (the W14 threshold.c precedent). Blocks were chosen for being self-contained (a comment-
delimited block, a computed geometry, a tail, a set-up) and different from each other;
candidates that the scratch copies showed as clones were dropped, not reshaped:
`SetTransposePage` (twin of `SetTransversePage`, 6.36), quantize.c's undithered row loop
(Code Duplication alone, 6.54), the Colorize/Tint parse twins, any second X/Y shear helper.

## Steps per file

**shear.c 8.61 -> 9.17.** a34e42644 E `GetShearAngles` (ShearImage); 072f95ef0 E
`GetCropGeometry` (CropToFitImage); 5879cd161 N pass-through `GetShearedExtent`
(CropToFitImage, binds the five `CropToFitRegion` members); 61bb8e8e5 E struct form
`ComputeShearBounds` (ShearIntegralImage; `bounds.height` is never set, so the struct goes by
pointer); 819497399 O `SelectShearDirection` (XShearLine, inside the parallel row loop:
both written variables are XShearLine's locals). Left: `YShearImage` (cc 14, 124 lines, 2
bumps; stubbed 9.92), clone-locked with X.

**effect-blur.c 7.73 -> 8.22.** 9524ddb71 E `RelinquishMotionTables` (MotionBlurImage's final
kernel/offset release). Left: Motion and AdaptiveBlur (stubbed together 9.09), clone-locked.

**layer.c 7.10 -> 7.55.** 8a4b5b975 E `GetCoalesceBounds` (CoalesceImages); 746b49876 E
`ClearTransparencyCanvas` (OptimizeImageTransparency). Left as recorded.

**resample.c 8.01 -> 8.53.** 5ae9cbf6e E `InitializeResampleFilter`; 8e2f247d9 N pass-through
`SetResampleEllipseBounds` (FitResampleEllipse); 3e692520e E step 7 `NormalizeResampledColor`;
fa0dae3ed E step 7 `FinishCheckerTileBlend`. All compiled code; nothing in the
`FILTER_LUT`/`DEBUG_*`/`EWA` branches moved.

**resize-image.c 7.82 -> 8.32.** 9d9cf875c E step 7 `AcquireContributionRows`; 0f1a13ce1 E
`RelinquishContributionRows`; 659aefc38 N pass-through `CloneFilterImage`
(ApplyResizeFilterPass); 6f53865e3 E `SelectDefaultResizeFilter`; 6e942cf89 P
`IsFilterRowMissing` at two sites (VerticalFilter, FilterHorizontalColumn).

**channel.c 8.06 -> 8.58.** 295e64366 E `SeparateRowPixels`; 34ff759d1 E
`SetSeparateImageAttributes`; 21a9cf668 E step 7 `FinishSeparateImage`; ef35fec15 E
`AddChannelMaskOption` (ChannelFxImage); 983d68d7f P `IsChannelPairUndefined`; a2f4f4c64 P
`IsChannelRowMissing` at two sites; 3fab0d685 P `IsChannelNotUpdated`. Left: `ChannelFxImage`
(a stub gives 10.0): its exits return `DestroyImageList(...)` through a variable, which I, V
and W (one constant) refuse.

**visual-effects.c 8.54 -> 9.09.** Sixteen E steps, 605782628 to b926ae469:
`ToneCharcoalImage`; `GetPolaroidQuantum`, `MountPolaroidPicture`; `PrepareShadowClone`,
`GetShadowBorder`, `ClearShadowBorderColor`; `ToneSketchDodge`, `BlendSketchOverlay`;
`ClearVignetteOval`, `SetVignetteColors`, `SetVignettePrimitive`, `CompositeVignetteMask`,
`FlattenVignetteImage`; `SolarizePseudoClassImage`, `ConformSolarizeColorspace`;
`TiltPolaroidImage`. Two Recipe D candidates (BlueShift's and Charcoal's tails equal
`ReleaseFailedImage`'s body) were measured and left out: D's copies-without-a-finding form
is kept only when the score rises, and in this chain they would have been flat.

**transform.c 7.39 -> 7.85.** Thirteen steps, e1baa7b18 to b9b15dd28: `GetShaveGeometry`,
`ShaveImagePage`, `CompositeExtentImage` (E step 7), `SetRollOffset` (struct form: the
offset's width and height are never set), `FinishRollImage`, `ResizeTransformImage`,
`OffsetTrimByPage`, `ResetImageOrientation`, `ApplyTrimArtifact`, `GetChopExtent`,
`SelectTransformImage`, `CropTrimImage`, and P `IsShaveTooLarge`.

**quantize.c 7.38 -> 7.80.** ccd27cabd, one rule 1 set of twelve Recipe E steps in seven
functions (GetQCubeInfo 2, QuantizeImages 1, QuantizeImage 3, DitherImage 3, RemapImages 1,
SetGrayscaleImage 2), under the owner's ruling that the 110-function limit may be passed
when the chain lifts the file: the 111th function switches on *Number of Functions in a
Single Module*, so each step alone scores 7.16 (one 7.34) and only the set raises the file.
The commit lists every block with its score alone. 110 -> 122 functions.

## feature-texture.c (5.25): measured, no commit

The plateau row's legal-only chain was built for real on a scratch copy
(`build-oracle/scratch/w15a/ft_full.c`, assembled from the original's line ranges by script
so that every moved block is the original text): one `TextureFeatureInputs` struct carrying
the five parallel loops' inputs and the zeroed statistics by value (Recipe N step 2's
read-then-written members), N pass-through helpers for the loops and their leaves, and the
non-parallel set-up (grays, co-occurrence matrix, normalisation). It scores **5.07**, below
5.25; adding the clean-up exits and a split of the contrast loop (stubs) reaches at most 5.52.
What binds:

- The per-direction finals (correlation, variance, difference variance, the measures of
  correlation) read slot `i` of statistics that the other iterations of the same parallel
  loop write. A helper taking those structs by value copies all four slots while other
  threads write them (a data race the single-threaded oracle cannot see); by address needs
  the pending ruling on per-iteration slots. So the finals stay in their loops, and those
  loops keep nesting 4 and Large Method (`ComputeFeatureVariances` cc 35, 231 lines).
- A helper holding a whole loop must bind `image` and `number_grays` for the `omp` clause;
  where its compiled body no longer reads them (the sum-average loop after its leaf moves) the
  non-OpenMP build warns of an unused variable, so that leaf has to stay.
- The contrast loop's leaves need five or six parameters; the only legal form fills a new
  struct per inner iteration of an O(n^3) loop.
- The file has six functions; the seventh switches *Overall Code Complexity* on, which
  `GetImageFeatures` (cc 151) keeps on until the very end of any chain (50 cc-1 functions
  do not clear it).

## Upstream behaviour noticed, not changed

- `AssignImageColors` discards both the dithering status (`(void) DitherImage(...)`) and
  the undithered row loop's status; a failed row or dither is not reported to the caller.
- `BlendSketchImage` (now `ToneSketchDodge`) ignores the status of its clamp, normalise,
  negate and `TransformImage` chain: a failure there still composites the dodge image.
- feature-texture.c (as W13/W14 noted): the Q green term divides by
  `density_y[x].direction[i].red`; `density_xy` is not cleared between the sum-average and
  difference passes; `p+=...;;`.

## Owner decisions and open questions

1. **The Overall Code Complexity lever.** Fifteen of campaign A's files carried the finding
   (compare.c, composite.c, fx.c and morphology.c carry it too, under functions too large
   for it to move) after their functions had been worked down; clearing it pays about +0.5
   a file, but takes from one to sixteen small extractions whose only measured effect is
   the file mean. This lane used it on nine files (all flat steps under rule 2, plans
   measured end to end). If the owner considers this metric-shaped rather than code-shaped,
   the commits are independent per file and revertible file by file. Not done:
   visual-effects-plasma.c (two natural cc-1 blocks short), paint-floodfill.c
   (macro-bound), attribute-depth.c and fourier.c (compiled code too thin), and lane b's
   colorspace.c and enhance-tone.c (left to avoid merge conflicts).
2. **feature-texture.c** needs either the per-iteration slot ruling (W14 lane b's question)
   or a ruling that a parallel-loop helper may read the iteration's slot of a shared
   statistics struct through a pointer to `const`.
3. Lane b's branch (`campaign/W15-b`) also changed `enhance-tone.c` and `colorspace.c`; this
   branch did not touch them.

## New files

None. No Recipe S split was made (the split sweep over every public root of the A files found
only splits that leave the original flat, or separate clone pairs, or fall outside Recipe S's
*Use when*: enhance.c's `AutoLevelImage`/`NormalizeImage`, attribute-depth.c,
visual-effects-stegano.c). Nothing to register for the Windows build.

## Performance

Median of 7 (some 9) runs, 1200x1200 noisy gradient, oracle builds (no OpenMP), base
9db184dd40 against the branch head: resize 250% 0.62 s both (a first run showed +4%,
three reruns 0 to +0.5%), resize 30%, distort SRT 30, rotate, shear, separate, vignette,
shadow, charcoal, `-colors 64`, `+dither -colors 32`, Riemersma dither, trim, roll, extent,
shave all within +-1% on the reruns (one first run showed shear 8% faster: noise).

Oracle: whole catalogue on the branch head, 15,900 cases, 0 diverged (baseline 9db184dd40).
