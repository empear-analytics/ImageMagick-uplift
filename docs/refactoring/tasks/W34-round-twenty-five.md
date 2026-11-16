# W34 round twenty-five (2026-11-16)

Base `1fad58737` (campaign/W34 after round 24, the AGENTS.md record `86e49745b` of the owner's answers to the round-twenty-four questions 1 to 7, and the two flat `xwindow-image.c` carves merged under them). Question 8 (`PolynomialImage`'s 2.9%, under the 1.03 limit) was left unanswered and stays as measured. Three Opus lanes, about three hours, refilled as they reported; one Sonnet reviewer per batch (lane a's two-step follow-up was read by the main session). All lane work merged.

| File | Before | After | How |
| --- | --- | --- | --- |
| `quantize.c` | 9.38 | **10.0** | lane a: the remap split redone under the pragma ruling; the assign/dither family in a second header `quantize-assign-private.h` (9.38, then 10.0 after two E steps) included only by `quantize.c` and the new `quantize-remap.c` (10.0). |
| `visual-effects-morph.c` | 9.38 | **10.0** | lane c: one E step. |
| `effect-adaptive.c` | 9.38 | **10.0** | lane c: three E steps (probed plan). |
| `widget-confirm.c` | 9.38 | **10.0** | lane b: three E steps. |
| `segment.c` | 9.09 | 9.68 | lane a: split to `segment-threshold.c` (9.68) through `segment-private.h` (10.0, with the `static const int` block); held by `SegmentImage`'s and `GetImageDynamicThreshold`'s public argument counts. |
| `visual-effects-plasma.c`, `paint-floodfill.c` | 9.09 | 9.68 | lane c: three E steps each; held by public argument counts. |
| `widget-dialog.c` | 9.09 | 9.68 | lane b: five E steps; held by `XDialogWidget`'s 5 arguments. |
| `fourier.c` | 8.74 | 9.31 | lane c: three E steps (object code identical, in callees of the `omp section`s). Left: Lines of Code; `InverseFourier`'s Bumpy Road (`#if`/`#else` merge loops). |
| `xwindow-image.c` | 8.28 | 8.28 | lane b: two more flat carves, `xwindow-info.c` (8.81, then 9.68) and `xwindow-annotate.c` (8.54, then 9.09). Every further carve lowers the kept file (to 7.10 or 7.32): plateau. |
| `xwindow-import.c` | 9.38 | 9.38 | lane c: flat carve of `XWindowByName`/`XWindowByProperty` to `xwindow-search.c` (10.0). |
| effect headers | | | lane a: new Recipe CH puts `ReleaseFailedImage` in `effect-common-private.h`; the 16 including files compile to byte-identical objects. |

**Gates on the merged tree (`3acaa2521`):** narrow 15,955 cases and wide 14,200 cases, 0 diverged; `ci_linux_warnings.sh --campaign-a` OK on 130 files (seven configurations); `playbook_lint` 158 recipes, 458 change rows, 0 problems; `check_trailers` 90 refactor commits valid. `quantize.c`'s remap: `omp_compare` 18 runs at 1 and 4 threads on unseeded 1024² to 2000² cases, 0 diverged, remap part 255 to 125 ms at four threads; dither and remap paths timed against the baseline (B/A 0.979 to 1.014). **Sweep** (cs-mcp 1.5.16): campaign A 130 files, mean **9.59** (was 9.54), **75 at 10.0** (was 69); MagickCore 9.30 over 230 files (was 9.26).

## Review notes

No majors in round 25.

- `quantize.c`: the split duplicates about 38 KB of object code (`quantize-remap.o` carries its own colour tree and assign family), not the 12 KB round 24 estimated. No cut avoids it: both products call `AssignImageColors`.
- Lane a's reading: Riemersma's `turns` is an automatic `const` object, not a static local, so round 24's revert message and review were wrong on that point (only the pragma needed the ruling).
- Recipe CH's change row said `Decided by: owner`; the recipe's wording and proof are the lane's, so it now says mandate (corrected in the report commit).
- Small E steps for Overall Code Complexity: several are one-`if` blocks (`9f7ddb235`, `293484cfb`, `652b9eb33`), inside the owner's ruling.
- `8114997cc` keeps `XDrawImage`'s per-pixel instructions; its stack frame shrinks by 0x80 and offsets move. `e09af9e7b`'s message says 40 cases for `XGetWindowInfo` where casemap says 512.
- `fourier.c`'s E steps cite the oracle but not `omp_compare`: the helpers sit in callees of the sections, no region boundary is crossed, and the object code is identical.
- Process: lane a once ran a script from the main session's scratchpad (it started a gate run in `~/im-campaign`); stopped within seconds, the clone checked clean. A reviewer briefly detached a lane's HEAD and restored it.

## Playbook changes

- Recipe S step 2: a header helper may hold an OpenMP pragma or a static local (owner, with the lane's checks: the region moves whole, write-set reading, real-thread `omp_compare`, timing; a writable static local refused); a later split may use a second family header `<stem>-<family>-private.h` (mandate, for the owner); a file-scope `static const` object moves into the header verbatim (owner).
- New Recipe CH: Share an Identical Header Helper Through a Common Private Header (proof: byte-identical objects of every including file).
- Known plateaus rows for `quantize.c` and `widget.c`.

## What holds the rest

- `xwindow-image.c` (8.28): Number of Functions and seven `MagickPrivate` argument counts; any further carve adds Code Duplication or Overall Code Complexity to the kept file.
- `xwindow-annotate.c` (9.09): Code Duplication in per-pixel twins; a TH merge needs a timing harness for annotate/draw.
- `opencl.c` (9.09): a split needs `extern MagickLibrary *openCL_library;` declared for the new file (the global already has external linkage).
- `morphology-apply.c` (8.26), `distort.c` (8.40): splits need headers of 20 to 70 shared statics and drop the kept file.
- `colorspace-transform-image.c`, `feature-texture.c`, `vision-components.c`, `montage.c` (9.38): Lines of Code with one public root.
- `fourier.c` (9.31): Lines of Code (36 shared statics); Bumpy Road in `#if`/`#else` merge loops.

## Owner questions

1. Is the about 38 KB of duplicated object code from the `quantize.c` split accepted?
2. Confirm the second family-header reading (`quantize-assign-private.h`, mandate).
3. Build a timing harness for annotate/draw so `xwindow-annotate.c`'s per-pixel twins can take Recipe TH?
4. May a split declare an existing external global (`openCL_library`) in a private header (`opencl.c`)?
5. Run a `morphology-apply.c` chain through a header of 20 or more statics, with a kept-file drop to 8.01 in between (products about 8.5)?
6. Record `xwindow-image.c` (8.28) as plateaued?
