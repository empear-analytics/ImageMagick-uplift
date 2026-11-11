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
