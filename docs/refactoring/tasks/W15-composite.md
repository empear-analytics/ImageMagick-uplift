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
3. **The N amendment** (792cfaac0), decided under the mandate.

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
