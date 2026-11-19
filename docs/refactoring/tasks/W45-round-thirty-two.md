# W45 round thirty-two (2026-11-19)

Campaign A, branch `campaign/W45` from main `46b61ab8d` (after #126). Three Opus lanes in batches, one Sonnet reviewer per batch; every batch merged only after its review. PR #129.

## Result

Campaign A sweep (cs-mcp 1.5.16, 150 `.c` files): mean 9.74 → 9.75, files at 10.0 88 → 89. Most of the round's work is in private headers, which the `.c` sweep does not list.

| File | Before | After | How |
| --- | --- | --- | --- |
| `accelerate-blur.c` | 9.34 | **10.0** | I, I, Y (stub-probed chain, one disclosed `--no-verify`) |
| `accelerate-motion-blur.c` | 8.46 | 9.30 | I, I, I, B, N, E, D (shared `DestroyAccelerateViews`, `TraceAccelerateImage` reuse) |
| `accelerate-histogram.c` | 8.38 | 8.75 | Y, I, Q, Y, I, I, D |
| `accelerate-resize.c` | 8.11 | 8.18 | I, I |
| `distort-map-private.h` | 9.68 | **10.0** | Recipe IP (new), 11 steps |
| `distort-private.h` | 9.09 | **10.0** | OH carve into `distort-linear-private.h` (9.68), then a 12-step BP/IP chain |
| `composite-image-coverage-private.h` | 9.38 | **10.0** | OH carves; `composite-image-alpha-private.h`, `-alpha-coverage-private.h` 10.0 |
| `composite-channel-private.h` | 5.65 | 6.17 | OH carve; `composite-channel-blend-private.h` 10.0 |
| `layer-frames-private.h` | 9.38 | **10.0** | OH carve; `layer-frames-bounds-private.h` 10.0 |
| `morphology-apply-private.h` | 9.68 | **10.0** | Recipe BP (new), 4 steps |
| `morphology-apply-kernel-private.h` | 9.68 | **10.0** | BP, IP |
| `compare-similarity-frequency-private.h` | 6.36 | 8.54 | 18 Recipe I step-8 steps; OH carve into `-frequency-images-private.h` (8.87) |
| `compare-similarity-private.h` | 8.37 | 8.63 | B on `GetSimilarityForMetric` |
| `compare-similarity-metric-private.h` | 7.27 | 7.50 | E, D |
| `compare-similarity-phase-private.h` | 7.99 | 8.01 | I |
| `compare-similarity-correlation-private.h` | 8.97 | 8.99 | FI |

No including `.c` file dropped at any step.

## Evidence

- Gates on `40721c8a1`: `ci_linux_warnings --campaign-a` OK (150 files); narrow oracle 15,966 cases, 0 diverged; wide 14,211 cases, 0 diverged (two xevents cases SETTLED by the harness as load-sensitive); `check_trailers` 81 refactor commits valid; `playbook_lint` 174 recipes, 0 problems. Baseline `46b61ab8d`.
- Speed (AGENTS.md 2026-11-18: time only suspect steps, Ir first). Region or per-pixel steps measured by callgrind Ir, OpenMP -O2 HDRI, 1 thread: `GetCorrelationTotals._omp_fn.0` −256; `GetPHASESimilarity._omp_fn.0` −0.78%; MAE/RMSE/MEPP/SSIM regions equal after the `GetSimilarityForMetric` B step; `RenderDistortImage._omp_fn.0` equal for `-distort Affine`, `Polynomial` and `Barrel` after the distort chain. No step raised a hot function's Ir, so no wall-clock run was needed.
- The oracle case maps were stale (2026-11-12, 2026-11-15) and were rebuilt for `46b61ab8d` (narrow 15,966 cases / 10,248 functions; wide 14,211 / 11,389).

## Taken out on review

- Three flat Recipe B steps (`statistic-image.c` `StatisticImagePixel`, `accelerate-point.c` `ComputeFunctionImage` and `ComputeModulateImage`): flat, no next step named (playbook rule 2). Left out by rebuilding the lane branch without them.
- A copied helper (`DestroyMotionBlurViews` equal to despeckle's `DestroyAccelerateViews`) was replaced by one `static inline` helper in `accelerate-core-private.h` (owner rulings of 2026-11-16 forbid copies between split products).

## New recipes and amendments (mandate, for owner confirmation)

- **IP** Pass the Input Struct in Place of Primitive Parameters Bound from It (Primitive Obsession); amended: a caller local reassigned only in blocks that return before the call, and a local bound through a const pointer local, with nothing writing the member in between.
- **BP** Bundle a Pointer and the Scalar It Travels With, for Primitive Obsession; amended for pointer groups (a string and its end) and for a call tested by an `if`.
- **OH Form A**: may carve an OH or split product; the new header's `#include` goes as the last line of the carved header's own include block, or, when it has none, in its one `.c` includer's block directly above it; Form A also takes Overall Code Complexity when both headers end at 10.0.
- **B** step 1 covers a `static inline` private-header function whose callers are all in its includers and change in one commit.

## What holds the rest

- Every remaining 9.68 `.c` file is held only by a MagickExport or MagickPrivate argument count (list in PR #129).
- `distort-linear-private.h` 9.68: the remaining Primitive Obsession sits in the per-pixel `PolyBasisFn*`/`poly_basis_fn`/`MagickRound` helpers; a carve leaves the new header at 8.54 (below 9.68, refused).
- `compare-similarity-*` headers: loop-body findings in SSIM, MEPP and PHASE; round 31 reverted the column-loop forms for speed; only Ir-equal forms remain open.
- `composite-channel-private.h` 6.17: `CompositeChannelColor` (per-pixel, cc 141).
- `accelerate-histogram.c`, `accelerate-resize.c`: owner questions (`#ifdef RECREATEBUFFER` blocks; a seven-value local-memory struct); `accelerate-despeckle.c` and `accelerate-point.c`: every probed step scores lower.
- `morphology-builtin-base-private.h` 9.36: `#if 0` labels (owner question).
- `fx-execute-private.h` 4.96, `fx.c`, `fourier.c`, `opencl-initialize.c`, display/widget/xwindow files: earlier owner questions.

## Owner questions

1. `#if 0` case labels in `IsBaseKernelNeeded` (9.36 → 10.0).
2. Recipe B over a function-pointer table (frequency header 8.08 → 8.33 probe).
3. Recipe D: `(Image *) NULL` versus `NULL` as the same token.
4. `reuse_check.py` over helpers in included private headers.
5. The uncompiled `#ifdef RECREATEBUFFER` blocks in `accelerate-histogram.c` (hold `ComputeEqualizeImage` Large).
6. `distort-linear-private.h`: change the per-pixel polynomial helpers with Ir equality, accept a carve with a product header below the original, or record it held.
7. Confirm IP, BP and the amendments.
8. Add `xevents/0e762518fe` to `cases.LOAD_SENSITIVE` (diverged once under load, equal three times alone).
