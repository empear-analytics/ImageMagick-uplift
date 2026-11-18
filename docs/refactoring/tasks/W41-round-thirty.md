# W41 round thirty (2026-11-18): split products, accelerate twins, pixel gamma fast path

Base `13741080d` (main `71f80adb8` after #122, plus the record of the owner's 2026-11-18 answers: "yes to 1, 2, 3, 5. 4 we keep pushing. and for 6 we make it all 10 sooner or later"). Three Opus lanes, one Sonnet reviewer each; all three lanes merged. Every hot-path step timed in the OpenMP -O2 HDRI build at 1 and 4 threads beside an A/A run (21 rounds, re-run at 41 when a first run was more than 0.03 above its A/A; every run quoted in the messages).

| File | Before | After | How |
| --- | --- | --- | --- |
| `morphology-builtin.c` | 8.79 | **10.0** | lane b: three Recipe OH steps (Form A, leading runs) into `morphology-builtin-shape-private.h` (10.0), `-type-private.h` (10.0), `-base-private.h` (9.36: `IsBaseKernelNeeded`'s `#if 0` labels). |
| `accelerate-local-contrast.c` | 9.68 | **10.0** | lane b: Recipe B. |
| `morphology-primitive.c` | 9.09 | 9.68 | lane b: two OH steps into `morphology-primitive-walk-private.h` and `-direct-private.h` (9.38 each); `MorphologyApply`'s 7 MagickPrivate arguments remain. |
| `composite-image.c` | 5.01 | 7.48 | lane c: OH Form B moved the `CompositeChannelColor` family (unchanged, performance-bound) into `composite-channel-private.h` (5.65); two SD steps, FB, new Recipe RX. |
| `accelerate-resize.c` | 7.52 | 8.11 | lane b: D (typedef), D form (b) with the getter reorder (disclosed `--no-verify`, 7.11, stub-probed), TI merge of the horizontal/vertical filter passes, N, V, Y. |
| `accelerate-blur.c` | 8.98 | 9.34 | lane b: B, Y, D. |
| `accelerate-motion-blur.c` | 8.43 | 8.46 | lane b: Y. |
| `compare-similarity-image.c` | 5.83 | 5.92 | lane a: CB (Recipe CB amendment: a critical governing one unbraced `switch`), two E (two disclosed `--no-verify`, 5.72 and 5.77, stub-probed chain). |
| `compare-similarity-private.h` | 5.11 | 5.13 | lane a: SC (`GetSSIMConstant`). |
| `pixel.c` / `pixel-gamma.c` | 8.74 / 10.0 | same | lane c: performance fix, `pixel-gamma-private.h` (10.0) restores `GetPixelIntensity`'s inlined linear gamma segment (see below). |

**Gates on `621f3834f`:** narrow 15,961 and wide 14,206 cases, 0 diverged (baseline `71f80adb8`); `ci_linux_warnings.sh --campaign-a` OK on 150 files; `check_trailers` 25 refactor commits valid; `playbook_lint` 168 recipes, 493 change rows, 0 problems. **Sweep** (cs-mcp 1.5.16): campaign A 150 files, mean **9.68** (was 9.64), 84 at 10.0 (was 82); MagickCore 9.41 over 251 files. The sweep scores `.c` files only; the private headers carrying moved debt are listed above.

## Notes

- **Pixel gamma fast path** (`e4e01a134`, `perf(MagickCore/pixel.c)`): in the fork GCC inlined the linear segment of `DecodePixelGamma`/`EncodePixelGamma` into `GetPixelIntensity` and called only the power-law part out of line; the `pixel-gamma.c` split made every gamma-weighted sample a cross-object call. The new header holds the helpers verbatim, defined once; the exported functions keep their definitions (no export or ABI change; `nm` identical apart from local symbols). Callgrind on a dark image: 530.2M (main) to 389.1M (fix), fork 386.7M; timing 0.83 to 0.93 against main on dark images, back to the fork; typical images take the power-law path, no measurable change. The helpers are plain `static`, not `static inline`: `static inline` inlined the whole polynomial at 12 sites and measured `-shade` 1.070, `-paint` 1.052, `-normalize` 1.053 against A/A; plain `static` reproduces the fork's partial inlining. Owner question 1.
- **Recipe OH** was written by lanes b and c independently (a leading run; a family). The reviewer merged both into one recipe with Form A (pp and object identity, `Verified: guard`) and Form B (`split_branch_check` IDENTICAL-MODULO-LAYOUT, oracle, OpenMP timing); every `Recipe: OH` trailer stays correct. Recipe RX (split a selection at its default, returning whether a moved group returned) stays a new recipe; Recipe CB gained an amendment.
- **NCC at 4 threads** gives 16383.8 or 16383.7 between runs, in the fork too (15 runs each: fork 13/2, main 12/3); summation order under a critical, not a campaign change; listed beside DPC.
- **Upstream oddities, not touched:** `SimilarityImage`'s `threshold_trigger` is set inside the critical but read only before it, so a met `-dissimilarity-threshold` never stops the scan, and `progress++` in its row loop has no `omp atomic`; `ComputeMotionBlurImage` never releases `offsetBuffer` (also in upstream `accelerate.c`).
- **Catalogue timeout:** narrow case `infra/0fd402f13a` (`-duplicate 10299 +adjoin`) times out on the baseline under load (30.7 s against a 30 s limit); output identical by hand.
- The `accelerate-unsharp.c` and `accelerate-contrast-stretch.c` call sites are commented out upstream, so no oracle case reaches them (casemap 0).

## What holds the rest

- `compare-similarity-private.h` (5.13): the per-channel trait-skip loop repeated in about 20 functions; `GetPHASESimilarity`, `GetMEPPSimilarity`, `GetSSIMSimularity`, `GetCorrelationTotals` (probe without them 6.89). `compare-similarity-image.c` (5.92): the four FFT metrics need about 5-6 Recipe I steps each (probe 7.38).
- `composite-image.c` (7.48): an RX step on Saliency/Seamless (+0.08); `CompositeImage`'s 7 public arguments. `composite-channel-private.h` (5.65): `CompositeChannelColor` (performance-bound) and the `GetSoftBurnBlend`/`GetSoftDodgeBlend` duplication (per-pixel; a timed merge).
- `fx.c` (9.09): a four-header carve probes at 8.95 because CodeScene attaches the include block to the first remaining function; a mid-file include would give 10.0 (owner question 2).
- `accelerate-point.c`, `-despeckle.c`, `-histogram.c`: `goto` cleanup chains, Overall Code Complexity, `#ifdef RECREATEBUFFER` blocks no build compiles (+0.02 to +0.05 per I/Y step).
- `animate-window.c` (8.81): body inside `#if defined(MAGICKCORE_X11_DELEGATE)`, `XAnimateImages`' 6 public arguments.

## Owner questions

1. `pixel-gamma-private.h` holds plain `static` helpers (one copy per includer) rather than the `static inline` of the 2026-11-16 rulings, because `inline` was 5-7% slower. Accept?
2. May a one-file private header be included mid-file, where the moved text stood (would take `fx.c` to 10.0)?
3. Confirm Recipe OH (both forms), Recipe RX, the Recipe CB amendment, and the lanes' readings (SD chain sharing one targets struct; RX locals by value on RV step 6 terms).
4. May oracle cases be added for the disabled `AccelerateUnsharpMaskImage` / contrast-stretch paths so those files can take oracle-level recipes?
5. Should `infra/0fd402f13a`'s timeout be raised or the case listed as load-sensitive?
