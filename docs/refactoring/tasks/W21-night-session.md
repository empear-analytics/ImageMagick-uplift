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

## Round two (2026-11-13 night)

Four lanes again (workflow with two reviewers per lane), cut from `campaign/W21` f01ce7a2a. Gate on the merged tree: whole catalogue 15,955 cases, 0 diverged; wide catalogue 14,200
cases, 0 diverged; `ci_linux_warnings.sh --campaign-a` OK on 92 files in seven configurations; `playbook_lint` 0 problems. The casemaps (narrow and wide) were stale and are rebuilt.
Sweep: campaign A 92 files, mean **9.03**, **44** at 10.0 (43 after round one).

**Merged.** Lane c (50 commits): `xwindow-import.c` 6.37 -> **9.38** (XImportImage, XSelectWindow, XGetWindowImage and XImportWindowImage leave Bumpy Road, Complex Method, Large Method and
Overall Code Complexity; only Lines of Code in a Single File is left, and cutting it would re-split a split file), `xwindow-visual.c` 7.28 -> 7.97 (`XBestVisualInfo` cc 41 -> 18, LoC 232 -> 110;
the depth-4 loops sit in `#if !defined(PRE_R4_ICCCM)`), `opencl.c` 7.59 -> 7.72 (`LoadOpenCLDeviceBenchmark`); playbook amendments EJ step 1 and O step 2. `xwindow.c` stays 2.55: stubbing any
one of its 11 large functions leaves it at 2.55 to 2.57, `XMakeImage` (19 bumps, depth 4, `HAVE_SHARED_MEMORY` `#if` blocks) alone 2.90, all twelve 6.44. Lane d: `magnify.c` 8.84 -> **10.00**
(new Recipe CQ and its step 7 amendment, Recipe NS on one `Xbr2X` site, Recipe P; six refactor commits). Lane b, `layer.c` 8.25 -> **8.92** (`OptimizeLayerFrames`: E, N, O, I, E, N and new
Recipe EX twice; nine commits).

**Evidence caveats the reviewers found (not rewritten; read before relying on `Verified: oracle`).** Lane c's reach judgements were made with a stale wide casemap (dated 2026-10-30, 5,394 functions):
commit messages that say "0 cases" for a helper are unreliable in both directions, and several X11 helper bodies are reached by no case (a fresh map: `XImportChildWindows` 0 of 8 lines covered, so
2d33ff0ea; `XSelectWindowByIDOrName` 0 of 11, so 2c64babc2; `XCropToRootWindow` 12 of 23 lines unexecuted, `XEliminateOrphanedWindow` 6 of 11, `XSetDescendForColormaps` 5 of 25, `XNameWindowImage` 6 of 14). For
those blocks the evidence is the guard, `obj_modulo.sh` and reading, not the oracle, although the trailer says `Verified: oracle` (the function-level criterion is met). 22cf6be2b says no case executes
`XImportWindowImage`; a fresh map says 173 cases do. 2d22c8814's message says cc 9 -> 8 for `XImportImage`; the 8 was inferred, not read. 9f446fc53's cc and LoC figures mix two commits. Lane d's
timing ratios (four decimals) sit inside measurement noise; every reviewer re-measure stayed within about +/-1.3% instructions (`Scale2X` plasma +1.4% to +2.2% CPU on two 21-run series). `Scale3X`'s CopyPixelsIf is not
directly above its users, as CQ step 3 says.

**For the owner (round two):**
- **Two block-scope `static const` tables moved into helpers verbatim** (9f446fc53 `colormap[MaxStandardColormaps]` via O; 86bc6cd85 `rank[]` via E): neither recipe says anything about it and AGENTS.md says never modify a
  static const table. The literal diff is empty, guard OK, 535 wide cases 0 diverged. Confirm, or revert those two (later `XBestVisualInfo` steps depend on them).
- **`layer.c`'s moved `#if DEBUG_OPT_FRAME` blocks name variables that do not exist in the helpers** (compiled with `DEBUG_OPT_FRAME=1` the base gives 0 errors and the head 21); the same holds for the resample dead text merged in
  round one. The blocks are dead in every build; the owner's ruling lists layer and resample; a debug build would no longer compile until the macro names are adjusted (which the rules forbid).
- **Recipe CQ step 7** (a `(C ? a : k)` argument) stretches the recipe and sits on Recipe NS territory; CQ's *Not covered by* still says NS needs identifiers. `Xbr2X`'s NS step differs from base in one of 1,055 instructions
  (`setbe` vs `setle` on a 0/1 weight), accepted under your 2026-11-12 ruling. The new recipes (CQ, EX; the amendments to EJ and O) were written under the 2026-11-10 OK, though the catalogue rows say `Decided by: owner`
  where the table's `mandate` form fits better.
- **Held: Recipe TS and the fourier.c step** (`e6b0c2997`, `5b87e13b0`; branch `w21b-layer-fourier-all`): `InverseFourierTransformImage`'s twin `omp parallel sections` bodies share one helper that writes the shared
  `status` through a pointer: 7.59 -> 7.66. The write-set rule's letter forbids passing the address; the lane relied on your fourier ruling, and the evidence is 200 runs of 25 hand-picked commands at 1 and 4 threads, not
  the 10 reaching oracle cases. A real OpenMP semantics question for so small a gain; not merged.
- **Held: lane a, `widget.c`** (branch `w21a2-file-chain`, 41 commits): File browser finished (`XFileBrowserWidget` has no finding), but the file ends at 3.08, below 3.15 (a step-down to 3.01 at the 18th helper, to 2.92
  when the function leaves Complex Method), repaid only after List: stubbing `XListBrowserWidget` on this tip gives 3.99, then +TextView 5.14, +Preferences+Notice 6.89 (about 20 steps each, not started). Seven of its commits
  were committed with `--no-verify` although the gate would have passed. A cheaper probe: stubbing `XMenuWidget`, `XScreenEvent` and `XEditText` together gives 3.17.
- **No step found:** `paint-floodfill.c` (8.35: the macro hoist alone 7.03; a stack-loop helper reaches 8.74 but `PushSegmentStack` names the loop-written local `s`), `quantize.c`, `distort.c`, plasma and `accelerate.c`
  (lane d did not reach them), `layer.c`'s remaining Complex Method (three failure exits through one helper adds the cases back).
