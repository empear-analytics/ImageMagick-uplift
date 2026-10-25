# M25 - refactor `MagickCore/quantize.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.82** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 4151 lines |
| Functions with findings | 23 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/quantize.c
```

The score must read 1.82. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 2 |
| Bumpy Road Ahead | 16 |
| Deep, Nested Complexity | 9 |
| Overall Code Complexity | file |
| Complex Method | 11 |
| Complex Conditional | 3 |
| Code Duplication | 7 |
| Large Method | 10 |
| Excess Number of Function Arguments | 3 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `ClassifyImageColors` | 754 | 48 | 5 | 8 | static | - | 573 | 18 of 19 |
| 2 | `AssignImageColors` | 500 | 30 | 6 | 7 | static | 1 | 436 | 7 of 13 |
| 3 | `FloydSteinbergDither` | 1532 | 32 | 5 | 7 | static | - | 16 | 11 of 19 |
| 4 | `PosterizeImage` | 2888 | 34 | 5 | 6 | public | 3 | 41 | 4 of 9 |
| 5 | `KmeansImage` | 2510 | - | 5 | 12 | public | 1 | 22 | 9 of 17 |
| 6 | `SetGrayscaleImage` | 3940 | 26 | 5 | 5 | static | 3 | 170 | 5 of 7 |
| 7 | `Riemersma` | 1845 | 44 | - | 2 | static | - | 363 | 5 of 5 |
| 8 | `ClosestColor` | 1106 | 11 | 5 | 3 | static | - | 436 | 2 of 2 |
| 9 | `RiemersmaDither` | 1720 | - | 4 | 4 | static | - | 373 | 9 of 13 |
| 10 | `DefineImageColormap` | 1250 | - | 4 | 4 | static | - | 454 | 6 of 7 |
| 11 | `QuantizeImages` | 3389 | 16 | - | 3 | public | - | 16 | 1 of 4 |
| 12 | `QuantizeImage` | 3287 | 15 | - | 2 | public | - | 368 | 3 of 6 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `ClassifyImageColors` (line 754)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 8 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 935, `remove_void_call`: `PruneLevel(cube_info,cube_info->root);`

Commit message: `refactor(MagickCore/quantize.c): simplify ClassifyImageColors`

#### Step 2: `AssignImageColors` (line 500)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 612, `eq_to_ne`: `if (cube.quantize_info->measure_error == MagickFalse)`
  - base line 667, `sub_to_add`: `image->colormap[1].red=(double) QuantumRange-intensity;`
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantize.c): simplify AssignImageColors`

#### Step 3: `FloydSteinbergDither` (line 1532)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1589, `add_to_sub`: `previous=pixels[id]+((y+1) & 0x01)*image->columns;`
  - base line 1610, `mul_to_div`: `pixel.blue+=7.0*cube_info->diffusion*current[u-v].blue/16;`
  - base line 1622, `mul_to_div`: `pixel.alpha+=cube_info->diffusion*previous[u+v].alpha/16;`
  - base line 1622, `div_to_mul`: `pixel.alpha+=cube_info->diffusion*previous[u+v].alpha/16;`
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantize.c): simplify FloydSteinbergDither`

#### Step 4: `PosterizeImage` (line 2888)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 3 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3075, `remove_void_call`: `SetPixelChannel(image,channel,PosterizePixel(q[i],levels),q);`
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantize.c): simplify PosterizeImage`

### Wave 2

#### Step 5: `KmeansImage` (line 2510)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 12 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 5 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2628, `lt_to_ge`: `if (n < (ssize_t) image->colors)`
  - base line 2644, `ne_to_eq`: `if (image->alpha_trait != UndefinedPixelTrait)`
  - base line 2795, `le_to_lt`: `if (fabs(distortion-previous_tolerance) <= tolerance)`
  - base line 2811, `lt_to_ge`: `for (n=0; n < (ssize_t) image->colors; n++)`
  - base line 2813, `remove_void_call`: `GetColorTuple(image->colormap+n,MagickTrue,tuple);`
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantize.c): simplify KmeansImage`

#### Step 6: `SetGrayscaleImage` (line 3940)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 3 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantize.c): simplify SetGrayscaleImage`

#### Step 7: `Riemersma` (line 1845)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).

Commit message: `refactor(MagickCore/quantize.c): simplify Riemersma`

#### Step 8: `ClosestColor` (line 1106)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/quantize.c): simplify ClosestColor`

### Wave 3

#### Step 9: `RiemersmaDither` (line 1720)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantize.c): simplify RiemersmaDither`

#### Step 10: `DefineImageColormap` (line 1250)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1320, `gt_to_ge`: `if (node_info->number_unique > cube_info->transparent_pixels)`

Commit message: `refactor(MagickCore/quantize.c): simplify DefineImageColormap`

#### Step 11: `QuantizeImages` (line 3389)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3481, `remove_void_call`: `ReduceImageColors(images,cube_info);`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantize.c): simplify QuantizeImages`

#### Step 12: `QuantizeImage` (line 3287)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3331, `gt_to_ge`: `if ((quantize_info->dither_method != NoDitherMethod) && (depth > 2))`
  - base line 3351, `gt_to_ge`: `if (cube_info->colors > cube_info->maximum_colors)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantize.c): simplify QuantizeImage`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/quantize.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/quantize.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/quantize.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, morning)

**Result: Code Health 1.82 → 1.95** (`cs-mcp` 1.5.12). Branch `campaign/M25-quantize`,
stacked on `campaign/M24-transform`. 9 refactor commits, all oracle-verified, every
function ready or careful with no survivor on a moved line:
- 3 Recipe P;
- 5 Recipe E;
- 1 Recipe D.

**Predicates:** `IsDitherPositionInImage`, `IsTwoColorGrayQuantize` and
`IsPosterizeDitherMap`. Complex Conditional leaves the file.

**KmeansImage** (bumps 12 → 10, 275 → 235 lines):
- `ParseKmeansSeedColors` returns the number of colours seeded.
- `SeedKmeansRandomColors` takes the `RandomColorComponent` macro with it; the block was
  the macro's only user.

**ClassifyImageColors** (bumps 8 → 6, cc 48 → 44): `GetPixelRunLength`, by E in the first
row loop and D in the second.

**SetGrayscaleImage** (bumps 5 → 4, cc 26 → 23): `CompactGrayscaleColormap` (+0.02) and
`SortGrayscaleColormap`.

Five of these commits left the score flat and are kept under rule 2, each naming its
finding.

**Refused:**
- The leaf-sum blocks in `ClassifyImageColors`' two row loops. The copies differ in a
  cast, `(double) OpaqueAlpha` against `(MagickRealType) OpaqueAlpha`. D does not fold
  a type difference, and E alone was flat.
- The cases of `Riemersma`. Each makes three dither calls whose three directions differ,
  and D allows one differing value.

**Whole catalogue:** run before the batch PR.

**What is left, at 1.95.**
- The parallel row loops in `AssignImageColors`, `FloydSteinbergDither` (not ready),
  `KmeansImage`'s refinement, `SetGrayscaleImage` (with an `omp critical`) and
  `PosterizeImage`. They need Recipe K or T.
- The cube-tree walkers `PruneLevel`, `PruneToCubeDepth` and `Reduce` are reported as
  duplicates. Each recurses into itself, so the copies differ by the function called.
