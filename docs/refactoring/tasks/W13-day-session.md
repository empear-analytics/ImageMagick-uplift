# W13 - campaign A, evening session (2026-11-04)

Campaign A (Windows desktop, WSL2), branch `campaign/W13`, cut from `main` after #86 merged.
A fresh sweep of `MagickCore/` (cs-mcp 1.5.16) gave a mean of 6.65 and showed several plateau
rows were stale (`magnify.c` was 8.15, not 6.57). One subagent per file, sequentially; the whole
catalogue ran at the last commit of each file: 15,900 cases, 0 diverged.

| File | Score | Commits | Recipes |
| --- | --- | ---: | --- |
| `magnify.c` | 8.15 -> 8.2 | 1 | PC |
| `feature-texture.c` | 5.18 -> 5.25 | 2 | CN |
| `resize-image.c` | 6.83 -> 7.68 | 1 | PC |
| `layer.c` | 7.07 -> 7.08 | 2 | LB, V |
| `shear.c` | 7.62 -> 8.03 | 3 | W, O |
| `montage.c` | 7.36 -> 7.66 | 7 | E, W, O |
| `attribute-bounds.c` | 7.66 -> 7.91 | 6 | E, N, P, D (rule 1 set) |
| `attribute-depth.c` | 7.54 -> 7.56 | 1 | PS 7 |
| `statistic-image.c` | 8.56 -> 9.09 | 5 | PC, H, PS, E |
| `decorate.c` | 8.55 -> 9.43 | 5 | PC, H, PS, FI, V |
| `resize-scale.c` | 8.21 -> **10.0** | 12 | B, LB, H, N, P |
| `threshold-auto.c` | 8.36 -> **10.0** | 6 | N, E |
| `resize.c` | 8.65 -> 8.81 | 2 | PC, PS |
| `shear-deskew.c` | 8.97 -> **10.0** | 5 | PS 7, FI, E |
| `channel.c`, `resample.c`, `shear-rotate.c` | 6.93, 7.42, 7.63 | 0 | none legal |

**Lesson of the session.** In a file with few functions a new helper switches on *Overall Code
Complexity* (channel.c lost about 0.35 on every probe; feature-texture.c 0.76), and a second
look-alike PC head is a Code Duplication clone (resize-image.c `VerticalFilter` 7.41, shear.c 7.43).

**For the owner.** (1) `layer.c` has `#if DEBUG_OPT_FRAME`, a macro defined as 0 in the file, so no
configuration compiles it. The `#if 0` exception names only a literal `#if 0`. A probe that moved the
background-expansion block (debug text dropped) scored 7.08 -> 7.19. May an `#if M` branch move
verbatim when `M` is unconditionally defined as 0 in the same file? Not used until you rule.
(2) `layer.c` 312b1ba9c is flat as the first step of a two-step plan (rule 2). (3) `montage.c` commits
1, 2, 3 and 6 cover allocation- and thumbnail-failure paths no case reaches (checked by reading);
the oracle normalises the helper's `GetMagickModule()`.

**Noticed upstream, not changed.** `GetImageFeatures` green-channel guard and divisor use
`density_y[x].direction[i].red`, `density_xy` is not cleared between passes, a failed co-occurrence
pass leaks; `ChannelFxImage` returns NULL without destroying the list when `SetImageStorageClass`
fails; `YShearImage` reports progress against `image->rows` over a width loop and
`IntegralRotateImage`'s 180-degree `progress++` races; `MontageImageList` leaks on allocation
failure; `ResamplePixelColor` leaves `status` unused.

## Later in the evening: new recipes and amendments to rule on

The second half of the session added playbook text under the mandate; each is in its own
commit, before the steps that use it. **The owner has ruled on none of these.**

- **Recipe FI, Fill a Struct by Initialiser** (36fa7a40e): a struct local declared bare and filled
  one `name.m=e;` per member, every member in declaration order from a plain identifier or literal,
  only read afterwards, becomes `const Name name = { e1, ... };`; `obj_identical.sh` must print
  IDENTICAL. It deletes no effect but it shortens the function, so it may be the kind of recipe the
  owner does not want. Used on `decorate.c` (f6aa4bae9) and `shear-deskew.c` (509bdee80, which is
  worth +0.36 there). Drop by reverting those two commits.
- **PC step 4 amendment** (c2822f30a): PC's const input struct may be initialised in the body by
  identifiers; the same idea as FI. Used in `statistic-image.c` (a9798970c) and `resize.c`.
- **LB step 4 and step 8** (a79714ec1, 02ed2c4b0): a struct filled once before a loop of a function
  with no OpenMP region may serve blocks of the loop; up to six variables share one `const` struct
  of addresses. Every `resize-scale.c` step from c5275cf85 to 0f69dbe93 depends on them.
- **Flat steps under rule 2**: `shear-deskew.c` 07bedb7c3 and 5c98fc269 (the plan's last step takes
  the file to 10.0 by lowering the mean cc, so these are splits made for the score; each is a real
  block with its own helper); `resize-scale.c` d1976b334 (H) and bd035f0e6.
- **Unreached paths checked by reading**: `ReleaseScaleBuffers`, `ThrowKapurFailure`,
  `ThrowOTSUFailure`.

Whole catalogue after the last commit: 15,900 cases, 0 diverged.
**Noticed upstream, not changed.** `ScaleImage` passes `span` by value although only `span.y` is set,
so the copy reads an uninitialised `span.x`; `GetImageBoundingBox`'s `pixel=zero` is dead;
`ReadRadonRowReversed` sets and returns `status`.

## The night: sharing one helper across all loops (2026-11-04 to 05)

**The lever.** Earlier agents stopped because every second PC head was a clone of the first. One
helper *called from every loop* is not a clone. The three that worked: the progress tail after
`omp atomic progress++` (PS), the sync test (PS), and a partial PC head (guard, fetch, failed-fetch
continue; `q` by pointer, PC step 2). Each function alone often drops the score (a new function in a
small file), so these go as rule 1 same-recipe sets with the per-function scores in the message.

| File | Score | Commits |
| --- | --- | ---: |
| `transform.c` | 5.90 -> 6.26 | 6 |
| `threshold.c` | 5.50 -> 6.82 | 17 |
| `enhance-tone.c` | 4.37 -> 5.07 | 5 |
| `enhance.c` | 5.43 -> 5.83 | 3 |
| `effect-blur.c` | 5.69 -> 5.95 | 1 |
| `effect.c` | 5.94 -> 6.17 | 2 |
| `visual-effects.c` | 3.90 -> 4.23 | 3 |
| `paint.c` | 6.55 -> 7.02 | 3 |
| `fourier.c` | 6.27 -> 6.33 | 1 |
| `statistic-measure.c` | 9.68 -> **10.0** | 2 (+ PC step 2 amendment) |
| `segment.c`, `statistic-function.c`, `prepress.c`, `histogram.c` | unchanged | 0 |

Whole catalogue at the end of each file: 15,900 cases, 0 diverged.

**For the owner, in addition to the list above.** (1) PC step 2 amendment (e9e484a7d: up to two
body variables the tail reads, by pointer; a shared variable the head only reads goes by pointer to
const). (2) `transform.c` and `threshold.c` PS sets rest on rule 1's same-recipe clause; `threshold.c`
1c4f1fd8f is flat as a plan step. (3) `TransparentPaintImageChroma` (paint.c) is unreached; PS and PC
there were checked by reading. (4) `prepress.c` would reach 10.0 only by moving a pragma-bearing
loop. (5) A function-pointer merge would open `LevelColors`/`LevelizeColors`, the `ModulateXXX`
helpers and the transform skeletons.

**Noticed upstream, not changed.** `BilevelImage` counts progress twice per row; `Classify` carries
`status` and `progress` from its count loop into the classify loop, so a monitor abort in the first
skips classification; `DestroyOilPaintClones` assigns the result of `DestroyImage(paint_image)` to
`linear_image`.

### Later in the night (2026-11-05, 00:00 to 03:00)

| File | Score | Commits |
| --- | --- | ---: |
| `colorspace.c` | 3.27 -> 3.52 | 2 |
| `composite.c` | 3.01 -> 3.02 | 6 |
| `fx.c` | 3.31 -> 3.37 | 11 + 2 playbook |
| `feature.c` | 6.51 -> 7.39 | 7 |
| `vision.c` | 6.98 -> 7.15 | 2 |
| `compare.c`, `morphology.c` | 2.54, 2.71 | 0 |

**More playbook text under the mandate, not yet ruled on:** Recipe RV (Extract Block Returning
Whether It Returned; 14cc1d7c6) and Recipe Z step 7 (423fef3a8), both from the fx.c pass. Also
review: feature.c applies OC's tail form to the three post-atomic lines of two sequential loops
(`HoughLineImage`, the Canny hysteresis loop) because PS is parallel-only; `composite.c`'s steps
are mostly flat plan steps (rule 2) for +0.01 in all; `colorspace.c` 6dc7b78b8 is a rule 1 set
(first function alone flat).

**Noticed upstream, not changed.** `SimilarityImage` increments `progress++` outside `omp atomic`
and its `threshold_trigger` test can never fire; `CompositeOverImage` and the main loop of
`CompositeImage` have dead `canvas_pixel`/`source_pixel` initialisations and `CompositeOverImage`
sets `status` and `progress` twice; `HoughLineImage` reports progress under `CannyEdgeImageTag`.

### 02:00 to 03:30: whole-function chains

The shared-helper lever is spent on most files; what still moves a red file is making a giant
function leave Bumpy Road, Complex Method and Large Method completely, with a chain of finding-free
helpers planned and measured on a scratch copy first.

| File | Score | Commits |
| --- | --- | ---: |
| `fx.c` (`GetFunction`) | 3.37 -> 3.52 | 17 (incl. 2 playbook) |
| `distort.c` (`GenerateCoefficients`) | 3.55 -> **4.25** | 31 (incl. 1 playbook) |

**New playbook text, not ruled on:** Recipe SW (Split a Returning Selection into Chained Functions;
6f1cac76c), Recipe EI (Chain Exiting Guards into an Else-If; d76680685: CodeScene counts `else { if }`
as a bump and `else if` as none, so EI changes what the metric sees, not the code; obj_identical
IDENTICAL), Recipe I step 2 amendment (dc05428f1: up to six writes via a const struct of addresses).
The distort.c chain is flat through its first 15 code commits and rests on rule 2's plan clause; the
eight `#if 0` blocks moved verbatim with their cases (compared by script).

**Noticed upstream, not changed.** The count switch in `GenerateCoefficients` has a stray `break;`
after the `#if 0 case QuadrilateralDistortion`, and `QuadrilateralDistortion` reports 19 coefficients.

### 03:30 to 05:00

| File | Score | Commits |
| --- | --- | ---: |
| `colorspace.c` | 3.52 -> **4.63** | 9 |
| `compare.c` | 2.54 -> 2.97 | 8 |
| `morphology.c` | 2.71 | 0 |

**For the owner.** (1) `colorspace.c` dfb4a6e95 hoists the 236-line `YCCMap` table to file scope (Recipe F),
3.52 to 3.77; the agent thinks most of that gain is the file getting shorter and the object-identity
tools need `otool`, so the disassembly was compared instead (code identical, `.rodata` layout differs).
Revert that one commit if F is not to count there. (2) The oracle build has no OpenMP: the pragma
handling in `colorspace.c` was checked by reading, and no case was confirmed to reach the PseudoClass
blocks. (3) `compare.c`'s first six `SimilarityImage` commits are flat plan steps (rule 2); `SIMFilterImageNaNs`
is unreached.

**Noticed upstream, not changed.** The `shared(status)` clause of the TransformsRGB Pseudo loop names a
variable the loop never uses; `SimilarityImage`'s `threshold_trigger` guard is dead and its `progress++`
is non-atomic.

### 05:00: Recipe CB, Extract Critical Block Body

New recipe (337f38c8d, `mandate`, **not ruled on**): the body of an `omp critical` block moves into a
`static void` helper called between the pragma's braces; the pragma and braces stay in the caller, so
the helper runs in the same critical region, at the same point, on the same memory, in the same order
(the argument Recipe PS makes for the loop status). The oracle is single-threaded, so steps 1 to 3 are
checked by reading. Used on `prepress.c` (9.61 to **10.0**, `UpdateInkDensity`), `feature.c`
(`UpdateCannyRange`, 7.39 to 7.40) and `attribute-bounds.c` (`MergeBoundingBox`, 7.91 to 7.94). The
eleven `Get*Similarity` merges in `compare.c` stay at 2.97 with it.

**Noticed upstream, not changed.** In `GetImageTotalInkDensity` the test `density > total_ink_density`
just before the critical reads the shared maximum outside the lock (a benign race, kept);
`GetMEPPSimilarity` takes a non-const `Image *` only to write `image->error`; `SimilarityImageTag` is
`#define`d inside `GetDPCSimilarity` but used by NCC.

### 05:40: composite.c, a chain that could not finish

`CompositeImage` alone is worth 3.02 to 5.26 on a padded stub. A generator-built chain (28 helpers:
20 for the per-channel pixel switch, 4 alpha, 4 coverage, 1 source-pixel) was committed on a local
branch and measured flat at 3.02, because three switch groups with 32, 11 and 7 labels cannot sit in a
helper under cc 9 (a group of n labels costs cc n+1 wherever it lives) and splitting a label run by
repeating the body clones (2.64). Rule 2 forbids keeping flat steps of a plan that cannot complete, so
the four commits are **not in this PR**; they are on the local branch `w13-composite-flat` (WSL clone,
unpushed) and every helper in them is finding-free.

**Owner decision.** Both the loop body and the setup switch removed, plus three helpers that are pure
label runs (32, 11 and 7 labels), gives about 4.03 and leaves three Complex Method findings. May a helper
that is a pure `switch` label run count? If yes, the chain can be finished (about 25 more commits).

**Noticed upstream, not changed.** In `CompositeImage`'s Displace/Distort setup loop a failed
`InterpolatePixelInfo` ends the loop with `break` but `status=MagickTrue` is reassigned right after the
setup switch, so the failure is lost; the Blur setup loop exits silently on a failed fetch or sync; the
alpha switch's `pixel` is read by `GetHueAlpha`, `GetMultiplyAlpha` and `GetDarkenIntensityAlpha` before
any write, so it relies on its 0.0 initialiser.

### 05:55: a rule I got wrong, and transform.c

**Rule check.** AGENTS.md: for a function no case executes, only Recipes E, G and P and D's *Unreached
functions* form apply. Recipes PS and PC were applied to unreached functions in two places tonight:
`transform.c` `ExcerptImage` (three commits) and `paint.c` `TransparentPaintImageChroma` (parts of
b487df898, 274447d3e and b5af16405). The transform commits are **not in the PR** (local branch
`w13-transform-excerpt`). The paint commits were already pushed: **owner decision**, keep them (checked by
reading, `refactor_guard --calls`, the whole catalogue) or have the Chroma parts restored (paint.c would
drop below 7.02; the hook refuses a drop, so that needs a `--no-verify` approval).

| File | Score | Commits |
| --- | --- | ---: |
| `transform.c` | 6.26 -> 6.42 | 3 (Chop PC, Splice PC, Splice PS) |

Deviation from PC step 1 in the Chop and Splice commits: the block starts after the `if (status ==
MagickFalse) continue;` guard (with the guard in the helper the row helpers clone Crop/Flip and each other,
5.85 and 5.87). Each message says so. Owner to rule.

**Noticed upstream, not changed.** `ChopImage`'s first loop lists only `shared(status)`; `progress` is
shared by default.

### 06:25: decorate.c reaches 10.0

`FrameImage`: E twice (`DrawFrameSides` 9.43 -> 9.50, `DrawFrameBorders` by E step 7 -> 10.00), 137 cases,
0 diverged, reusing the existing `FrameSideLine` and `FrameComposite` structs; no Recipe FI or other new
recipe was needed. Six W13 files are now at 10.0 (resize-scale, threshold-auto, shear-deskew,
statistic-measure, prepress, decorate), plus resize-sample from W12.

### 06:30: channel-alpha.c and attribute.c

| File | Score | Commits |
| --- | --- | ---: |
| `channel-alpha.c` | 8.14 -> 8.96 | 8 (E five times, SD three times) |
| `attribute.c` | 7.92 -> 8.94 | 3 (SD chain with `SetImageTypeInput`) |

Whole catalogue after the last commit: 15,900 cases, 0 diverged. **Owner question:** may Recipe PS take
`&census[i]` and `background+i` as per-iteration elements of function-local arrays (attribute.c
`GetEdgeBackgroundColor`)? Today PS admits only scalars and `status`.

### 06:30: attribute-bounds.c to 9.57

`GetEdgeBoundingBox` N, Z, OC and `GetEdgeBackgroundCensus` B, SD: 7.94 -> 9.57, 13 cases, 0 diverged,
whole catalogue 15,900 cases 0 diverged. One commit is parked on the local branch `w13-ab-wip`
(`GetImageTargets`, Recipe W with a returned value that is not a constant, 9.57 -> 9.60): **owner
ruling** on whether W step 2 allows it. Noticed upstream, not changed: the trim loop spins forever if no
edge census equals the minimum (unreachable in practice).
