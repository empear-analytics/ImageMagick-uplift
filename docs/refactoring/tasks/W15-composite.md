# W15: composite.c

Campaign A, branch `campaign/W15` from `main` 9db184dd4, 2026-11-07. Harness: Claude Code
(Claude Opus 5.5), WSL2 Ubuntu. Code Health from `cs-mcp` 1.5.16 through `tools/ch.py`
(the committed baseline records 1.5.12; the before and after scores below are both from
1.5.16, and 1.5.16 reproduces the baseline's 3.08 for this file).

**Result: `MagickCore/composite.c` 3.08 -> 3.46** in 8 refactor commits and 1 playbook
amendment. Every commit passed the build, `tools/ci_warnings.sh`, `refactor_guard.py`
(literals `OK`, `--calls` moving only the new helper `+2`, or the deduplication
fingerprint for the D step), the oracle on the 1,678 cases that reach `CompositeImage`
(0 diverged) and the pre-commit CodeScene gate. Whole catalogue at the end: 15,900 cases,
0 diverged.

## Steps

| Commit | Recipe | What moved | Score |
| --- | --- | --- | ---: |
| 51cfccba4 | N | column loop of `CompositeImage`'s row loop -> `CompositeImageRow(row,p,q)`, 22 inputs in `CompositeRowInputs`, filled per row in the loop body | 3.08 -> 3.37 |
| 6580c063a | N (pass-through) | authentic-pixel block -> `CompositeImagePixel(row,p,q)` | 3.44 |
| 4876e702e | D | source-cursor advance -> existing `AdvanceOverSourcePixel` | 3.44 |
| d81314476 | Z | coverage switch -> `SetCompositeCoverage(row,Sa,Da,&alpha)` | 3.44 |
| fdeda6dac | SD (3 steps) | coverage groups -> `SetCompositeCoverageModulus`, `...Product`, `...Dissolve`; the 32-label run stays (owner's label-run ruling) | 3.44 |
| 792cfaac0 | playbook | Recipe N step 2: combined pass-through form (several names + one new struct) | - |
| b8b3b7206 | N (combined) | channel loop -> `CompositeImageChannels(row,&authentic,q)` | 3.44 |
| 21573749b | Z (combined) | alpha-channel switch -> `CompositeAlphaChannel(row,authentic,q,&pixel)` | 3.44 |
| 9a3d7f5d7 | Z (combined) | colour switch -> `CompositeChannelColor(row,authentic,&tile,&pixel)` | 3.46 |

Flat steps were kept under rule 2: each took the targeted function out of a category
(`CompositeImageRow` out of Complex Method; `SetCompositeCoverage` out of Large Method;
`CompositeImagePixel` and `CompositeImageChannels` out of Large Method and Bumpy Road).

Argument recorded in 6580c063a and 9a3d7f5d7, checked by reading: the colour temporaries
`red`..`luma` and the two `PixelInfo` values are written before they are read on every
path, because every colour-space path of `ConvertRGBToGeneric` and `ConvertGenericToRGB`
writes all three outputs (including the early returns of the HSB, HSI, HSL, HSV and HWB
converters) and `GetPixelInfoPixel` clears the whole struct first, for exactly the six
operators whose groups read them.

Performance (median of 7, 1500x1500, non-OpenMP oracle builds, including image reads):
Multiply 0.131 s -> 0.138 s (+6%), Hue, Dissolve, Overlay, Atop within -1% to +3%.

## Final findings of composite.c (3.46)

Lines of Code in a Single File; Overall Code Complexity; Bumpy Road: `CompositeImage` (4),
`BlendRMSEResidual` (3), `SetVirtualCompositeChannels` (2); Deep, Nested Complexity:
`BlendRMSEResidual` (4); Complex Method: `CompositeChannelColor` 188,
`CompositeImage` 72, `CompositeAlphaChannel` 41, `SetCompositeCoverage` 34 (label run),
`SetVirtualCompositeChannels` 24, `BlendDivergentImage` 20, `SeamlessBlendImage` 19,
`SaliencyBlendImage` 17, `BlendRMSEResidual` 14, `TextureImage` 13,
`CompositeImageChannels` 13, `CompositeImagePixel` 12, `CompositeOverImage` 10; Large
Method: `CompositeChannelColor` 802, `CompositeImage` 471, `CompositeAlphaChannel` 168 and
the five Blend/Over functions; Excess Arguments: `CompositeImage` (public),
`SaliencyBlendImage`, `SeamlessBlendImage`, `SetVirtualCompositeChannels`,
`IsBlendRowsMissing`. No Brain Method any more. The file has 94 functions.

## Why it stops here (measured)

Padded stubs (the removed lines replaced by a data table, so *Lines of Code* stays):

- Removing any single remaining function gains at most +0.02 (setup switch,
  `SetVirtualCompositeChannels`); the colour switch, the alpha switch, `CompositeImagePixel`
  or `CompositeImageChannels` alone gain nothing.
- The gain is superadditive: the pixel path and `CompositeImage` stubbed together give 4.83
  (with only the setup switch stubbed, 4.45).
- Label-run helpers cost about 0.1 each (11-run 0.11, 7-run 0.08); `SetCompositeCoverage`
  costs 0.43 at the 4.8 level.
- The alpha SD chain (four finding-free helpers plus the 11- and 7-runs) measures flat,
  3.46; partitions with single-group helpers of one shape add Code Duplication (3.13,
  3.21), which also drags in old near-clone pairs (`GetFreezeAlpha`/`GetReflectAlpha`,
  `GetSoftBurnBlend`/`GetSoftDodgeBlend`).
- The colour switch's groups add about 186 to cc (CodeScene counts `?:` and `continue`:
  calibrated on a scratch file), so a finding-free chain needs about 30 helpers; each
  colour-space group alone is cc 9. The file would pass 110 functions (Number of
  Functions, about -0.09 measured). With the colour switch stubbed outright and
  `CompositeImage` unchanged the file is 3.47 (3.38 with the count tripped).

So the remaining lift, about 3.7 to 3.8 at best, needs the colour chain, the alpha chain
and the setup switch together (on the order of 60 steps); the steps in between are flat,
and the one that trips the function count lowers the score, so the gate would refuse it
unless bundled under rule 1. Not started.

## Owner decisions

1. **Whether to run the long chain** (colour ~30 helpers, alpha 4, setup switch ~25) for
   about +0.3, accepting ~125 functions in the file. A Recipe S split of the Texture family
   (`TextureImage` and its 7 statics) would free 8 function slots; S needs the go-ahead.
2. **`BlendRMSEResidual`** (+0.2 alone, the file's only Deep, Nested Complexity): every
   loop that would move writes `area` and `channel_residual`, named in `reduction`
   clauses; the OpenMP rule forbids moving such writes even through their addresses
   (inside the region they are the thread's private copies, so a pointer to them would be
   sound, but that is a ruling, not a recipe).
3. **The N amendment** (792cfaac0), added under the mandate; awaiting the owner's review.

## Upstream behaviour noticed, not changed

- `BlendRMSEResidual` sets `channel_residual=0.0` at the start of every row inside the
  `reduction(+:channel_residual)` loop, so (serially) only the last row's sum survives,
  and under OpenMP each thread keeps only its last row: the RMSE residual that drives the
  saliency/seamless blend iterations looks wrong.
- `CompositeImageRow` (formerly the row loop of `CompositeImage`) fills `canvas_pixel` and
  `source_pixel` with `GetPixelInfo` once per row; nothing reads those fills, since every
  reader overwrites them with `GetPixelInfoPixel` first (dead stores, kept).
- W14's notes still stand (`white_luminance` set at its declaration and again before use;
  `SetGeometryInfo` called twice on the Displace path).

## Second pass: the owner's rulings of 2026-11-07

Rulings: (1) run the long chain, accepting about 125 functions, bundled so the gate
passes; (2) a Recipe S split of the Texture family may free slots; (3) leave
`BlendRMSEResidual` and every OpenMP reduction variable alone; (4) the N amendment
(792cfaac0) is not ruled on and stays awaiting review.

No commit came out of the second pass. Measured on scratch copies (`cs-mcp` 1.5.16):

- **Colour chain, built for real.** A partition of the colour switch's groups into SD
  helpers of cc 8 or less needs 24 helpers (the six colour-space groups stay in
  `CompositeChannelColor`, cc 44). Packing in source order gives eight *Code
  Duplication* helpers (2.92); first-fit-decreasing gives six (2.99); the member form
  of the bindings makes it worse (2.92). A local search over partitions (moving and
  swapping groups between flagged helpers, 10 s per measurement) found a clone-free
  one after ten tries. With the alpha chain (4 helpers) it scores **3.37**, below
  today's 3.46: the file reaches 122 functions (*Number of Functions*) and
  `CompositeImage` still holds its setup switch.
- **Setup switch.** Padded stubs of the bodies of the Intensity, Blur and Displace
  groups plus the row-loop tail: 3.48 without the colour chain, 3.48 with it; with also
  the Copy, Dissolve, Blend, Saliency and Seamless bodies moved, 3.52 with the colour
  chain. Each piece alone: 3.46. So the whole plan ends at about **3.52** (+0.06), and
  only if every group body and the tail leave `CompositeImage`, which stays a Complex
  (cc 35) and Large (168 lines) Method even then. The 3.82 of a fully stubbed
  `CompositeImage` needs the switch itself gone, but its groups write nine caller
  variables on mixed `break`/`return` paths (Recipes I and Z stop at six).
- **First real setup step refused by the measurement.** Recipe E step 7 with N step 1
  on the Intensity group (its block after the guards, holding the parallel loop) gives
  `CompositeIntensityImage`, which CodeScene pairs with W14's `CompositeCopyImage` (the
  same row loop with another row helper): *Code Duplication*, 3.46 -> 3.21. Reverted
  before commit. The two differ in more than one value, so Recipe D does not merge them.
- **Recipe S on the Texture family** is refused by `tools/split_source.py`:
  `SyncCompositeRow` is a static shared by the family and the kept code (W13's D step
  made ten loops use it), and Recipe S forbids widening its linkage; copying it would
  need the parked, unapproved `--copy-static` amendment. It would not help either: the
  chain passes 110 functions with or without the seven slots, and the file without the
  family scores the same 3.46.

**Plateau: 3.46.** The long chain's measured end (about 3.52) depends on a bundled
commit across the function-count limit, a clone-free partition that a later change can
break, and a setup-switch decomposition whose first step already hits the duplication
cliff. Left for a decision with these numbers: either a ruling that admits a
`CompositeCopyImage`/`CompositeIntensityImage` merge through a row-operation parameter,
or accepting the plateau.

Oracle: the code is unchanged since the whole-catalogue run on 9a3d7f5d7 (15,900 cases,
0 diverged).
