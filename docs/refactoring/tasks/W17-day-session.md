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
