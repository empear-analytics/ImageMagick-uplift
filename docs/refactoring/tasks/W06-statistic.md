# W06 - statistic.c

Not a generated task file. Campaign A (Windows desktop, WSL2), branch `campaign/W06`,
stacked on `campaign/W05` (PR #68); the oracle's baseline is `main` at `c74bb179e`.
Measured with `cs-mcp` 1.5.16.

### Report

**statistic.c 3.23 → 3.56.** One recipe, one function per commit.

| Function | Step | Recipe | Cases | Code Health |
| --- | --- | --- | ---: | --- |
| GetImageStatistics | `ComputeCompositeStatistics`: the composite channel's averages | E | 416 | 3.23 → 3.30 |
| EvaluateImages | `SetEvaluatePixels`: the non-median branch's write-back | E | 46 | 3.30 → 3.33 |
| EvaluateImages | `SetEvaluateMedianPixel`: the median branch's write-back | E | 46 | 3.33 → 3.35 |
| EvaluateImages | the zeroing loop now calls `ClearPolynomialPixels`, its exact copy | D | 46 | 3.35 → 3.36 |
| GetMedianPixel | `SwapPixels` hoisted above the function (`pp_identical`: IDENTICAL) | H | - | 3.36 → 3.36 |
| GetMedianPixel | `SetMedianPivot`: the median-of-three pivot | E | 416 | 3.36 → 3.38 |
| GetImagePerceptualHash | `SetPerceptualHashes`: one colorspace's hashes from the moments | E | 32 | 3.38 → 3.45 |
| EvaluateImages | `ScaleEvaluateProduct`: the Multiply case's rescaling | E | 46 | 3.45 → 3.46 |
| EvaluateImages | `NormalizeEvaluatePixels`: the Mean, Multiply and RMS final step | E | 46 | 3.46 → 3.56 |

Every block moved out of `EvaluateImages` sits in a parallel row loop and writes only
through `q`, the thread's `evaluate_pixel` and counters declared in the loop body.
`ScaleEvaluateProduct` comes first so that `NormalizeEvaluatePixels` stays under the
Complex Method threshold (cc 9 otherwise); each new helper was checked for findings of
its own, which CodeScene's PR gate rejects. The whole catalogue: 15,900 cases, 0
diverged.

### Tried and refused

- **`GetImageStatistics`' entropy loop** (`ComputeChannelEntropy`, 3.30 → 3.38): the
  helper has a Bumpy Road of its own. Moving only the loop body would turn its leading
  `continue` into a `return`, which no recipe covers; moving only the inner loop takes
  six parameters.
- **`ResetEvaluatePixels`** (3.33 → 3.26): a copy of `ClearPolynomialPixels`, which
  the Recipe D step above reuses instead.
- **The statistic wrappers' shared prefix** (`AcquireImageStatistics`, Recipe D over
  `GetImageEntropy`, `GetImageKurtosis`, `GetImageMean` and `GetImageMedian`): flat, and
  the Code Duplication finding stays, now naming the new helper.
- **`GetImageStatistics`' median block** writes `y`, which the final `y < rows` test
  reads; it stays.

### Noticed, not changed

- `GetImageStatistics` frees its result when `y < image->rows` after the median pass,
  whose own row loop reassigns `y` once per channel. The test therefore reflects the
  last median loop, not the statistics pass it seems to guard.
- `ClearPolynomialPixels` is now also EvaluateImages' helper; its name was kept, since a
  rename is outside the recipes.
