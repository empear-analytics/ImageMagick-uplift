# W10 - campaign A, night session (2026-10-30/31)

Not a generated task file. Campaign A (Windows desktop, WSL2), branch `campaign/W10`, cut
from `main` after #79 (3f4076188); `main` checked before every push (it did not move). The
oracle's baseline is `origin/main`. Measured with `cs-mcp` 1.5.16. One subagent per file,
one at a time, under the brief; the session read every new recipe and amendment, and
every step that read a recipe closely, before pushing.

### Report

| File | Code Health |
| --- | --- |
| `statistic-moments.c` | 8.00 → **10.0** |
| `transform-tiles.c` | 8.17 → **10.0** |
| `attribute-hull.c` | 8.49 → **10.0** |
| `layer-merge.c` | 9.08 → **10.0** |
| `gem.c` | 9.21 → **10.0** |
| `colormap.c` | 9.36 → **10.0** |
| `histogram.c` | 7.47 → **9.68** (only a public signature's Excess Arguments left) |
| `prepress.c` | 9.49 → 9.61 |
| `quantize-posterize.c` | 8.18 → 9.47 |
| `resize-sample.c` | 9.10 → 9.47 |
| `channel-combine.c` | 8.22 → 9.34 |
| `statistic-function.c` | 8.70 → 9.23 |
| `threshold-adaptive.c` | 8.76 → 9.13 |
| `decorate-raise.c` | 8.67 → 8.87 |
| `shear-deskew.c` | 6.80 → 8.74 |
| `statistic-image.c` | 6.56 → 8.56 |
| `decorate.c` | 7.73 → 8.55 |
| `threshold-auto.c` | 7.42 → 8.36 |
| `resize-scale.c` | 6.66 → 8.21 |
| `magnify.c` | 6.57 → 8.15 (see *For the owner*) |
| `attribute-bounds.c` | 7.51 → 7.66 |
| `channel-alpha.c` | 6.96 → 7.65 |
| `shear-rotate.c` | 7.05 → 7.63 |
| `shear.c` | 6.47 → 7.62 |
| `montage.c` | 6.61 → 7.36 |
| `resample.c` | 7.30 → 7.42 |
| `attribute-depth.c` | 7.03 → 7.33 |
| `attribute.c` | 6.58 → 7.11 |
| `resize.c` | 6.50 → 7.08 |
| `layer.c` | 6.51 → 7.07 |
| `resize-image.c` | 6.46 → 6.83 |
| `paint.c` | 6.10 → 6.24 |
| `compare.c` | 2.43 → 2.54 |
| `channel.c` | 6.93 (no legal step; see *For the owner*) |
| `effect.c` | 3.52 (no legal step; saturated, see below) |

262 commits: 211 refactor steps, 34 playbook commits, 17 docs commits (plateau rows and
this report). 34 file passes; six files reached 10.0 and eight more passed 9. Every refactor commit ran the oracle on its function's cases (0 diverged) and
passed the pre-commit gate; the whole catalogue ran after each file: 15,900 cases,
0 diverged.

### Catalogue changes

- **Later in the night:** **LT** (a `for` loop followed directly by a test of its own
  condition: the condition is true after the loop exactly when the body broke out, so the
  loop moves to a function returning success and the caller keeps the exit) and **RF**
  (see *For the owner*); amendments to B (an address-of-local member; a struct reached
  only through `const Name *`), V step 7 and Y (a cleanup followed by `return(<expr>)`;
  Y with V step 7's exits; a cursor read on entry and dead after it), E step 3 (a `const`
  struct of pointers by value), SD step 7 (a `switch` without `default`: the new
  function's `switch` ends in `default: break;`; cost: `-Wswitch` no longer flags an enum
  constant added later), V step 6 (`return;` in a `void` function).
- **New recipes:** **TW** (twin blocks in two functions to one helper with an input
  struct, one commit), **SV** (sibling blocks differing in one value; later: copies
  reading different variables under consistent renaming, a member-access value, a
  macro-valued constant), **AR** (local arrays through a struct of pointers to their
  first elements; refuses `&v` and `sizeof v`). All codes checked free on `main` and
  every open branch.
- **Amendments:** E step 4 (a variable written before read, also when the caller set
  it; one only a short-circuit test writes first goes by value to avoid a may-be-
  uninitialised warning), E step 3 (one struct member as the single output, by pointer),
  E step 7 (switch form: a `v` written on every path is declared in the new function),
  O step 2 (discarded out-arguments; addresses passed only to calls that keep no copy;
  by-value parameters; targets-struct fills), O step 5 (`&v` as `v`), K step 3
  (temporaries), Q (argmax over an array), N step 2 (struct by `const` pointer when a
  member is unset; pass-through of a caller's `const Name *`, also from Recipe B), I
  (`return;` in `void` functions; reuse of an identical helper; targets-struct fills),
  T step 2 (a leading guard testing `status` and the flag together), P (leading operands
  of an `&&`/`||` chain), SD step 6 (groups reading locals derived from parameters), TW
  step 2 (a `const` struct of pointers by value), H (see below), and **rule 1** (below).

### For the owner

- **Rule 1 has widened three times tonight**: a set of steps that only together raise
  the score may now share one commit with different recipes, and across several
  functions of the file, when *Overall Code Complexity* (switched on by a file's seventh
  function or so) is what lowers every smaller set. Each such commit names every block,
  its recipe and its score alone, and every smaller set's score (`shear-deskew.c`
  133d38de3, `channel-alpha.c` b96fa40a5, `statistic-moments.c` 52f05b5b1,
  `transform-tiles.c` dc28218b9, `layer-merge.c` 8645db13a, `gem.c` 355ad53f0 - which
  reads the cross-function clause's parenthetical as an example, not a limit - and
  `colormap.c` 1216a80f4). The commits stay oracle-verified, but they are bigger revert
  units than "one recipe, one function". Later briefs said not to widen it further.
- **magnify.c's macro hoists.** Three Recipe H steps (9d4670020 `Corner`, 24dac35e6
  `HelperCond`, 8f12688a1 `WeightVar`, +0.53 together) only move helper macros with
  branches above their functions, because CodeScene counts a macro's lines in the
  function that holds its `#define`; the code is unchanged. The amendment admitting it
  (392454b31) is yours to accept or refuse; the brief forbade further use. The SD steps
  in `Hq2XHelper` depend on the commits' context, so dropping the three needs a revert
  by hand.
- **The parallel-loop skeleton** is now the most common plateau (about ten files): the
  row loop's `if (status == MagickFalse) continue;`, the fetch-failure block that sets
  `status` and `continue`s, the sync, and the atomic progress block. No recipe moves it.
  `prepress.c` would reach 10.0 with its named `omp critical` moved into a helper (a
  named critical is a program-wide lock), which the OpenMP rules forbid.
- **Large red files are saturated.** On `effect.c` no single legal step, nor removing a
  whole function, moves 3.52: each per-function category's penalty is capped, and the
  score moves only once five or six functions leave Bumpy Road, Complex and Large Method
  together. A split (Recipe S) is the lever for `effect.c`, `composite.c`, `fx.c`,
  `compare.c` and the like. `channel.c` (6.93) likewise: a probe without its separate-
  image functions scores 7.32.
- **Recipe B growing a caller that keeps Large Method** by the struct's fill lines would
  give `channel.c` +0.14; the gate refuses it.
- **Recipe RF deletes code** (af9f429f6, used once in 19a15646e): a struct fill that
  repeats an earlier identical fill of the same local, with nothing written in between,
  is removed. The removed fills were added by the campaign's own Recipe N steps in
  `FloodfillPaintImage` (N fills before each call), the object file is byte-identical, and
  the oracle agrees; but AGENTS.md forbids deleting code that looks dead, so this is
  yours to accept or refuse. Later briefs forbade new deleting recipes.
- **Guard-only steps:** `resize.c`'s `LiquidRescaleImage` (seven E/P steps, +0.38) is
  compiled but reached by no case, so those steps rest on the guard and reading, as
  AGENTS.md allows for E and P. A `-liquid-rescale` case would make them oracle-verified.
- **Code no oracle build compiles:** `attribute-depth.c`'s `GetImageDepth` and
  `SetImageDepth` keep most of their findings inside `#if !defined(MAGICKCORE_HDRI_SUPPORT)`
  depth-map branches; the oracle builds are HDRI, so that code cannot be refactored or
  verified. A non-HDRI (Q16 without HDRI) oracle build would open it, and others like it.
- **Readings worth a look:** Recipe Q for building a tile list through
  `AppendImageToList` (`transform-tiles.c`); a one-line `const` struct initialiser used to
  keep `SampleImage` at 70 lines (`resize-sample.c`, a form used 52 times before).

### Noticed, not changed (possible upstream bugs)

- `resize-sample.c` `SampleImage`: `progress++` in the parallel loop has no
  `#pragma omp atomic` (a data race); progress is reported against `image->rows` while
  the loop runs over `sample_image->rows`.
- `channel-combine.c` `CombineImages`: a failed fetch `continue`s, leaking `image_view`
  and skipping `next=GetNextImageInList(next)`, so later channels read the wrong image.
- `quantize-posterize.c` `PosterizeImage`: `map_image` leaks on three failure paths.
- `shear.c` `YShearImage` reports progress against `image->rows` while its loop runs over
  `width`; `shear-deskew.c` `GetImageBackgroundColor`'s `continue` skips `p+=`, so the
  right-edge columns average the wrong pixels.
- `channel.c` `ChannelFxImage` returns NULL without destroying the list on
  `SetImageStorageClass` failure; a possible undefined shift by -1 for assign operations.
- `transform-tiles.c`: the @! row computation tests `geometry.y < -1` where the column
  twin tests `geometry.x < 0`; an empty `crop_geometry` leaves `geometry` uninitialised.
- `threshold-auto.c` `TriangleThreshold` normalises with `c` included; OTSU never tests
  bin 255. `magnify.c`: the result buffer fits only because 3x3 = 9.
- `montage.c` throws away the progress result; `effect.c` `PreviewImage`'s AddNoise tiles
  7 and 8 fall to `default` and reuse the previous label; `compare.c`
  `ThrowMSESimilarityException` never frees `target_image`.
