# W03 - campaign A's red files, first night

Not a generated task file. Campaign A (Windows desktop, WSL2), branch `campaign/W03`,
stacked on `campaign/W02` (PR #61); the oracle's baseline is `main` at `c74bb179e`.
Measured with `cs-mcp` 1.5.16.

### Report

**feature.c 3.90 → 4.00** (leaves the Red band), **vision.c 2.66 → 3.05**. One recipe,
one function per commit, except where the table says otherwise.

| File | Function | Recipe | Cases | Code Health |
| --- | --- | --- | ---: | --- |
| feature.c | GetHoughMaxima and WriteHoughHeader (`HoughCell`, `HoughWindow`): one commit under rule 1's exception, each alone 3.90 → 3.89 | B | 21 | 3.90 → 3.99 |
| feature.c | VoteHoughLines (`HoughCell`): leaves Excess Arguments | B | 19 | 3.99 → 3.99 |
| feature.c | TraceEdges: `IsUntracedEdgePixel` | P | 12 | 3.99 → 4.00 |
| vision.c | ConnectedComponentsImage: `MergeObjectsOutsideRange`, six threshold blocks | D | 59 | 2.66 → 2.73 |
| vision.c | ConnectedComponentsImage: `SetObjectMergeByIds`, keep-ids and remove-ids | D | 59 | 2.73 → 2.83 |
| vision.c | ConnectedComponentsImage: `SetObjectMergeByColor`, keep-colors and remove-colors | D | 59 | 2.83 → 2.86 |
| vision.c | ConnectedComponentsImage: `SetEquivalencePathRoot`, the union step's two path loops | D | 59 | 2.86 → 2.87 |
| vision.c | ConnectedComponentsImage: `GetObjectSortKey` | E | 59 | 2.87 → 2.91 |
| vision.c | ConnectedComponentsImage: `GetObjectSortOrder` | E | 59 | 2.91 → 2.99 |
| vision.c | ConnectedComponentsImage: `GetGreatestAdjacentObject` | E | 59 (31) | 2.99 → 3.00 |
| vision.c | ConnectedComponentsImage: `ResetObjectStatistics` | E | 59 | 3.00 → 3.01 |
| vision.c | ConnectedComponentsImage: `NormalizeObjectStatistics` | E | 59 | 3.01 → 3.02 |
| vision.c | ConnectedComponentsImage: `PrintObjectsHeader` | E | 59 | 3.02 → 3.04 |
| vision.c | ConnectedComponentsImage: `PrintObject` | E | 59 | 3.04 → 3.05 |

`ConnectedComponentsImage` went from cc 202, 46 bumps and 813 lines to cc 142, 27 bumps
and 659 lines. Every Recipe D duplicate was checked by script, token for token once
comments are stripped and the one differing value (or the renamed variable) is
substituted. The whole catalogue: 15,900 cases, 0 diverged.

### Tried and refused

- **effect.c `Hull`** (8 arguments, Brain Method): its calls pass `image->columns` and
  `image->rows` with `image`, `pixels` and `buffer` at sixteen calls; Recipe B step 4
  allows member accesses only in a struct for one call.
- **effect.c `AdaptiveBlurImage` / `AdaptiveSharpenImage`**: their channel loops are
  identical up to renaming, but one helper for both (Recipe D, eight parameters)
  becomes a Brain Method itself: 1.59 → 1.57. Smaller steps inside the loop first.
- **feature.c `MeanShiftImage`**: the window loop assigns the shared `status` from
  `GetOneCacheViewVirtualPixelInfo(...)` on every pixel, which Recipes J and T refuse.
  **Possible upstream bug, not fixed:** inside the parallel loop each lookup overwrites
  `status`, so an earlier failure can be masked by a later success.
- **feature.c `GetCannyNeighbors`** (6 arguments): bundling `(x, y)` leaves five; the
  sixth would need `pixel.orientation`, filled member by member, and a non-const struct
  may not be read in an OpenMP region (Recipe B, *Careful*).
- **enhance.c**: bundling the arguments of `LevelImageRow`, `LevelizeImageRow` and
  `SigmoidalContrastImageRow` is flat (2.90): five public functions keep *Excess Number
  of Function Arguments* in the file whatever the static ones do. Not committed.
  `visual-effects.c` has the same situation (seven public functions).
- **vision.c's moment thresholds**: the second-moment pass tests `status` at each row,
  which Recipe J refuses; the four functions stay duplicates.
- **shear.c `ShearRotateImage`**: see W02 (the seven-function cliff).

### Notes for the owner

- **FFTW is not compiled in this build either** (`FFTW_DELEGATE` undefined in
  `build-oracle/cand`), though CAMPAIGNS.md expected Linux to bring `fourier.c` and
  `compare.c`'s FFT functions into reach. Installing `libfftw3-dev` and rebuilding the
  oracle baselines would; that is an infrastructure change left to the owner.
- No case sets `connected-components:exclude-ids` to true; `PrintObject` moved that
  branch unchanged, unrun.
- Rule 1's exception was used once (HoughLineImage's helpers).
