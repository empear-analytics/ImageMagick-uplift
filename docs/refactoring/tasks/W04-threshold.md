# W04 - threshold.c leaves the Red band

Not a generated task file. Campaign A (Windows desktop, WSL2), branch `campaign/W04`,
stacked on `campaign/W03` (PR #62); the oracle's baseline is `main` at `c74bb179e`.
Measured with `cs-mcp` 1.5.16.

### Report

**threshold.c 3.28 → 4.01.** One recipe, one function per commit.

| Function | Step | Recipe | Cases | Code Health |
| --- | --- | --- | ---: | --- |
| BlackThresholdImage, WhiteThresholdImage | `GetThresholdLevels`: the 26-line parse of the thresholds argument, identical in both | D | 16 + 16 | 3.28 → 3.38 |
| BlackThresholdImage | `BlackThresholdPixel`: the channel loop | E | 16 | 3.38 → 3.50 |
| WhiteThresholdImage | `WhiteThresholdPixel`: the channel loop | E | 16 | 3.50 → 3.63 |
| BilevelImage | `BilevelPixel`: the channel loop | E | 182 | 3.63 → 3.67 |
| ColorThresholdImage | `IsPixelInColorRange`: the channel loop, returning `foreground` | E | 24 | 3.67 → 3.79 |
| PerceptibleImage | `PerceptiblePixel`: the channel loop | E | 7 | 3.79 → 3.90 |
| ClampImage | `ClampImageColormap`: the PseudoClass branch (step 7) | E | 128 | 3.90 → 3.93 |
| OrderedDitherImage | `GetDitherLevels`: the levels parse | E | 77 | 3.93 → 4.01 |

The channel loops sit in parallel row loops. Each writes only through `q` and variables
declared in the loop body (Recipe E's OpenMP rule); a variable such as `pixel` that the
loop updates across channels goes by value, so its updates stay inside the moved loop.
Black and White's channel loops differ in two places (the comparison and the value
written), so they are two extractions, not a Recipe D. The whole catalogue: 15,900
cases, 0 diverged.

### Noticed, not changed

- `BlackThresholdImage` converts a gray image with `SetImageColorspace`,
  `WhiteThresholdImage` with `TransformImageColorspace`. One of them is probably not
  what upstream meant.

### Left

- `RandomThresholdImage`'s channel loop needs the thread's `random_info[id]` as well:
  five parameters.
- `OrderedDitherImage`'s channel loop needs `map`, `levels`, `x` and `y`: six.
- `GetThresholdMapFile` (cc 25, 169 lines) and `RangeThresholdImage`'s helpers are not
  yet looked at.
