# W16 - campaign A, parallel lanes (2026-11-10)

Campaign A (Windows desktop, WSL2), branch `campaign/W16` cut from `main` 72f10d28d after #91. Four lanes in git
worktrees, as in W14. The owner's rulings of 2026-11-07 apply (a file may pass 110 functions when the chain lifts it;
label-run helpers may be Complex Method; Recipe S splits the tool accepts when the new file scores at least the
original; no new OpenMP exceptions). Start: campaign A mean 8.19, 27 of 74 files at 10.0.

## Lane d, first batch
- `enhance-level-colors.c` 9.36 -> 9.68 (e226d3e2d, cherry-picked).
- **Parked for the owner, not in the PR:** two `shear.c` commits that take it 9.17 -> 10.0 (local branch
  `w16-shear-enum`). The first (c74c70fd5) folds Recipes PC, N and a D reuse into one commit and replaces
  `YShearImage`'s local `UP`/`DOWN` enum and switch with the file's existing `ShearDirection` and an `if`
  (LEFT = UP, RIGHT = DOWN); behaviour is the same (43 cases, 0 diverged) but removing the enum is a rewrite no
  recipe covers. Merge it if you accept that.
- No step on `vision.c`, `paint.c`, `statistic.c`, `effect.c`, `feature.c`, `resize.c`, `visual-effects.c`: their
  remaining findings are public argument counts and file length (or need B on an unreached function).

## Lane b: channel.c 8.58 -> 9.28, effect-blur.c 8.22 -> 8.68, resize-image.c 8.32 -> 8.78 (Opus, 8 commits)

One temporary drop: fcf5f64f4 (`resize-image.c` 8.32 -> 8.25, `--no-verify`, step 1 of a measured plan; the next step
reaches 8.78). Upstream, not changed: `ChannelFxImage` leaks the image list when `AppendChannelCanvas` fails;
`SharpenAdaptiveRows` releases its kernels with an inline copy of `RelinquishAdaptiveKernelSet`'s body.

## Lane d, second batch: threshold.c, resample.c, colorspace-convert.c at plateau (Sonnet, no commits)

## Lane a: enhance-tone.c 5.99 -> 6.99, fourier.c 6.33 -> 6.45 (Opus, 11 commits)

**For the owner:** 6f54268cc uses Recipe N for a tail that reads only three caller variables (N's *Use when* asks
for five or more), because whole-parallel-loop tails go to N or Z, not E; should E admit whole loops under N step 1's
conditions? dfad29877 and e7f09ffbc use I step 2's const-addresses form for three variables. **Upstream, likely a
bug, not changed:** `EqualizeImage`'s `memset(black,0,sizeof(*black))` and `memset(white,...)` clear one double
each, not the whole arrays.

## Second looks under the 2026-11-07 rulings: composite.c, morphology.c, compare.c unchanged (Opus, no commits)

All three stay where they were (3.46, 3.04, 2.97); see their plateau rows. **Owner options that would move them:**
merging `CompositeCopyImage`/`CompositeIntensityImage` through a row-operation parameter; helpers writing OpenMP
reduction variables through pointers (`BlendRMSEResidual`); merging mirrored forward/reverse twins
(`morphology.c`). **Upstream, likely a real bug, not changed:** `BlendRMSEResidual` resets `channel_residual=0.0`
on every row inside its reduction loop, so only the last row's sum survives.
