# W11 - campaign A, day session (2026-10-31)

Not a generated task file. Campaign A (Windows desktop, WSL2), branch `campaign/W11`,
stacked on `campaign/W10` (#80) after `main` was merged into it (9d668a8d5). Measured
with `cs-mcp` 1.5.16. One subagent per file, one at a time, under a brief that holds the
owner's open questions open: one recipe and one function per commit (no use of rule 1's
widened forms), no recipe that deletes code, every number measured, and no step that
adds *Overall Code Complexity* to a file. The session read every step a subagent
flagged as a close reading before pushing.

### Report

| File | Code Health | Commits |
| --- | --- | ---: |
| `vision.c` | 5.16 → 5.64 | 12 |
| `statistic.c` | 5.24 → 5.84 | 8 |
| `segment.c` | 5.27 → 5.70 | 11 |
| `transform.c` | 5.45 → 5.60 | 9 |
| `quantize.c` | 5.51 → 5.60 | 5 |
| `fourier.c` | 5.94 → 6.02 | 7 |
| `threshold.c` | 5.20 → 5.35 | 6 |
| `feature.c` | 5.23 → 5.24 | 1 |

58 refactor commits (E 23, N 9, W 9, SD 6, D 3, Q 2, T 2, V 2, H 1, I 1, O 1) and one docs
commit (plateau rows). No new recipe and no amendment. Every refactor commit ran the
oracle on its function's cases (0 diverged) and passed the pre-commit gate; the whole
catalogue ran after each file: 15,900 cases, 0 diverged. None of the steps adds
*Overall Code Complexity* or *Code Duplication* to a file, except as noted for
`threshold.c` below.

### Where the files stop

- **The file-level limits now bind.** `feature.c` has 999 lines of code against
  *Lines of Code in a Single File* at 1,002 (a probe: -0.28); `quantize.c` has 110
  functions, and a 111th switches on *Number of Functions in a Single Module* (-0.12).
  Both stop every extraction. `statistic.c`, `segment.c`, `transform.c` and `fourier.c`
  already carry *Lines of Code in a Single File*. A split (Recipe S) is the lever for all
  six; that is the owner's decision.
- **The parallel-loop skeleton** again: `statistic.c`, `transform.c`, `threshold.c`,
  `segment.c`'s `Classify` and `fourier.c`'s parallel sections keep their bumps in the
  `status` `continue` blocks and the atomic progress blocks.
- **Look-alike loops.** In `transform.c` and `threshold.c`, shrinking one more row-loop
  function exposes the others' skeletons as *Code Duplication* (probes 5.01 to 5.18).
- **Unreached code.** `segment.c`'s `GetImageDynamicThreshold` (no case, no in-tree
  caller) takes only E, G, P and D's unreached form; a case reaching it would open the
  rest. `vision.c` stopped at the brief's 12-commit limit, not at a plateau.

Plateau rows for `feature.c`, `statistic.c`, `segment.c`, `transform.c`, `quantize.c`,
`fourier.c` (replacing the Mac row, written without FFTW) and `threshold.c` are in the
playbook.

### For the owner

- **Recipe D without a *Code Duplication* finding** (`vision.c` 2221d8d57): eight
  identical threshold-range parses inside `ConnectedComponentsImage` became calls of one
  helper. CodeScene does not report clones inside one function, so D's *Use when* is not
  met as written; the copies are identical by script and the oracle agrees. Accept, or
  say D needs the finding and the step is reverted by hand (later steps touch the same
  function).
- **`threshold.c` 8023c54b7:** after it, `PerceptibleImage`'s existing row-loop skeleton
  joins the *Code Duplication* group (5 functions to 6). No code was duplicated; it is the
  same exposure as W04's, which you suppress. Please confirm the suppression covers it.
- **A thrown warning names the helper.** Steps under W and E moved
  `ThrowMagickException(...,GetMagickModule(),...)` into helpers (`transform.c`,
  `fourier.c`, `quantize.c`, `threshold.c`). The oracle normalises locations
  (`ORACLE.md`) and Recipes V and I say so in *Careful*; W and E do not. A one-line note
  in E and W would make it explicit.
- **Patterns reported, no recipe added:** `KmeansImage`'s seeding leaves through
  `return(status);` with `status` read later (W and V refuse it); `Classify`'s removal
  loop in a function with a parallel region elsewhere (K step 6 and Q step 1 ask for no
  OpenMP region at all); `ColorThresholdImage`'s in-place scaling of two struct locals.

### Corrections made before pushing

- `vision.c` d45ad426a was written with the trailer `Recipe: OC`; its block has no
  `continue`, so plain Recipe O fits. The message was corrected before the push; the
  tree is unchanged.
- `threshold.c` d0ea1b9f4's message says the map lookup is the next step towards
  *Large Method*; the later W step took the function to 79 lines, still a *Large Method*.

### Noticed, not changed (possible upstream bugs)

- `vision.c`: `IntegralImage` tests `p` twice and never `q`; in
  `SetEllipseAngleMetric`, `fabs(M11) < 0.0` is never true.
- `segment.c` `GetImageDynamicThreshold`: leaks the histograms, the extrema and any
  clusters already allocated when a cluster allocation fails; passes `0.2f` for
  `min_tau` where `SegmentImage` passes `0.2`.
- `fourier.c` `ForwardFourierTransformChannel` tests `magnitude_info == NULL` before
  releasing it (`!=` is meant).
- `threshold.c`: `BilevelImage` increments `progress` twice per row; `OrderedDitherImage`
  leaks `map` when `SetImageStorageClass` fails and always returns `MagickTrue`.
- `feature.c` `MeanShiftImage`: each lookup overwrites `status` inside the parallel loop,
  so a later success can mask an earlier failure (W03).
