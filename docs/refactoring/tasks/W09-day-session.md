# W09 - campaign A, day session (2026-10-30)

Not a generated task file. Campaign A (Windows desktop, WSL2), branch `campaign/W09`,
stacked on W08 (#75); the oracle's baseline is `origin/main` at 727f2598d. Measured with
`cs-mcp` 1.5.16. The owner asked in the morning for the recipes W08's subagents had
described but not added; the day added them, then used the grown catalogue on 16 files.
Each file was worked by a subagent under one brief, one at a time; the session read every
new recipe and amendment, and every step that read a recipe closely, before pushing.

### Report

| File | Code Health |
| --- | --- |
| `layer.c` | 4.99 → **6.51** |
| `paint.c` | 5.10 → **6.10** |
| `fourier.c` | 4.83 → **5.94** |
| `transform.c` | 4.22 → **5.45** |
| `quantize.c` | 4.05 → **5.51** |
| `segment.c` | 4.93 → **5.27** |
| `statistic.c` | 4.20 → **5.24** |
| `feature.c` | 5.20 → **5.23** |
| `threshold.c` | 4.60 → **5.20** |
| `vision.c` | 4.18 → **5.16** |
| `effect.c` | 2.54 → 3.52 |
| `fx.c` | 3.08 → 3.31 |
| `colorspace.c` | 3.01 → 3.15 |
| `composite.c` | 2.68 → 3.01 |
| `morphology.c` | 2.62 → 2.71 |
| `compare.c` | 2.18 → 2.43 |
| `distort.c` | 3.01 (no legal step raises it) |
| `visual-effects.c` | 3.27 (no legal step raises it) |

219 commits: 183 refactor steps, 32 playbook commits, a contract update and three report commits.
Ten files are now above 5. Every refactor commit ran the oracle on its function's cases (0 diverged)
and passed the pre-commit gate, except those on functions no case reaches, which say so
and rest on the guard or, for Recipe D, on an identical object file. The whole catalogue
ran after each file: 15,900 cases, 0 diverged. CodeScene's PR gate passed on #79's pushes.

### The recipes the owner asked for

| Request (W08) | Became | First use |
| --- | --- | --- |
| cleanup then `return(v)` | V step 7: the shared cleanup stays in the caller | `threshold.c` `GetThresholdMapFile`, 4.60 → 4.97 |
| V with dead cleanup writes | V step 7 widened to `return(c)`; no dead-write rule needed | `layer.c` `OptimizeLayerFrames` |
| struct-valued read-only members | N step 2, used by N, Q, U, W, Y, Z | `feature.c` (+0.03) |
| a sequential J | Q: in a function with no OpenMP region `status` goes by pointer | `vision.c` merge census, nesting 6 → 4 |
| V with an out-parameter struct | new **Recipe I**: written locals through a `const` struct of their addresses | `fx.c` `GetFunction`, its last Deep Nesting |

### Further catalogue changes (each its own commit, before the steps that use it)

- **New recipes:** **SD** - Split a Selection at its Default (a `switch` loses some groups,
  `default` among them, to a helper with the same `switch`); **LB** - Extract Block
  Returning Success for a Loop Break (`break` becomes `return(MagickFalse)`, the caller
  breaks), with step 7 (`S continue;`, a retry) and step 8 (two to four written variables
  through an address struct).
- **Amendments:** Q step 5 (written struct values by pointer), Q step 8 (up to six written
  variables, arrays included, through an address struct; a `const` initialiser form);
  K step 6 (sequential K); T step 4 (a block ending the parallel loop body may `continue`
  it); E step 7 (the tail of a loop body); O step 2 (an address passed only to calls that
  keep no copy; by-value parameters count as locals); I steps 2 and 8 and V step 7 (a
  function-local cleanup-and-throw macro stays in the caller); V step 2 (temporaries);
  D (casts of a variable to its own type are not a differing value); rule 1 (several
  blocks of one function may share a commit when only together they raise the score);
  E step 4 (a variable read on entry, written, and read by nothing after goes by value);
  V step 2 (an address passed elsewhere only to calls that keep no copy, as O step 2).
- **Owner-approved:** Recipe D for functions no case reaches, proven by
  `tools/obj_identical.sh` printing IDENTICAL for the whole object file (91ed57bbc,
  43f2b75c6); AGENTS.md and VERIFICATION.md say so.
- **Codes:** every free single letter is now used; A, C, R and X belong to the earlier
  playbook lineage and old branches and were not reused. New recipes take two-letter
  codes (SD, LB). One subagent first took C; it was renamed to SD before pushing.

### For the owner

- **Moving a helper's definition** unchanged above a new caller (`morphology.c`
  31bdef462, `threshold.c` 0ff15871b), so Recipe D can reuse it. Same effect as a
  forward declaration, oracle-verified; no recipe says it in words. A ruling, or a
  one-line amendment to D.
- **Two forms to hide a clone.** `composite.c`'s Darken/Lighten helpers are near-clones;
  one is written in the member form and the other with `const` locals, and CodeScene then
  reports no duplication. Behaviour is identical, but the duplication is only hidden. Later
  briefs forbid it; the pair is left for your ruling (revert, or accept).
- **`&status` inside a parallel loop** would open `ImplodeImage`, `SwirlImage` and
  `resize.c`'s `InterpolativeResizeImage` (a block that assigns the shared `status` and
  tests it). The same memory operations in the same order, but the playbook refuses it
  on purpose.
- **Function-pointer merges** (W08) and **Recipe S** splits remain the levers for
  `enhance.c`, `composite.c` (`CompositeImage` alone holds it near 3), `distort.c`
  (`DistortImage`'s per-pixel `status`) and `feature.c` (16 lines under the 1,000-line
  threshold).
- **Cases that would open code:** `-liquid-rescale` (`resize.c`), `-morphology ... @file`
  (`AcquireKernelInfo`'s file branch), `-monitor -integral` (`vision.c`), a
  `TransparentPaintImageChroma` case (`paint.c`), `-segment` reaching
  `GetImageDynamicThreshold`, Ghostscript separations (`ConsolidateCMYKImages`).
- **A commit message with a wrong number,** pushed before I could amend it: 549e51748
  says `PolynomialImage` cc 14 -> 12; the review measures 13. Two cc values in
  `statistic.c` messages that morning were estimates. Later briefs require measured
  numbers.

### Where files plateau

- `distort.c` 3.01: `DistortImage`'s per-pixel `status` from `ResamplePixelColor` holds
  Brain Method and depth 4; stubbing `DistortImage` gives 4.00.
- `visual-effects.c` 3.27: Implode/Swirl's shared-`status` assign-and-test, MorphImages'
  pragma-bound loop.
- `composite.c` 3.01: `CompositeImage` (Brain Method, 29 bumps, 1,800 lines).
- `feature.c` 5.23: the 1,000-line threshold.
- `compare.c` 2.43, `morphology.c` 2.70: near-copy kernels and SIM shells that become
  duplicates when extracted; status/`continue` paths in parallel loops.
- `fx.c` 3.31: `GetOperand`'s remaining "try" blocks return several different values or
  fall through; a recipe "return whether it returned, with the value through `T *result`"
  would cover them (about +0.02, not added). `ExecuteRPN` (cc 229) keeps its bumps.
- The rest stopped at their commit budget or at the parallel loops' `status`/progress
  skeletons, which no recipe moves.

### Noticed, not changed (possible upstream bugs)

- `layer.c` `DisposeImages`: `for (next=image; image != NULL; image=GetNextImageInList(image))`
  never advances `next`; it also sets `dispose_image->background_color.alpha_trait`
  instead of `dispose`'s.
- `transform.c` `ApplySpliceGravity`: WestGravity adds `width/2` to `y` where every other
  case uses `height`.
- `composite.c` `BlendRMSEResidual`: resets `channel_residual` inside a `reduction(+)`
  loop; `CompositeOverImage`'s copy channel takes `Sc` where `CompositeImage` takes `Dc`.
- `threshold.c` `BilevelImage` increments `progress` twice per row; `OrderedDitherImage`
  leaks `map` when `SetImageStorageClass` fails and always returns MagickTrue.
- `visual-effects.c` Implode/Swirl and `resize.c` `InterpolativeResizeImage`: per-pixel
  writes of the shared `status` in parallel loops.
- `compare.c` `ThrowMSESimilarityException` destroys `alpha_image` twice (harmless);
  `segment.c` `GetImageDynamicThreshold` sizes `extrema[i]` by `ssize_t`, not `short`.
- W07 and W08's lists stand.
