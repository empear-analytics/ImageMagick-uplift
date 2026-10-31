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
| `visual-effects.c` | 3.27 → 3.50 | 10 |
| `effect.c` | 3.52 → 4.10 (split) → **4.36** | 1 + 12 |
| `effect-blur.c` (new) | 4.91 → **5.29** | 11 |
| `distort.c` | 3.01 (no legal step) | 0 |
| `enhance.c` | 2.97 (saturated) | 0 |
| `morphology.c` | 2.71 (saturated) | 0 |

93 refactor commits (E 31, N 21, W 9, SD 6, V 6, H 4, D 3, Q 3, U 3, T 2, I 1, LB 1, O 1, S 1, Y 1)
and 7 docs and tool commits. No new recipe and no amendment. Every refactor commit ran the
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

- **`visual-effects.c` is a red file that moves only in sets.** Single steps and
  Recipe B on all seven static helpers were flat; the score moved once `SketchImage`,
  `PolaroidImage` and `SteganoImage` left *Complex Method* and *Large Method* together
  (3.27 to 3.38, five flat steps first, under rule 2), then `StereoAnaglyphImage` (to
  3.50). A second pass stopped at a plateau: `PlasmaImageProxy` (cc 21,
  103 lines, 4 bumps; 3.68 in a stub probe) stays: Recipe Y takes three of its
  four pixel blocks, but the clone group that results costs more than it gains (one block
  3.50, two 3.25, three 3.17) and the left block leaves through two constants. The other functions are held by the parallel-loop
  skeleton or by Deep Nesting under a shared `status`.

- **The other three red files are saturated for a session's budget.** Stub probes found
  the lever in each (`distort.c`: `DistortImage` alone 4.00; `morphology.c`: all four big
  functions together 4.76; `enhance.c`: six or seven of its largest functions brought
  to a bare skeleton, 3.02 to 3.69), and no chain of legal steps reaches it: see the
  plateau rows. `enhance.c` would also cross the function-count cliff (twelve extra
  functions, -0.07). Recipe S, or the function-pointer merge, is the lever for all three.

- **`effect.c` was split (Recipe S).** The owner read `W11-split-proposal.md` and said
  "You can do this. Keep going."; the session read that as the go-ahead for the file the
  proposal names. The blur and convolution family (eleven public functions, 46 functions,
  3,312 lines) is now `MagickCore/effect-blur.c` (commit 5408dde16): `effect.c` 3.52 to
  4.10 (2,602 lines), `effect-blur.c` 4.91 (3,416 lines), the whole oracle 15,900 cases,
  0 diverged, guards over the pair unchanged (674 literals, 1052 call sites), the eleven
  public functions the only exported symbols of the new object. Confirm the go-ahead or
  revert the commit; the Windows build configuration outside this repository still needs
  the file.

- **After the split `effect.c` and `effect-blur.c` are no longer saturated:** 3.52 to
  4.36 and 4.91 to 5.29 (23 steps; the whole catalogue 15,900 cases, 0 diverged after each
  pass). `PreviewImage` (cc 56) and the Adaptive pair's clone problem are what remain.
  The same split, for the files whose stub probes found nothing else (`morphology.c`,
  `enhance.c`, `composite.c`, `fx.c`, `compare.c`), is the lever; `effect.c` is the proof.

### For the owner

- **Close readings of the `effect.c` and `effect-blur.c` passes**, for you to accept or
  revert: `effect-blur.c` 42d350a1b calls an existing helper (`CloneDirectImage`) for a
  second identical block under V step 6, which names copies of a V helper, not a second
  copy; with no *Code Duplication* finding it is D in all but name (like `vision.c`
  2221d8d57); 3ba0a7626 (Y with a single exit's cleanup left in the caller, V step 7);
  d0dd89745 passes a local array as the pointer it decays to where N says "exact type";
  `effect.c` 233020a42 relies on a failure branch that no case reaches being dead after the
  block; 55c71a999 (H) moved 4.17 to 4.18 by itself and is kept to unblock 0fea5fbc0.

- **`distort.c`, two rulings** (the pass made no commits for want of them):
  (1) `DistortImage`'s resample site, `status=ResamplePixelColor(...)` and its test in
  the parallel loop, is refused by T, U and J, which allow only `status=MagickFalse;`.
  A new recipe would cover it: the helper returns the call's result and the caller
  writes `status=Helper(...)`, the same write at the same point (`resize.c`'s
  `InterpolativeResizeImage` and `feature.c`'s `MeanShiftImage` have the same site).
  (2) The `Cylinder2Plane` and `Plane2Cylinder` cases hold a never-compiled `#if 0`
  debug block naming `i` and `j`; the preprocessor-branch rule keeps it from moving with
  its case. With both, about 11 steps take the file from 3.01 to about 3.55, all flat
  but the last (a probe).

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

- `effect.c` `PreviewImage`: the threshold preview applies `BilevelImage` to `thumbnail`,
  not to the clone `preview_image`; AddNoise's case 4 never sets `factor`, so its label
  repeats the previous tile's. `effect-blur.c`: `BlurMotionPixel` and `BlurRotationalPixel`
  copy a struct field by field for every channel; `AdaptiveSharpenImage` and
  `AdaptiveBlurImage` are near-clones.

- `distort.c` `DistortImage`: `status=ResamplePixelColor` can overwrite an earlier
  failure with `MagickTrue` (W07); `enhance.c` `NegateImage`'s grayscale path returns
  `MagickTrue` after a failed row (W07); `morphology.c` `AcquireKernelInfo` leaks
  `kernel_cache` on a parse error (W07).

- `visual-effects.c`: `IsPlasmaSegmentAtMidpoint` returns `MagickFalse`, the failure value,
  for a degenerate segment, so `DividePlasmaSegment` ANDs it into its status;
  `PlasmaImageProxy` returns `MagickFalse` from its right-pixel
  block when pixels are missing, `MagickTrue` from the left, bottom and top blocks;
  `MorphImages`' single-image branch writes `status` uninitialised and never reads it;
  `StereoAnaglyphImage`'s cancelled progress report sets `status` but the row loop
  carries on.

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
