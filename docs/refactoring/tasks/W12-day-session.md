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
| `resize.c` | 7.08 → **7.11** | 3 |
| `enhance.c` (again) | 4.85 → **4.91** | 1 |

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

- **A ruling that would open `resize.c` and the other `status`-assigning loops:** a recipe
  that moves a whole block of a parallel loop body into a helper taking `MagickBooleanType
  *status` and writing and reading it through the pointer (`*status=...`), instead of T's
  and AS's local copy. The reads and writes then happen at the same points, in the same
  order, to the same memory as inline, so the argument is the one the skip-guard probe
  used; what it needs from you is the OpenMP rule, which the playbook sets at E step 5 (a
  block moves only if every variable it writes is declared inside the loop body) and Q step
  1 (no OpenMP region at all). Measured: `InterpolativeResizeImage`'s whole channel loop as
  a helper gives `resize.c` 8.6 (7.11 now), not committed because it is not legal today;
  `feature.c`'s `MeanShiftImage` and `effect.c`'s `SpreadImage` and `KuwaharaImage` (an
  assign-and-test `status=Interpolate...` with a `break`) meet the same wall. The oracle
  runs single-threaded and cannot see a race, so every such step would be checked by
  reading, as T and AS are.

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

### The Recipe D amendment, first uses (one commit, three probes left out)

`enhance.c` f72f8fa8a, form (b): the progress block of `CLAHEImage`'s second row loop equals
`UpdateCLAHEProgress`'s body once `progress` and `status` are `(*progress)` and `(*status)`
(by script, token for token), so it becomes `UpdateCLAHEProgress(image,&progress,&status);`:
4.85 to 4.91, Bumpy Road gone, cc 16 to 14, 109 to 100 lines, 13 cases, 0 diverged. The W11
note that the block "cannot reuse the helper" applied to the loop extracted whole, where
`status` is a plain variable; in place the addresses exist. Not committed: `enhance-tone.c`'s
`GetGrayscaleIntensity` (the Rec709 block equals `EncodeGrayscaleGamma`'s body and the two
sRGB decode blocks are identical: 4.17 to 4.20, but cc 11 and 70 lines remain flagged, so no
finding goes and the amendment's condition fails); `effect-blur.c`'s `AdaptiveSharpenImage`
(`PrepareAdaptiveEdge` reuse: 5.29 to 5.30, no finding removed; its inner loop differs from
`BlurAdaptiveRow` in extra parentheses, equal in value, not the same text). A token-level
scan of seventeen files (vision to resize) found three further whole-body hits, none in a
function where a finding would go.

### `resize.c`: Recipe AS does not apply to `InterpolativeResizeImage`

Three small steps (T on the progress call, E step 7 on the tail, E step 3 on the scale
assignments: 7.08 to 7.11, 104 to 96 lines). AS step 2 fails on every candidate block: the
channel loop, the column loop and the body past the guard leave `status` alone when a loop
runs zero iterations (`GetPixelChannels(image) == 0`) or every channel hits the `continue`,
and the channel loop's `break` leaves the block (step 1); only `offset.x=...;
status=InterpolatePixelChannels(...)` meets step 2, needs a nine-variable struct and probes
7.11 to 7.09. **The lever is a pointer-status form for parallel loops** (below, for the owner).

### The skip-guard recipe, probed and not added

The owner allowed a recipe for the parallel loop's head, `if (status == MagickFalse)
continue;` followed by a fetch and `if (p == NULL) { status=MagickFalse; continue; }`,
which no recipe covers (T, LB, OC and W refuse it). The idea was one helper that reads and
writes the shared `status` through a pointer at the same points, in the same order, and
returns whether to skip (`if (SkipRow(&status,...) != MagickFalse) continue;`); the
ordering argument holds. It does not pay, measured with `tools/ch.py`:

| Probe | Before | After | Finding |
| --- | --- | --- | --- |
| `GetImageRange` (`statistic-measure.c`), 8-argument helper | cc 10, file 9.68 | cc gone, file 9.68 | the helper has *Excess Number of Function Arguments* (8); a 5-argument form is still flagged |
| `ClampImage` (`threshold.c`) | cc 11, file 5.35 | cc 9, file 5.37 | new Excess Arguments (8) |
| all nine row loops of `threshold.c`, one helper | file 5.35 | file 5.41 | every cc drops by 2, bumps stay 2, *Code Duplication* grows from 6 to 8 functions |
| `FlipImage` (`transform.c`), 9-argument helper | cc 12, file 5.60 | cc 9, file 5.52 | the file score drops |
| `ContrastImage`, six sites (`enhance.c`) | file 4.85 | file 4.97 | two new helper findings (one per fetch kind) |

Why: the helper needs the status pointer, the view, the row, the width, the output pointer
and the exception (five arguments at the least, with `x=0` and one row baked in), and
CodeScene flags a function over four; the bumps stay because the second bump of each loop
is its progress tail, not its head. A bundle struct of view, exception and status would get
under four arguments, at one new type per file. No recipe was added.

### Correction made before pushing

- Commit f190a8c9b's message gave "72 -> 69 lines" for `EvaluateImage`, numbers inferred
  from a probe, not measured; `code_health_review` lists the function under *Large Method*
  before the commit and not after. The message was rewritten to say that; the tree is
  unchanged.
