# W15: morphology.c

Campaign A, branch `campaign/W15` (after the composite.c commits, 29a280da5), 2026-11-07.
Harness: Claude Code (Claude Opus 5.5), WSL2 Ubuntu. Code Health from `cs-mcp` 1.5.16
through `tools/ch.py` (the committed baseline records 1.5.12; 1.5.16 reproduces its 3.04
for this file).

**Result: no refactor commit. `MagickCore/morphology.c` stays at 3.04.** Every candidate
chain was built on scratch copies first, as the task asked, and none pays under the
current rules: the owner's rulings of 2026-11-07 (function count, label runs, Recipe S)
remove blockers that turn out not to be the binding ones. What binds is that the code
holds four pairs of mirrored near-copies, and CodeScene flags them as *Code Duplication*
as soon as a decomposition puts the halves in functions of their own. The measurements
below say how much each lever is worth, so the owner can decide.

Scratch copies, generators and probes are in `build-oracle/scratch/w15m/` (local, not in
git). Every number is a `tools/ch.py` score of a whole file.

## What the score is made of (padded stubs)

The four big functions' bodies replaced by `return(0);`, the removed lines added back as
a data table so *Lines of Code* stays (P = `MorphologyPrimitive`, D =
`MorphologyPrimitiveDirect`, B = `AcquireKernelBuiltIn`, A = `MorphologyApply`):

| Stubbed | Score | Stubbed | Score | Stubbed | Score |
| --- | ---: | --- | ---: | --- | ---: |
| none | 3.04 | PD | 3.59 | PDB | 4.00 |
| P | 3.33 | PB | 3.42 | PDA | 3.62 |
| D | 3.19 | PA | 3.35 | PBA | 3.75 |
| B | 3.12 | DB | 3.27 | DBA | 3.29 |
| A | 3.06 | DA | 3.21 | PDBA | 4.63 (4.76 unpadded) |

The other 13 *Complex Method* functions (`RotateKernelInfo`, `AcquireFreiChenKernel`,
`SetKernelArgumentDefaults`, `AcquireThinSEKernel`, `ParseKernelArray`, ...) are worth
nothing while the big four stand (all 13 stubbed: 3.04) and almost everything after
(all 17 stubbed, unpadded: 9.09; one of the 13 left back in: 8.36 to 8.81).

## What a decomposition costs (measured)

- **Code Duplication.** On the file with the big four stubbed (4.76), one new pair of
  look-alike functions switches the finding on (4.38), and it then also lists the latent
  `AcquireDiamondKernel`/`AcquireOctagonKernel`/`AcquireDiskKernel` trio, which is below
  the threshold on its own. Pairs measured: the Erode and Dilate kernel walks as
  helpers; Erode and IterativeDistance (a plain kernel walk is enough); the PC heads of
  the column (vertical convolve) and row loops of `MorphologyPrimitive`; even the
  15-line pixel loops `for x { for i Channel(...); p+=; q+=; }` of the two paths. The
  6-line Erode/Dilate taps (`if (!IsNaN(*k) && (*k >= 0.5)) pixel=Minimize...`) are not
  flagged. At the top of the scale the finding costs about 0.8 (9.09 to 8.28).
- **Number of Functions in a Single Module.** Appears between 103 and 111 functions in
  the real file (98 today) and replaces *Lines of Code in a Single File*: -0.15 measured
  (3.13 with it, 3.28 with 12 small functions deleted). Allowed by the owner's ruling (1),
  but the commit that crosses it lowers the score and the gate refuses it unless the same
  commit gains more.
- **Bumps.** Each parallel loop body keeps two or three bumps after any form of
  extraction except PC (the failed-fetch `if`, the inner loops, the atomic progress
  block), and even a PC loop (`if (Head()==False) continue;` plus the progress block)
  counts 2. Five helpers with 2 or 3 bumps each, their *Large Method* (IM's
  one-variable-per-line declarations put a struct-unpacking helper over 70 lines
  quickly) and the function-count finding cost about 1.15 together on the
  four-stubbed file (3.61 against 4.76).

## The four functions

**`MorphologyPrimitive`.** Its two parallel paths (the special column pass for 1-D
vertical convolve kernels, and the row pass) are near-copies: guard, fetch, failed
fetch, `center`, the pixel loop, sync, atomic progress. Every bump-free form gives each
path a PC head, and the two heads clone (probe above). Inside the row path the method
switch holds five plain kernel walks (Erode, Dilate, ErodeIntensity, DilateIntensity,
IterativeDistance) and the two intensity walks are mirror images with 3-output taps.
Built for real on scratch copies (whole parallel loops by N, heads by PC, per-thread input
structs, the switch by Z and SD with Z step 7's targets, taps by E and Z):

| Variant | File | D, B, A stubbed |
| --- | ---: | ---: |
| symmetric, 21 helpers (heads clone, intensity taps clone) | 2.77 | 3.13 |
| row path by PC, column pass left whole (intensity taps clone) | 2.83 | 3.32 |
| clone-free: the five walks left inline in the channel helper | 3.02 | 3.31 |
| padded stub, for comparison | 3.33 | 4.00 (PDB) |

The clone-free form leaves a channel helper with nesting 4, 7 bumps, cc 39 and 213
lines, and adds 13 functions: it does not pay. Leaving the column pass whole and
decomposing only the row pass is clone-free for the heads, but only because the two
twins were decomposed differently; that is choosing a shape to get past the clone
detector, which the campaign does not do (W14 lane c, W15 composite), and it still scores
below 3.04.

**`MorphologyPrimitiveDirect`.** The forward and reverse passes are mirrored; as RP
helpers they clone (W13: 2.75). With only the two per-channel `switch (method)` blocks
moved out (no clone, no finding on either helper) it is flat: 3.04, and 3.95 against
3.85 with P, B and A stubbed. What is left (both passes, 6 bumps, nesting 4, cc 37, 218
lines) can only shrink further by separating the twins.

**`AcquireKernelBuiltIn`.** About 60 case labels (its cc of 115 matches a count that
includes the 20 labels in the `#if 0` branch of the first switch). Plus/Cross and Manhattan/Octagonal/
Euclidean (with the existing `AcquireChebyshevKernel`) are look-alikes, so as helpers
they clone; as SD groups by the source's own comments ("Boolean or Shaped", "Distance
Measuring") they keep 2 and 3 bumps. Sobel..Kirsch differ in one string and would be one
SV helper. Alone it is worth +0.08 even as a stub.

**`MorphologyApply`.** No mirrored code; its pieces (the method set-up switch, the
special Direct block with its two cleanup labels, the post-processing switch, the
multi-kernel tail) could go clone-free, but alone it is worth +0.02 even as a stub.

## Why it stops here

- A clone-free chain that does not shape twins differently ends at about 3.0 to 3.3: P
  and D keep their nesting and bumps in the function that holds both twins, and the
  13 or more new functions cross the function-count line.
- A chain that accepts *Code Duplication* would end near 4.0 (4.27 for a padded stub of
  all four with one clone pair, less the function-count finding and the residual helper
  findings), but its first clone step lowers the score by about 0.3 and every gain comes
  at the end (superadditive), so the gate refuses the chain step by step, and rule 1's
  bundles cannot cover it (the clone steps lie in three functions and take several
  recipes).
- Recipe S does not help either: `RotateKernelInfo` is a static that both the kernel
  builders and `MorphologyApply` (through `AcquireReflectedKernel`) call, so neither
  family can leave (W14 lane b); the splits that are possible (`MorphologyImage`,
  `Scale*KernelInfo`) are flat and would only buy function-count slots.

**Plateau: 3.04**, unchanged since W14 lane a.

## Owner decisions

1. **Whether a measured chain may pass through *Code Duplication*.** For example: a chain
   whose end state is built for real on a scratch copy (compiled, oracle-checked) and
   raises the file may commit steps that lower the score, with the gate told which ones.
   Without it the big four stay. Estimated end of the full chain (all four, about 50
   steps): about 4.0.
2. **Or a recipe for mirrored twins.** A kernel walk taking its start pointer and step
   (`k++` against `k--`) and comparator as parameters would merge Erode/Dilate and the
   intensity pair, and a pass taking its direction would merge the Direct passes; that is
   more than Recipe D's one value, so it needs a ruling, not an amendment.
3. **The N amendment** (792cfaac0) stays unreviewed; nothing here used it.

## Upstream behaviour noticed, not changed

- `MorphologyPrimitive` returns `0` when its column pass fails (`status ? ... : 0`) but
  `-1` when the row pass fails; `IterateMorphologyKernel` treats only a negative count as
  an error, so a failed vertical convolve is reported as "no change".
- The pixel's start value for `ErodeIntensityMorphology` is `0.0` (it shares the
  Dilate case of the start-value switch), while Erode starts from the centre pixel: when
  no kernel value of an intensity erode is at least 0.5, the channel is set to 0 rather
  than left unchanged as DilateIntensity leaves it.
- The DilateIntensity comment says "The kernel is not reflected" but the walk starts at
  the reflected end (`k=&kernel->values[width*height-1]`, `k--`), as the offset switch
  expects.
- Erode tests `*k >= 0.5` and Dilate `*k > 0.5`: a kernel value of exactly 0.5 counts for
  erosion but not for dilation.
- Cosmetic: `(void) (void) FormatLocaleFile` twice in the verbose reports, and an
  unreachable `break;` after the last `default:` block of `AcquireKernelBuiltIn`.

Oracle and performance: no library code changed on this branch since the whole-catalogue
run on 9a3d7f5d7 (15,900 cases, 0 diverged), so neither was rerun.
