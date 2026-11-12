# W19 morning session (2026-11-12)

Batch cut from origin/main 7aebe4c2b after #94 (W18) merged, for the two hours to 9 am the owner asked for. Sonnet 5.5 lanes
in `~/im-w14a..d` on `campaign/W19-a..d`; each finished lane merged into `campaign/W19` and gated on the whole catalogue
(and `tools/ci_linux_warnings.sh --campaign-a`, six configurations) before every push. Start: campaign A 84 files, mean 8.86,
36 at 10.0. Housekeeping first: CI on merged main fails only on campaign B files (type.c, utility.c, quantum-import.c,
pixel-import.c, pixel-export.c, annotate.c, blob.c); whole wide catalogue on merged main 14,200 cases, 0 diverged; the stale
`casemap.json` rebuilt (`tools/oracle/build.sh cov`, then `casemap.py`: 15,955 cases, 8,180 functions reached, about 5 minutes,
in the foreground: a detached background job is killed when the `wsl` call returns).

## Pattern mining (no commit)

**Lane d, narrow files:** scanners for HT, UX, SG, LC/CR, PL/TB/KT, LS, RT and FB/FI over every below-10 narrow file found
no legal chain that raises a score (HT 0, UX 0, LC/CR 0, PL/TB/KT 0 legal matches, LS 0; one SG candidate: `PlasmaImageProxy`,
rejected: SG alone 8.50 -> 7.98, SG plus CN 7.63, Bumpy Road stays at 3 because each block still holds its missing-pixel
`return` test and a Complex Conditional appears; composite.c's stub ceiling 3.69 -> 3.42 with the five statics stubbed, 3.74
with eight, so it cannot move without the owner-held chains; `MorphologyApply`'s stage-limit switch would give a cc-9
helper and the caller stays Complex and Large; `poly_basis_dx` cases return expressions, not constants, so TB does not
apply). The only new idea: a Recipe S split of `montage.c` to free its 1,000-line budget for `MontageImageList`.

## Lane a: opencl.c 5.80 -> 7.36, accelerate.c 3.19 -> 3.27 (wide build, merged)

6 commits; guard and `--calls` PASS; `ci_linux_warnings` OK in all six trees; whole wide catalogue 14,200 cases, 0 diverged
(at the opencl tip and again at HEAD); merged gate 15,955 cases, 0 diverged. 783febde5 opencl.c `GetOpenCLEnabled`, Recipe V
step 6: a new helper `HasCurrentOpenCLEnv(&clEnv)`; 5.80 -> 6.33; the Code Duplication category disappears (`GetOpenCLEnabled`
and `SetOpenCLEnabled` were a clone pair, one of three; six duplicated functions fall to four). 919b6f0db playbook: **new
Recipe GC, Guard Returns from a Conjunction** (`if (A1&&..&&An) return(T); return(F);` becomes one `if (Ai') return(F);` per
operand, then `return(T);`); e84534496 `IsBenchmarkedOpenCLDevice` GC 6.83; 507ac087a `IsSameOpenCLDevice` GC 7.36 (Complex
Conditional leaves the file; the two twins do not clone-flag). 349376f4d playbook: **new Recipe GD, Guard Returns from a
Disjunction** (`if (A1||..||An) return(F);` becomes n guards); fa5ac1e23 accelerate.c `AccelerateGrayscaleImage` GD 3.27
(Complex Conditional leaves; obj_modulo IDENTICAL). Both recipes Decided by `owner` under the 2026-11-10 OK to evolve the
recipes; they split a short-circuit chain into sequential guards, preserving the operands' evaluation order and
short-circuit (the How text is in the playbook commits). Mining (`mine.py` over functions with findings): HT 0, SG-a 0,
UX 1 (display.c, its inner `if` has no exit, so it does not fit), SG-b 36 hits (mostly xwindow, 25; one-function stubs flat),
PL 1 (widget.c `XScreenEvent`: macro labels, not enums or decimal literals), LC/CR 19 blocks and FILL/RT 286 runs (mostly used by
earlier lanes). **The Code Duplication threshold is a cliff:** six duplicated functions flag; with four the category is gone
(+0.53); adding a new twin pair brings it back (an E helper `SetScoreOnSameOpenCLDevices`, twin of
`EnableSameOpenCLDevices`, scored 6.77 against 7.36). Rejected: accelerate.c Recipe P predicate `HasUndefinedRGBTraits` (flat
3.19: the new function adds a clone); `BenchmarkOpenCLDevices` inner loop E (-0.59, the clone cliff); the doctype scan of
`LoadOpenCLDeviceBenchmark` as a helper (7.36 -> 7.43, but the helper is itself Bumpy and the branch is unreached: owner
question). Remaining open for the owner: opencl.c `GetOpenCLCacheDirectory` (stub 7.65, pinned by its `#if` code);
accelerate.c's ten wrapper clones (the preamble question); animate.c `XMagickCommand` cc 9 (stub 4.49) needs throws moved into
helpers (the `GetMagickModule()` question), `XAnimateBackgroundImage` and `XAnimateImages` stubs 5.30 and 6.73 but 20+ steps;
`GetOpenCLPlatformString` and `GetOpenCLDeviceString` are the last twin pair (function pointers). Tooling: backgrounded `wsl`
jobs freeze (the clock stops): run the oracle in the foreground; with `WIDE=1` the `casemap.json` does not match the wide
catalogue (`--function` fails): run the whole wide catalogue; `verify_wt.sh` prints VERIFY FAILED for wide files.

## Complex Conditional scan (no commit) and closing

Lane c (wide files) and lane b (narrow files) listed the review categories of all 84 campaign A files (`tools/ch.py --review`,
cross-checked against `docs/refactoring/tasks/W18-survey.md` and its 596-row table): the only *Complex Conditional* findings
were `AccelerateGrayscaleImage`, `IsSameOpenCLDevice` and `IsBenchmarkedOpenCLDevice`, which lane a's Recipes GC and GD
already removed. Recipes GC and GD have no further targets in campaign A (a new one could only appear as a side effect of
another recipe). Nothing was committed by lanes b, c or d in this batch.

Closing sweep of the tip (cs-mcp 1.5.16): campaign A 84 files, mean **8.86 -> 8.88**, files at 10.0 unchanged at 36. The
lowest files: `xwindow.c` 2.47, `display.c` 2.68, `compare-similarity.c` 2.97, `widget.c` 3.15, `accelerate.c` 3.27,
`composite.c` 3.69, `animate.c` 4.40, `fx.c` 4.61, `morphology.c` 6.19, `opencl.c` 7.36 (up from 5.80). Gate on the merged
tip: 15,955 cases, 0 diverged; `ci_linux_warnings.sh --campaign-a` OK on 84 files in six configurations.

**Where the campaign stands:** every remaining finding in the 48 files below 10.0 is one of: a public signature, a file-level
count (lines, functions), code inside an `#if defined` branch, an owner question (listed in the W17 and W18 reports and the
PR descriptions), or a cliff (Code Duplication threshold, added lines, Large Method at 70 lines) that a helper-only chain
cannot cross. Mining for every recipe shape added on W17 to W19 found no further legal raise. The routes that remain are the
owner's: the twin-fold recipe (widget.c, and the File and List browsers), the magnify `?:` fold, the `#if` and OpenMP
questions, the wrapper preamble, and the one new idea of this batch, a Recipe S split of montage.c to free its 1,000-line
budget for `MontageImageList`.

## Afternoon: montage.c 9.11 -> 9.12 (merged); the split re-scan; one split held

**Lane d, montage.c.** `split_source.py` dry-run on every function as a root: every static, including the families around
`PlaceMontageTile`, `WrapMontageRow`, `CopyMontageTiles`, `AdvanceMontagePage` and the tile placement statics, is refused for
shared statics (a non-public family cannot move because `MontageImageList`, which stays, calls it); `MontageImageList` with all
its statics is refused at the `extern "C" {` block around `SceneCompare`; `MontageImages` alone is accepted (14 lines, the new
file scores 10.0, guards unchanged, 29 and 49 oracle cases pass) but frees only 12 lines: the file's real margin is 15 lines
under 1,000 (padded probe 1,007 at 22 added lines), so the split gives about 27 lines of room, which only a whole-loop helper
(9.27 on a scratch copy) would use, and that cannot be formulated legally (three exits, two of them `ThrowImageException`,
one silent `return NULL`); the full Large Method fix (164 -> 70 lines) needs one or two structs of 20 or more members. The
split was not committed. Merged: 3d369e2a6 E `LogMontageImages` (the trace-event block; cc 10 -> 9, 164 -> 163 lines, flat,
first step of the plan; the trace event under `-debug` now names the helper as the function) and ebdea5c25 E
`DestroyMontageTexture` (cc 9 -> 8, leaves Complex Method, 163 -> 162 lines): **9.11 -> 9.12**. Whole catalogue 15,955 cases,
0 diverged. Plateau 9.12: Large Method at 162 lines; the remaining blocks need large struct fills or more than four parameters.

**Lane a, Recipe S re-scan.** For 26 files with Lines of Code or Number of Functions findings (plus xwindow, resample, magnify,
plasma, floodfill, compare, colorspace-convert, attribute-depth*) the current tool was run on every public root and on every
union of closed root sets (roots linked by shared statics); accepted unions were scored on both files. **One legal split
rises: effect-blur.c 9.09 -> 10.00** (commit 6817fef41 on `campaign/W19-a`; HELD, see below). Refused (one closed set or
shared statics): compare-similarity, composite, fx, feature, segment, enhance-tone, effect-filter, statistic,
transform-region, vision-components, feature-texture, fourier, distort, magnify, plasma, floodfill, accelerate (14 roots),
animate (2), widget (13; `DestroyXWidget` refused on type `XWidgetInfo`), display (`XDisplayImage` = 197 functions), quantize,
layer (7 roots plus `RemoveZeroDelayLayers`). Accepted but not raising the kept file: enhance, threshold (even with
`--copy-macro ThresholdsFilename`), layer, accelerate, animate, morphology (63 unions, Number of Functions stays 161+): flat;
visual-effects-pixel (only moves that leave the kept file a stub rise; the new file 8.54, below the original: unnatural
boundary); colorspace-transform (rises only by moving the namesake `TransformImageColorspace` family out: unnatural
boundary). **Accepted by the tool but the output is invalid:** opencl, xwindow, display, animate: the wide files sit inside
an `#if` X11 or OpenCL block (with a stub `#else` for some) and the new file opens that `#if` without closing it, or carries
none ("unterminated conditional" for opencl; undeclared X types for display and animate). xwindow: 63 accepted splits, all
rule-OK; the kept file rises only 2.47 -> 2.49 to 2.55; new files score 5.0 to 9.5 because finding-bearing functions leave (the
one-function-per-file lever, e.g. `DestroyXResources` alone 8.15); a natural candidate is `XBestVisualInfo`,
`XDisplayImageInfo`, `XInitializeWindows` (kept 2.53, new 7.28); it needs a tool extension to wrap the new file in the `#if`
and close it: an owner question. enhance.c: the four-root family (Contrast, Enhance, Modulate, Sigmoidal) plus WhiteBalance
would put enhance.c at 10.0 (kept under about 500 lines, new about 9.68); two blockers: function-pointer typedefs between
functions (a six-line patch to the tool, tested on a scratch copy and not committed) and the multi-definition `#if/#else`
macros of Sigmoidal and ScaledSigmoidal.

**The effect-blur.c split (6817fef41), held on branch `w19-effect-blur-split` (in `~/im-w14a`) for the owner:** moves 9 publics
(AdaptiveBlur, AdaptiveSharpen, BilateralBlur, Convolve, Emboss, MotionBlur, RotationalBlur, SelectiveBlur, Sharpen), 69
statics and typedefs (78 functions, 4,235 lines) to a new `effect-blur-adaptive.c` (9.09, the original's score); the kept
file holds `BlurImage` and `GaussianBlurImage` with one static (236 lines) and scores 10.00. Only a union works: single roots are
refused (shared `SetBlurProgress`, `ReleaseFailedImage`, `MagickSigma`), and `ConvolveImage` must move with the others
because Emboss and Sharpen call it. Checks: guard `--combined` 309 literals and 617 call sites unchanged; `ci_warnings` and
`ci_linux_warnings` clean; `nm` shows no linkage change; oracle on all 11 functions, 0 diverged; whole catalogue 15,955
cases, 0 diverged; the CodeScene gate passed. `split_branch_check.py` prints IDENTICAL-MODULO-LAYOUT in ci-gcc-min, ci-gcc-dlg
and ci-gcc-x; for `cand` and `cand-wide` (the builds that compile the OpenCL branches) it prints DIFFERENT because clang emitted
`ConvolveImage` ahead of `BilateralBlurImage` in the original object; a per-function comparison by name (lane a's copy of the
tool's routines) is identical at -O2 (10 functions) and -O0 (90 functions): the tool should compare by name. **Held because**
(1) it moves 9 of 11 public functions to a file that keeps the original's score while the kept file becomes a near-stub (the
open owner question on splits that relabel most of a file), and (2) the 2026-11-11 ruling's check does not pass as written in
the two builds that compile the OpenCL branches. If the owner allows relabelling splits and a by-name comparison in
`split_branch_check.py`, it merges as is.
