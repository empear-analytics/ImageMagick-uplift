# W46 round thirty-three (2026-11-19)

Campaign A, lanes `campaign/W46-a/b/c` cut from `campaign/W45` `44f5f4392` (PR #129, round 32), merged back into `campaign/W45`. Three Opus lanes in batches, one Sonnet reviewer per batch; every batch merged only after its review. Started on the owner's answer of 2026-11-19 to PR #129 Q7: "Sure go. New recipes are fine. Keep grinding." That answer is not yet written into AGENTS.md (the edit was blocked by a permission check; the owner may add it).

## Result

Campaign A sweep (cs-mcp 1.5.16, 150 `.c` files): mean 9.75 → **9.78**, files at 10.0 **89 → 90**. MagickCore 9.44 → 9.46.

| File | Before | After | How |
| --- | --- | --- | --- |
| `accelerate-despeckle.c` | 8.55 | **10.0** | DI, a stub-probed I/Y/F/N chain (two disclosed `--no-verify`), SE, E, E |
| `accelerate-resize.c` | 8.18 | 9.30 | DI, then I, JD, FU, MV (the two arm twins merged; one disclosed `--no-verify`) |
| `accelerate-motion-blur.c` | 9.30 | 9.68 | DI |
| `accelerate-point.c` | 9.09 | 9.68 | D, E, E, E, V (a probed five-step plan clearing *Overall Code Complexity*) |
| `accelerate-histogram.c` | 8.75 | 8.85 | DI, I (step 8 throw-macro form), V |
| `compare-similarity-image.c` | 8.51 | 9.02 | D, GL, H, N (the OpenMP row loop moved whole), E |
| `compare-similarity-frequency-images-private.h` | 8.87 | 9.09 | HN step 7, E step 8 (the OpenMP row loop moved whole) |
| `compare-similarity-metric-private.h` | 7.50 | 7.73 | D |
| `compare-similarity-private.h` | 8.63 | 8.66 | FB inside the SSIM per-pixel loop (region code identical, Ir equal) |
| `compare-similarity-phase-private.h` | 8.01 | 8.03 | W |
| `accelerate-histogram-private.h` | 9.68 | **10.0** | B |
| `opencl-environment-private.h` | 9.38 | **10.0** | BP chain (three steps) |
| `composite-channel-private.h` | 6.17 | 6.75 | OH Form A carves; `composite-channel-groups-private.h` and `-mix-private.h` 10.0 |

No including `.c` file dropped at any step. The remaining 9.68 `.c` files are held only by a MagickExport or MagickPrivate argument count.

## Evidence

- Gates on `03c4579ec`: `ci_linux_warnings --campaign-a` OK (150 files, 7 configurations); narrow oracle 15,966 cases, **0 diverged**; wide 14,211 cases, **0 diverged**; `check_trailers` 123 refactor commits valid (range `46b61ab8d..HEAD`); `playbook_lint` 179 recipes, 0 problems. Baseline `46b61ab8d`. Each lane also ran both catalogues on its own tip.
- Speed (AGENTS.md 2026-11-18). Steps inside OpenMP regions or per-pixel loops were measured by callgrind Ir at one thread in the OpenMP -O2 HDRI build and checked with `omp_compare` at 1 and 4 threads on cases that start threads: `SetUnityImageRows._omp_fn.0` Ir equal (1,230,238 + 1,080,200), 108 instructions on both sides; `MeasureSimilarityImage._omp_fn.0` 205 instructions on both sides, inclusive Ir 335,252,346 → 334,509,529; `GetSSIMSimularity._omp_fn.0` equal and the region identical; `GetPHASESimilarity._omp_fn.0` equal. The `CompositeChannelColor` carves leave `composite-image.c` preprocessed- and object-identical under clang and under gcc -O2 -fopenmp (re-run by the reviewer). Everything else is once-per-call OpenCL host code or object-identical. No step raised a hot function's Ir.
- `--no-verify`, each disclosed in its message and inside a chain stub-probed first whose committed end raises the file: `170aed8bb` (resize 8.21 → 7.79, chain end 9.30); `c14a59fec` and `7df4e2d85` (despeckle 8.70 → 8.34 and → 8.18, chain end 10.0).
- New tool `tools/obj_sorted.sh` (Recipe DI step 5): compares object code per function with relocations attached and accepts only reordered stores of constants to distinct stack slots. The first version dropped relocations (a reviewer showed it passing a retargeted call and two swapped calls); fixed in `645f80277` with a `--self-test` for both cases. The one DI step that relied on it gives the same verdict with the fixed script.
- New omp case file `tools/omp_cases/compare-similarity-image-large.txt` (600x600 subimage searches, large enough to start threads).

## Stopped for speed (not committed)

- `SIMMinimaImage`/`SIMMaximaImage` merge: about 7 Ir more per row in each region (the selector is read from the shared record every row).
- Recipe PS on `MeasureSimilarityImage`'s progress block: 9.02 → 9.09, but region self Ir +433 (one more spill per row). Owner question 1.
- SSIM `window`/`sums` fills as initialisers: object-identical under the oracle's flags, but +0.06 % Ir in the OpenMP region. This led to the Recipe FB step 6 amendment.

## New recipes and amendments (under the owner's general OK of 2026-11-19; please confirm)

- **GL** Fold a Guard into the Lookup It Encloses, Returning NULL When It Fails.
- **DI** Declare a Local with the Constant Its First Statement Assigns; amended (declared before the first statement, no `goto` before the run, same preprocessor conditions, blank lines, C++ check), and **step 5's exception for reordered constant stores (relaxes IDENTICAL; explicitly for the owner's confirmation)**.
- **JD** Join Twin Call Arms Around a Common Fill into One Status Dispatch.
- **MV** Merge Twin Functions Differing in Values Through an Order Struct.
- **SE** Share Identical Exiting Copies Through One Helper Binding the Input Struct.
- **HN** step 7, the self-guarded head.
- **FB** step 6: a fill inside an OpenMP region or per-pixel loop also needs the OpenMP -O2 region identical or its Ir equal or lower.
- **BP** step 2 (a typedef inside the `#if` block holding all its users) and **B** step 5 (plain-identifier block form; a call site a header B step forces inside an unreached caller takes the *Unreached functions: B and N* checks).

## What holds the rest

- 9.68 `.c` files (motion-blur and point included): MagickExport/MagickPrivate argument counts.
- `accelerate-resize.c` 9.30: `RunResizeFilterPass`'s local-memory loop and argument counts. `accelerate-histogram.c` 8.85: the `#ifdef RECREATEBUFFER` blocks (#129 Q5).
- `compare-similarity-*`: the rest sits in per-pixel region bodies; every form tried raised Ir or scored flat; the frequency header's method table is #129 Q2.
- `composite-channel-private.h` 6.75: `CompositeChannelColor` (cc 141, per pixel); one-case extractions give +0.07 and then lower the header, so not pursued.
- `xwindow-import.c` 9.38: every split shares the `XSelectWindow` family and `XClientWindow` (owner question 3). `animate-window.c` 8.81: Number of Functions needs a carve (#126 Q2).
- `fourier.c`, `opencl-initialize.c`: #126 Q8. `statistic-image.c`: the hot `GetStatisticPixel` switch and the public `StatisticImage`.
- `accelerate-blur-private.h` 9.68: owner question 2.

## Owner questions (new)

1. May a step that raises a region's Ir by a tiny amount be taken? Recipe PS on `MeasureSimilarityImage` gives 9.02 → 9.09 at +433 self Ir in its region (one spill per row). As the rule stands: no.
2. May a header Recipe B step change the two calls in the never-called `AccelerateUnsharpMaskImage` on `obj_modulo` MODULO-IDENTICAL (the playbook asks IDENTICAL when a function has more than one caller)? `accelerate-blur-private.h` 9.68 → 10.0.
3. `xwindow-import.c` (9.38): may a split move the shared `XSelectWindow` family and `XClientWindow` into a private header (as #126 Q2 asks for X11 files)?
4. Upstream private headers that campaign B's files and coders also include (`colorspace-private.h` 3.69, `token-private.h` 6.94, `quantum-private.h` 7.55, `composite-private.h` 8.82, and others below 10.0): campaign A's to work, or out of scope for both campaigns?
5. For #126 Q8 (`#else` branches no build compiles): add a CI configuration with `ac_cv_header_complex_h=no` (like `gcc-noatanh`) so `fourier.c`'s `#else` branches are compiled and fall under the 2026-11-12/13 rulings?
6. Confirm the recipes and amendments above, in particular the DI step-5 exception.

`infra/0fd402f13a` timed out on the baseline under load in two lanes (equal when rerun alone): more evidence for #126 Q5.

## Upstream oddities noted, not touched

- `accelerate-despeckle.c`: `DestroyAccelerateViews` is reached with a NULL view when `RequestOpenCLDevice` or `AcquireOpenCLCommandQueue` fails (`DestroyCacheView(NULL)` asserts); the `AcquireOpenCLKernel` returns are unchecked; in `RunHullPassLoop` a `if (k==0) clStatus=...` value is overwritten before it is read.
- `accelerate-histogram.c`: `ComputeEqualizeImage`'s `ThrowBinaryException` on histogram allocation failure returns without releasing the queue and device.
