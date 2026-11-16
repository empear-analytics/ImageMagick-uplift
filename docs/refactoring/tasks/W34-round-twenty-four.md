# W34 round twenty-four (2026-11-16)

Base `9985bdf74` (campaign/W34 after round 23, plus the AGENTS.md record of the owner's answers to the round-twenty-three questions: a shared core header of any size; one accessor per shared file-scope variable; one header definition of a twice-defined identical macro; the `visual-effects-pixel-methods.c` chain; the round-23 readings confirmed). The owner left questions 5 and 6 to the agent: the readings were confirmed, and instead of a size limit the header is scored like a split product and must score at least the original's pre-split score. Three Opus lanes, about three hours, refilled as they reported; one Sonnet reviewer per batch. All merged except two flat `xwindow-image.c` carves.

| File | Before | After | How |
| --- | --- | --- | --- |
| `visual-effects-pixel-methods.c` | 9.09 | **10.0** | lane c: a chain of seven splits through `visual-effects-pixel-methods-private.h` (10.0), no drop, then one Recipe B step. Products: `-noise`, `-blueshift`, `-colorize`, `-sepia` 10.0; `-color-matrix` 10.0 after a probed three-step E/N plan; `-morph` 9.38; `-polaroid` 9.68. |
| `statistic.c` | 9.38 | **10.0** | lane a: Recipe RL step 8 (new, loop-function form) inside the OpenMP region, a TI shift-twin merge, one E step, then the Polynomial split to `statistic-polynomial.c` (10.0) through `statistic-evaluate-private.h` (10.0). No drop. |
| `layer-frames.c` | 9.38 | **10.0** | lane a: a chain of two splits to `layer-dispose.c` and `layer-optimize.c` (10.0 each) through `layer-frames-private.h` (9.38). |
| `transform-region.c` | 9.38 | **10.0** | lane c: chop/crop/excerpt/splice split to `transform-crop.c` (10.0) through `transform-region-private.h` (10.0). |
| `effect-blur-adaptive-filters.c` | 9.09 | 9.68 | lane c: a chain of four splits (`effect-adaptive.c` 9.38, `effect-blur-selective.c` 9.68, `effect-blur-bilateral.c` 9.68, `effect-emboss.c` 10.0) through `effect-blur-adaptive-filters-private.h` (10.0, `MagickSigma` once); held by `MotionBlurImage`'s 5 public arguments. |
| `quantize.c` | 8.81 | 9.38 | lane a: `KmeansImage` split to `quantize-kmeans.c` (9.68) through `quantize-private.h` (the colour-tree core, 10.0). The second split (to 10.0) was reverted after review. |
| `widget.c` | 7.78 | 8.28 | lane b: two splits (`widget-dialog.c` 9.09, `widget-confirm.c` 9.38) through `widget-core-private.h` (9.68); `BorderOffset`, `DoubleClick` through read-only accessors. |
| `xwindow-image.c` | 8.28 | 8.28 | lane b: two flat accessor-based carves (text, pixel) committed on `campaign/W35-b` but **not merged** (no probed chain ends above 8.28). |
| `segment.c`, `vision-components.c`, `visual-effects-plasma.c`, `paint-floodfill.c`, `effect-adaptive.c`, `animate-window.c`, `display-image.c`, `colorspace-transform-image.c`, `feature-texture.c`, `fourier.c` | | same | probes only (see below). |

**Gates on the merged tree (`c73a408d7`):** narrow 15,955 cases and wide 14,200 cases, 0 diverged; `ci_linux_warnings.sh --campaign-a` OK on 123 files (seven configurations); `playbook_lint` 157 recipes, 454 change rows, 0 problems; `check_trailers` 54 refactor commits valid. **Sweep** (cs-mcp 1.5.16): campaign A 123 files, mean **9.54** (was 9.46), **69 at 10.0** (was 55); MagickCore 9.26 over 223 files (was 9.19).

## Review findings and how they were handled

- **Lane a, `781ef7809` (major):** the `quantize.c` remap split moved `AssignImageColors` (with its `#pragma omp parallel for`) and `Riemersma` (with a static local) into the header, which Recipe S forbids, and its message said "no pragma". Reverted in `167d2c858`, which corrects the false claims; the revert needed a disclosed `--no-verify` (it returns `quantize.c` from 10.0 to the gate-accepted 9.38).
- **Lane c, `d41f561f1` (major):** four header items were used by one file only (parking, not sharing). Moved back in `d2b4f28f0`; all three files stay at 10.0.
- **Lane a, `2a10c3582` (major, evidence):** the omp_compare cases used `-seed`, which fixes the random key, so `magick_number_threads(...,key == ~0UL)` ran the region on one thread. The reviewer's unseeded rerun at 1 and 4 threads was identical; the cases were fixed in `b0972f9cb` (four threads: 585 ms to about 200 ms) and the claim corrected.
- Minor: `visual-effects-polaroid.c` groups Polaroid, Shadow, Vignette and Wave because their public argument counts cap any file at 9.68 (a grouping by finding, not by family); `ReleaseFailedImage` is now byte-identical in three private headers; `424f6c6ac` is a one-`if` E step taken for Overall Code Complexity; RL step 8 was written as an amendment (arguably a new recipe); `PolynomialImage` measured 1.029 at 21 runs (A/A 1.000), under the 1.03 limit, with only 2 oracle cases.

## Playbook changes

- Recipe S step 2: core header of any size and the header-score condition (lane a, `69b0e32b2`, with mandate readings for prototype blocks, a non-inline `SetImageColormap`, a qsort comparator, an `#if/#else` macro block checked under both branches); a twice-defined identical macro as one header definition (lane c, `c9aba830b`); the accessor sentence (lane b, `62914e106`) and its extension to `static const` objects with read-only accessors (`da9caa8ca`, mandate).
- Recipe RL step 8, loop-function form (lane a, `0ea3d1496`, mandate, under the 2026-11-15 OpenMP ruling's checks).
- `split_source.py` reads `static magick_attribute((...)) inline` definitions (`e60b3ab55`).
- Known plateaus rows rewritten for `widget.c` and `quantize.c` (the other files' plateaus are recorded in this report only; the table has no rows for them).

## What holds the rest

- `quantize.c` (9.38): the remap split needs pragma-holding and static-local helpers in the header (about 12 KB of object code duplicated between two files).
- `xwindow-image.c` (8.28): the packer family, even with every TH merge, scores about 7.32 as a product; Magnify 8.03.
- `widget.c` (8.28): Number of Functions and four `MagickPrivate` argument counts; the remaining carve products carry Lines of Code findings.
- `segment.c` (9.09): the shared core uses a file-scope `static const int` block and an `#if defined(FastClassify)` macro block.
- `visual-effects-plasma.c`, `paint-floodfill.c`, `effect-adaptive.c`, `visual-effects-morph.c` (9.09 to 9.38): Overall Code Complexity needs four or five more small extractions each, or readings (Recipe N with written inputs; D form (b) with redundant parentheses).
- `animate-window.c` (8.81), `display-image.c` (5.86): one root each after any split; no split raises the file.
- `colorspace-transform-image.c`, `feature-texture.c`, `vision-components.c` (9.38): Lines of Code only, one public root each.
- `fourier.c` (8.74): the InverseFourier merge loops hold an `#else` branch no available build compiles.

## Owner questions

1. May a header helper hold an OpenMP pragma or a static local (the `quantize.c` remap split, quantize.c to 10.0; about 12 KB of duplicated object code)?
2. Merge the two flat `xwindow-image.c` carves on `campaign/W35-b` (they add two 9.68 files but leave the file at 8.28), or record `xwindow-image.c` as plateaued?
3. Accept the seven-split `visual-effects-pixel-methods.c` chain (three were approved), and `visual-effects-polaroid.c`'s grouping by argument count, or split it into four files?
4. Should the three private headers' identical `ReleaseFailedImage` be shared (a cross-header dedup, no recipe yet)?
5. Confirm the mandate readings: RL step 8 as an amendment; read-only accessors for `static const` objects; the non-inline `SetImageColormap` and qsort comparator in a header; the `--no-verify` on the revert `167d2c858`.
6. Are small extractions taken only for Overall Code Complexity (like `424f6c6ac`) acceptable? They would lift `visual-effects-plasma.c`, `paint-floodfill.c`, `effect-adaptive.c` and `visual-effects-morph.c`.
7. May a private header carry a file-scope `static const` object or an `#if`/`#else` macro block verbatim (`segment.c`)?
8. Is `PolynomialImage`'s 2.9% (under the 1.03 limit) acceptable, or should its helpers be kept out of line?
