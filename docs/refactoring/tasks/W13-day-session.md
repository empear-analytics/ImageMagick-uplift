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
