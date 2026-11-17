# W32 round twenty-one (2026-11-16): re-carve split products and flat chains

Base `fd5c82c66` (main after #119, plus the AGENTS.md rulings of 2026-11-16). Three Opus lanes, one Sonnet reviewer for the lane with commits. Owner rulings in force: a Recipe S split product may be carved again; long flat chains may be committed in a dedicated lane with disclosed `--no-verify` after a stub probe of the end score.

| File | Before | After | How |
| --- | --- | --- | --- |
| `distort.c` | 7.90 | **8.40** | 30 Recipe E extractions (lane c), flat at 7.90 for 29 commits, the 30th (`a345c4ecf`, `GetDistortViewport`) clears Overall Code Complexity. The gate refused none, so no `--no-verify`. Seven commits are `Verified: guard` (the moved lines run in no case); `a345c4ecf` moves a throw, so its `GetMagickModule()` names the helper (ruling of 2026-11-12). |
| `morphology-apply.c` | 7.41 | 7.41 | not started: the pad probe needs 51 to 54 steps (50 pads stay 7.41, 54 reach 7.88) and the lane ran out of time. |
| `threshold-image.c`, `enhance-tone.c`, `effect-filter.c` | 9.09 | 9.09 | lane a: held by Lines of Code only. `split_source.py` refuses every root: each file has one small static (`SetThresholdProgress`, `SetToneProgress`, `ReleaseFailedImage`) that every public root calls, so every partition leaves it on both sides. Recipe S step 2 forbids widening a static and the 2026-11-07 ruling forbids copying one. |
| `layer-frames.c`, `colorspace-transform-image.c` | 9.38 | 9.38 | lane b: `layer-frames.c` is one closed 73-function component (all 62 root subsets refused); `colorspace-transform-image.c` has one public root, so nothing to carve. |
| `animate-window.c` | 8.81 | 8.81 | lane b: both roots share statics (136 functions, one component); a stub probe that removed 26 to 66 small statics scores 8.54 (Lines of Code and Overall Code Complexity replace Number of Functions). |
| `xwindow-image.c` | 8.28 | 8.28 | lane b: the cheapest legal cut (146 functions) scores 7.10 and 6.89 (Code Duplication in 57 functions, the LSB/MSB `XPack*` twins); only the Draw family (8.54) passes the gate, and the kept file stays flat. |

Gate on the tip (lane c's `a345c4ecf`, same tree as the merge): narrow catalogue 15,955 cases, 0 diverged; wide 14,200, 0 diverged; `ci_linux_warnings.sh --campaign-a` OK on 98 files; `check_trailers` 30 commits valid. Lanes a and b made no commits.

**Reviewer notes (all minor).** `c6a9388fc`'s message says `SetPolarRadii` holds the `coeff[1]` ternary; the diff moves only the `Rmax` if/else and a trailing comment (behavior unaffected, message wrong). All 30 messages say "Step N of 32" although the chain ended at step 30 (steps 31 and 32 were committed and reset away: they came after the raising step). Messages cite the 2026-11-16 ruling although the gate refused no step.

**What is left in `distort.c` (8.40):** `poly_basis_dx` Complex Method (cc 17, split measured 5% to 20% slower), argument counts on `DistortImage` (6) and `SparseColorImage` (5) (MagickExport), Number of Functions (156; not a split product, shared statics). The margin is thin: mean cc about 3.87 against a threshold near 3.88. Helpers that trip Code Duplication and must not be taken: `PrintPolarFX`, `PrintPlane2CylinderFX`, the Affine/Perspective FX tails, `PrintPerspectiveProjection` with `PrintAffineProjection`, `PrintPolynomialYTerms`.

**Owner questions.** (1) May a Recipe S split share one small static helper between the two files, by widening it to `MagickPrivate` (the open question from #112) or by one `static inline` definition in a new private header? That would relax the 2026-11-07 ruling ("never by copying static helpers"), and would unlock `threshold-image.c` (stub: 9.68 each half, the half without `RangeThresholdImage` 10.0), `enhance-tone.c` and `effect-filter.c` (cuts near `EqualizeImage` and `KuwaharaImage`). Ceiling 9.68 for a file that keeps a MagickExport function of 5 or more arguments, 10.0 otherwise. (2) A timed merge chain for the `XPack*` LSB/MSB twins (Recipe MT or a new recipe) before `xwindow-image.c` can be opened. (3) A longer lane for the 51 to 54 steps of `morphology-apply.c` (+0.47).

**Sweep on the merged tree (`cs-mcp` 1.5.16, Linux):** campaign A 98 files, mean 9.38 (distort alone adds about 0.005), 49 at 10.0; MagickCore 9.15 over 198 files (6 red, 44 yellow, 58 green, 90 optimal).
