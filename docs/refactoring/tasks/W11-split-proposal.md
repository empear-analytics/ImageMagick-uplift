# W11 - proposal: Recipe S split for the saturated red files

Recipe S needs the owner's go-ahead per file. This is the plan, measured by dry run
(`tools/split_source.py --out`, both halves scored with `tools/ch.py`, `cs-mcp` 1.5.16);
when written, nothing in `MagickCore/` or the build files had changed. **Done for
`effect.c` (commit 5408dde16) after the owner said "You can do this. Keep going."; the
measured numbers below held (4.10 and 4.91).**

## Recommendation: `effect.c`, one split, then maybe a second

`effect.c` (3.52, 5,915 lines, 77 functions) is the file where the stub probes found
nothing but a split: no single legal step, nor deleting a whole function, moves it.

**Split off the blur and convolution families into `MagickCore/effect-blur.c`:**
`AdaptiveBlurImage`, `AdaptiveSharpenImage`, `SelectiveBlurImage`, `MotionBlurImage`,
`RotationalBlurImage`, `BilateralBlurImage`, `SharpenImage`, `EmbossImage`, `BlurImage`,
`GaussianBlurImage`, `ConvolveImage`, with the statics and helper structs only they use.

| | Lines | Code Health |
| --- | ---: | ---: |
| `effect.c` today | 5,915 | 3.52 |
| `effect.c` after | 2,602 | **4.10** |
| `effect-blur.c` (new) | 3,416 | **4.91** |

Both halves score above the original, so the pre-commit gate (a split-off file must not
score below its original) passes without `--no-verify`. Both still carry *Lines of Code
in a Single File*; a second split would clear it on the kept half. Accepted by the tool
as independent families, to be scored: `KuwaharaImage` (346 lines), `LocalContrastImage`
(348), `PreviewImage` (566), `SpreadImage` (152). `DespeckleImage` and `ShadeImage` are
refused: `HullBuffers` and `ShadeImages` are structs near the top of the file that only
they use, which the tool does not move yet.

Why this cut: `MagickSigma` is used by the adaptive, selective, motion, sharpen and
emboss families and `BlurImageTag` by the others, and Recipe S refuses a file-level macro
on both sides; the group above is the smallest one that closes over both macros and
every shared static (`NormalizeKernelValues` ties `SharpenImage`, `EmbossImage` and
`ConvolveImage` together).

## The other two

- **`morphology.c` (2.71):** one family holds 3,906 of its 5,536 lines (77 functions,
  tied by shared statics: `AcquireKernelBuiltIn`, `AcquireKernelInfo`, `CloneKernelInfo`,
  `MorphologyApply`, `UnityAddKernelInfo`). The rest is `MorphologyImage` (121 lines) and
  five small kernel functions. A split moves almost nothing and does not separate the four
  big functions the stub probes need apart. Not recommended.
- **`enhance.c` (2.97):** 22 public functions in 21 independent families (the largest:
  `CLAHEImage`/`ContrastImage` 655 lines, `ModulateImage` 441). The first dry run
  (`ModulateImage`, `ColorDecisionListImage`, `GrayscaleImage`, `GammaImage`,
  `SigmoidalContrastImage`, `ClutImage`, `HaldClutImage`) was refused for the macro
  `Sigmoidal`, used on both sides. A group that closes over it is still to be found. It
  stays at 105 functions and a split would also lift the function-count cliff
  (*Number of Functions in a Single Module*) that blocked the extraction route.

## Tooling change made for this (commit 814828757, only `tools/split_source.py`)

The tool refused every family the campaign had extracted from: its helper structs and
`#define`s lie between the family's functions, and the tool accepted them only above the
first function. It now moves a typedef or `#define` inside a run with the region, and
refuses when the kept part names a moved type or a macro is used on both sides.

## If you approve `effect.c`

1. `python3 tools/split_source.py MagickCore/effect.c MagickCore/effect-blur.c
   "MagickCore Image Blur and Convolution Methods" <the eleven roots above>`.
2. `tools/register_source.py` in `MagickCore/Makefile.am`, `Makefile.in`, `Make.com` and
   `scripts/format_c_api_docs`. The Windows build configuration outside this repository
   still needs the file (the commit says so).
3. Verify: build, `ci_warnings`, `refactor_guard --combined` and `--combined --calls` over
   the pair, `pp_identical.sh`/`obj_identical.sh` where they apply, then the whole oracle
   catalogue. Commit in two steps (the move, then the registration).
4. One PR (draft), stacked on W11; the CodeScene PR gate will see a new file at 4.91.

## Other rulings that would open files

- `distort.c`: a recipe for `status=Helper(...)` in a parallel loop, and whether an `#if 0`
  debug block may move with its case (see `W11-day-session.md`): 3.01 to about 3.55.
- The function-pointer merge: `enhance.c` 3.04 to 3.27 (W08).
