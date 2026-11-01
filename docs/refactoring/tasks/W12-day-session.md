# W12 - campaign A, day session (2026-11-01)

Not a generated task file. Campaign A (Windows desktop, WSL2), branch `campaign/W12`, cut
from `campaign/W11` (#85) after `main` was merged into it twice (#83, #84). Measured with
`cs-mcp` 1.5.16. The owner marked #85 for merge and said "You can continue with the other
splits you want. that's fine." (2026-11-01).

### Report

| File | Code Health | Commits |
| --- | --- | ---: |
| `statistic.c` | 5.84 → 6.55 (split) → **6.73** | 1 + 8 |
| `statistic-measure.c` (new) | 9.09 → **9.68** | 1 |

Ten refactor commits (the split, eight on `statistic.c`, one on `statistic-measure.c`). Every
step ran the oracle on its function's cases (0 diverged); the whole catalogue ran after
each file: 15,900 cases, 0 diverged.

### Splits examined (`tools/split_source.py --out`, both halves scored)

- **`statistic.c`: done** (0129266cc). The measurement family (eight public functions, 35
  functions, 1,211 lines) is `statistic-measure.c`: `statistic.c` 5.84 to 6.55, the new file
  9.09, guards over the pair unchanged (256 literals, 452 call sites), the eight public
  functions the only exported symbols of the new object.
- **`transform.c`**: the trim, consolidate, auto-orient, transform, extent and shave
  families (684 lines) would go to a new file at 9.38, but `transform.c` stays at 5.6: no gain.
- **`quantize.c`**: `GetImageQuantizeError` (181 lines, new file 10.0) is the only family
  the tool accepts (`MaxTreeDepth` and `QCubeInfo` cross every other cut); it leaves
  `quantize.c` at 5.6 and takes three functions out of the 110.
- **`threshold.c`** (`ThresholdsFilename`, `ThresholdImageTag`) and **`feature.c`**
  (`CannyEdgeImageTag`, which `HoughLineImage` uses too): a file-level macro crosses every
  cut that has a useful balance, which Recipe S refuses.
- **`segment.c`, `vision.c`, `fourier.c`, `layer.c`, `resize.c`**: one family holds nearly
  the whole file, or the rest is too small to matter.

### For the owner

- **Confirm or revert the `statistic.c` split**, as for `effect.c` and `enhance.c` (#85). The
  Windows build configuration outside this repository still needs `statistic-measure.c`.
- **Close readings:** `statistic.c` commits 3, 4, 5 and 7 pass the progress macro's tag as a
  `const char *` so one `SetProgress` helper serves three functions (T plus D's single
  differing value), and reuse a helper an earlier T commit created (T has no text for a
  second caller); the second `I` step (d60dde784) calls the helper the first created for an
  identical block.
- **Recipe-text gaps found:** a `continue` block mid-body in a parallel loop (`p == NULL` or
  a failed fetch, then `status=MagickFalse;`) fits no recipe: T, LB, OC and W each refuse
  it, and it is what holds `GetImageRange` at 9.68 and `EvaluateImages` at cc 27.

### Correction made before pushing

- Commit f190a8c9b's message gave "72 -> 69 lines" for `EvaluateImage`, numbers inferred
  from a probe, not measured; `code_health_review` lists the function under *Large Method*
  before the commit and not after. The message was rewritten to say that; the tree is
  unchanged.
