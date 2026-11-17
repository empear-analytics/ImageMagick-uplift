# W34 round twenty-three (2026-11-16)

Base `dc3d0add8` (campaign/W34 = main `e25aff32c` after PR #120, plus the AGENTS.md record of the owner's answers to the round-twenty-two questions: several shared statics, a typedef and macros through one private header; chains of splits; `xwindow-image.c` continues). Three Opus lanes, one Sonnet reviewer per batch. All lanes merged.

| File | Before | After | How |
| --- | --- | --- | --- |
| `threshold-image.c` | 9.09 | **10.0** | lane a: Recipe S (amended for several shared statics, `d3fb98ec4`) splits the `ThresholdImageRows` family to `threshold-level.c` (9.68) through `threshold-private.h` (10.0). |
| `enhance-tone.c` | 9.09 | **10.0** | lane a: split to `enhance-level.c` (9.68) through `enhance-private.h`, a chained second split to `enhance-equalize.c` (10.0 after one Recipe B step), then one Recipe E step. |
| `effect-despeckle.c` | 9.38 | **10.0** | lane a: two Recipe CF twin merges (`HullRow`, `HullEdgeRow`), object code identical. |
| `enhance-color.c` | 9.09 | 9.68 | lane a: `WhiteBalanceImage` split to `enhance-balance.c` (10.0) through `enhance-color-private.h`; held by `SigmoidalContrastImage`'s 5 arguments. |
| `effect-sharpen.c` | 9.09 | 9.68 | lane a: five-step E/PS/D plan (four flat, probed); held by the `ShadeImage` and `UnsharpMaskImage` argument counts. |
| `feature.c` | 9.09 | 9.68 | lane c: `HoughLineImage` split to `feature-hough.c` (9.68 after three E steps) through `feature-private.h`; held by `MagickExport` argument counts. |
| `montage.c` | 9.12 | 9.38 | lane c: five-step N/E/FB chain, one step (`0c7a9692d`, 9.12 to 8.57) with disclosed `--no-verify`; held by Lines of Code only. |
| `xwindow-image.c` | 8.28 | 8.28 | lane b: round 22's work redone as new Recipe TH (twin merge checked by a display-less harness), five timed twin merges (function count 246 to 241), all flat, no slowdown (median ratios 0.96 to 1.02). |
| `quantize.c`, `widget.c`, `display-image.c` | 8.81, 7.78, 5.86 | same | lane c probes: no legal step (see below). |
| `statistic.c`, `effect-blur-adaptive-filters.c`, `visual-effects-pixel-methods.c`, `colorspace-transform-image.c`, `layer-frames.c`, `animate-window.c` | | same | lane a probes: no step taken (see below). |

**Gates on the merged tree (`0b807dde5`):** narrow 15,955 cases and wide 14,200 cases, 0 diverged; `ci_linux_warnings.sh --campaign-a` OK on 105 files (seven configurations); `playbook_lint` 157 recipes, 448 change rows, 0 problems; `check_trailers` 27 refactor commits valid; the `-DNDEBUG` scan output is unchanged from round 22. **Sweep** (cs-mcp 1.5.16): campaign A 105 files, mean **9.46** (was 9.40), **55 at 10.0** (was 50); MagickCore 9.19 over 205 files (was 9.16).

## Playbook changes

- Recipe S step 2 amended (lane a, `d3fb98ec4`): several helpers, a typedef and file-scope macros in one header (owner); a macro each part defines for itself, byte-identical, is neither shared nor copied (the lane's reading, mandate). `split_source.py` accepts the latter (`66ad5950a`).
- Recipe FB step 3 (lane c, `fd35660f8`): the one call may be `return(F(...,&name,...));` (mandate).
- Recipes PS and D form (b) (lane a, `0a2fd9dd9`): PS for a Recipe T-shaped block whose helper a D step of the same plan reuses; D form (b) against the file's Overall Code Complexity within a probed plan. Both were written after their first use (`83684530a`, `34f5fb29e`), as the reviewer asked; the text says so.
- New Recipe TH (lane b, `dd9d67068`) with amendments `199c17185` and `b42c4739a` (a timing ratio above 1.03 is re-measured with 21 runs beside an A/A run). Tools: `xpack_compare.sh cover`, `twin_fold_check.py`.
- Known plateaus rows rewritten for `widget.c` (was 2.26), `quantize.c`, `montage.c`, `feature.c`, `enhance-tone.c`.

## Reviewer notes

All batches ok, no majors.

- Lane c: the `endbr64` difference in the gcc object of `SetFeatureProgress` once it is `static inline` is benign (static, called directly).
- Lane c, montage: `0c7a9692d`'s `--no-verify` sits in a five-step chain, while the long-flat-chain ruling speaks of about 30 to 55 steps. Taken under that ruling's conditions (stub probe first) and the owner's 2026-10-29 acceptance of temporary drops serving CH 10; for the owner to confirm.
- Lane a: the self-defined-tag reading moves only lines the original had, so nothing is copied.
- Lane b: `always_inline` is written through `magick_attribute` (precedent in `resize-image.c`); `xwindow-image.c` compiles only under `MAGICKCORE_X11_DELEGATE`, so the MSVC C4244 caveat does not arise. `dd9d67068`'s message says 40 cases for `XMakeImageLSBFirst` where casemap says 512 (corrected in `fcde05cb5`). Timings were taken with three lanes loading the machine (A/A ratios up to 1.02).

## What holds the rest

- `quantize.c` (8.81): 126 functions; every split shares the colour-tree core (about 33 statics, two typedefs, the cache macros).
- `widget.c` (7.78): Number of Functions (368) and five `MagickPrivate` argument counts; four file-scope variables tie the families; the only cut shares 34 helpers and its new file scores 7.32.
- `display-image.c` (5.86): Number of Functions, Overall Code Complexity, Lines of Declarations, `XDisplayImage` (Large Method, 7 public arguments). Cutting it to 68 lines would give 6.11 but needs a recipe for a written local array whose `sizeof` and address a context holds; judged too contrived. A flat split of `XDisplayBackgroundImage` to a new 10.0 file was not taken (it raises the mean without raising the file).
- `xwindow-image.c` (8.28): each remaining packer merge adds ternaries that switch on Overall Code Complexity (blue-high probe 7.78); a carve is tied by the file-scope gamma variables and `xerror_alert`.
- `statistic.c` (9.38): the Polynomial split needs 10 shared helpers and drops the kept file to 8.81.
- `effect-blur-adaptive-filters.c` (9.09): `MagickSigma` is defined twice (identically) inside the Adaptive family and used by others.
- `visual-effects-pixel-methods.c` (9.09): a three-split chain probed (kept 9.09, products 10.0, 9.68, 9.68) with intermediate drops to 8.54; not started.
- `layer-frames.c` (9.38): a second cut needs about 16 functions and 4 typedefs in the header (which would score 9.38).
- `animate-window.c` (8.81): two roots; the split leaves 113 functions; `SceneCompare`'s address goes to `qsort`.

## Owner questions

1. May a split share a core of about 30 or more helpers (`quantize.c`'s colour tree) through a private header?
2. May split products share a file-scope variable (`xwindow-image.c`'s gamma variables, `xerror_alert`) through one accessor in the private header?
3. May a split move a twice-defined identical macro (`MagickSigma`) into the header as one definition?
4. Run `visual-effects-pixel-methods.c`'s three-split chain with disclosed intermediate drops?
5. Confirm the lane-written readings: the self-defined tag (S), FB's `return` form, the PS and D(b) amendments, Recipe TH and its timing rule; and the five-step `--no-verify` in `montage.c`.
6. Is a size limit wanted for a shared header?
