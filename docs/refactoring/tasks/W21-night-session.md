# W21 night session (2026-11-12 to 2026-11-13)

Owner rulings of 2026-11-12 ("all of these are ok", seven questions) are in AGENTS.md. This session first merged `origin/campaign/W19` into
`campaign/W21`: PR 96 had base `campaign/W19`, and PR 95 had already merged into main, so PR 96 merged into the old W19 branch and its content
(the xwindow and opencl splits, morphology.c 6.70, the W19b report) was not in main. Four lane agents then worked about two and a half hours
(a workflow with two independent reviewers per lane: rules, evidence). Scores: cs-mcp 1.5.16, `tools/ch.py`. Closing sweep on the merged tree:
campaign A 92 files, mean 8.96 (the new files from splits count as files), 43 at 10.0 (37 at the start of the day's batch).

Gate on the merged tree: whole catalogue 15,955 cases, 0 diverged; wide catalogue 14,200 cases, 0 diverged; `ci_linux_warnings.sh --campaign-a`
OK on 92 files in seven configurations (gcc-noatanh is new); `playbook_lint` 0 problems.

## Merged

**Lane b, Recipe S splits and tools.** `enhance.c` 9.09 -> **10.00** (the held 392a6d9e1; `enhance-color.c` 9.09), `colorspace-transform.c` 9.38 ->
**10.00** (`colorspace-transform-image.c` 9.38), `effect-blur-adaptive.c` 9.09 -> **10.00** (`effect-blur-adaptive-filters.c` 9.09), `visual-effects-pixel.c`
9.09 -> 9.68 (`visual-effects-pixel-methods.c` 9.09). Tools: `ci_linux_warnings.sh` builds the gcc-noatanh configuration (an available build for
`split_branch_check.py`, owner 2026-11-12); `split_source.py` follows function-pointer typedefs and calls through macros. `split_branch_check.py`
prints IDENTICAL-MODULO-LAYOUT for all four splits in ci-gcc-min, ci-gcc-dlg, ci-gcc-noatanh and ci-gcc-x (and cand and cand-wide for enhance and
colorspace-transform). **For the owner:** in the clang -O2 builds (cand, cand-wide) the check prints DIFFERENT for two splits, an inlining decision in
the new translation unit: `AddNoiseImage` (the header's `static inline AcquireRandomInfoTLS` is inlined only in the new file, 295 vs 361 lines) and
`EmbossImage`/`SharpenImage` (clang had inlined the public `ConvolveImage` within one file, now a call, 2 lines each); both are per-call, not per-pixel,
and no timing was taken. Reviewer notes: fab25b294's message says 2,758 lines moved, about 2,782 body lines did (the multiset check lost none).

**Lane d, the `#if`/OpenMP sites.** `attribute-depth.c` 8.16 -> **10.00** and `attribute-depth-measure.c` 8.16 -> **10.00** (E, E, PS step 7, N, FI, OC and D
chains in the `#if !defined(MAGICKCORE_HDRI_SUPPORT)` branch, which the oracle never compiles: evidence is the non-HDRI ci_linux_warnings, private
non-HDRI Q16 OpenMP builds of base and candidate compared at `OMP_NUM_THREADS=1` and `4` (60 runs per step, 0 diverged) and `obj_identical.sh` with
`BUILD=ci-clang-min` (tools commit 7b8500840)); `resample.c` 8.53 -> 9.68 (Q step 8, I, Z step 7; the dead `#else`/DEBUG text moved verbatim, a reviewer's own check found
the same 98 dead lines before and after); `feature-texture.c` 7.63 -> 7.76 (`RecordFeatureGrays`: writes the shared `grays` table through a pointer, which the
write-set rule's letter does not cover; the commit says so and relies on the 2026-11-12 ruling; 26 runs at 1 and 4 threads compared, 0 diverged). Reviewer notes: the
speedup ratios of 0.98 to 0.99 quoted in resample commits 861a9c8f5 and 6fcb03ee6 did not reproduce (measured 1.001 and 0.997: no change either way; only d8a9b8958's
about 1.4% gain reproduces); the cited `deadcheck` script is not in the repository (an independent checker agrees); disassembly line counts in 6bec03d28, 088b2d86f do not reproduce
(the identity conclusions do).

## Held for the owner

**Lane a, branch `w21a-splits-held`:** `animate.c` 4.40 -> **10.0** (new `animate-window.c` 4.4) and `display.c` 2.68 -> **10.0** (new `display-image.c` 2.68), via `split_source.py`
extended to carry a family inside a file-level `#if` with its wrapper, two-line forward declarations, enumerations, tables and an `extern "C"` pair
(e9f60a2b5, a49717cfc). Both splits move two `#if defined(__cplusplus) || defined(c_plusplus)`/`extern "C" {` pairs verbatim (XPredicate, SceneCompare): no available build compiles
the C++ side, so they fit neither the 2026-11-01 `#if 0` exception nor the 2026-11-11 one. Held on that account; the rest of lane a's checks are green (wide 14,200 and narrow 15,955, 0
diverged; split_branch_check IDENTICAL-MODULO-LAYOUT in cand-wide and ci-gcc-x). **Question: may a Recipe S split move a verbatim `#if defined(__cplusplus)` `extern "C"` pair?** Also on
the branch: Recipe TF (identical-twin fold; FT is the fall-through recipe in main) with `tools/twin_fold_check.py`, whose gate has three holes (a body naming a macro that expands to
`GetMagickModule()`, the "not inside a preprocessor conditional" condition is not checked while its own example sits inside the X11 `#if`, and a third) and which is used by no merged commit.
`widget.c` stays 3.15: three TF folds keep 20 File-browser helpers at 3.15, the file steps down to 3.01 at 259 non-trivial functions, and finishing File and List needs about 35 more steps (branch `w21a-widget-chain`);
no widget.c split exists (shared statics).

**Lane b:** `compare.c` 9.68 -> 10.00 by moving `CompareImages` (new `compare-images.c` 9.68) passes every check but `compare.c` has no Lines of Code finding, only Excess Arguments, so Recipe S's *Use when* is not met
(local branch `w21b-compare-split-held`). `enhance-tone.c`, `feature.c`, `segment.c`, `statistic.c`, `transform-region.c`, `vision-components.c`, `feature-texture.c`, `distort.c`, `effect-filter.c`, `compare-similarity.c`:
every public root is linked to every other by shared statics (one closed set). `threshold.c`: all 30 unions are refused or flat.

**Lane c (no commit):** `composite.c` 3.69: the Saliency/Seamless twin merge scores 3.13 (shared relax loop plus a Prepare helper each) or 3.29 (loop only), because the callers clone each other: below 3.69, so
the lane reset (18 flat commits, new Recipe MP, parked on `w21c-composite-flat-chain`). `fx.c` `ExecuteRPN`: a 2-way range-test split costs +3.7% to +5.0% instructions, a 2-level chain +4.1% to +8.2%, `always_inline` does
not help. `compare-similarity.c`: Recipe S refused (36 statics shared); the shared `MergeChannelSimilarity` (Recipe CB) applies flat; stubbing all six gives 3.01.

**Lane d, left:** `morphology.c` `AcquireKernelBuiltIn` (a new helper keeps its own Bumpy Road; `#if 0` labels count in cc), `paint-floodfill.c` Recipe H plus N reaches 7.81 below 8.35 and cannot go further without editing
`PushSegmentStack`, `layer.c`, `fourier.c` (five near-twin sections), `magnify.c` (Recipe NS step 1 forbids a call in the condition), `distort.c` and plasma (not on the ruling's list).
