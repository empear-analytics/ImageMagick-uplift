# M56 - refactor `MagickCore/histogram.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **5.97** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1247 lines |
| Functions with findings | 10 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/histogram.c
```

The score must read 5.97. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 6 |
| Deep, Nested Complexity | 2 |
| Overall Code Complexity | file |
| Complex Method | 6 |
| Code Duplication | 4 |
| Large Method | 3 |
| Excess Number of Function Arguments | 2 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `CheckImageColors` | 670 | 20 | 5 | 4 | static | - | 256 | 11 of 16 |
| 2 | `ClassifyImageColors` | 206 | 17 | 5 | 4 | static | - | 573 | 6 of 7 |
| 3 | `UniqueColorsToImage` | 1152 | 13 | - | 3 | static | - | 12 | 8 of 9 |
| 4 | `GetNumberColors` | 1030 | 10 | - | 3 | public | - | 129 | 8 of 10 |
| 5 | `MinMaxStretchImage` | 927 | 9 | - | 2 | public | - | 42 | 0 of 5 |
| 6 | `DefineImageHistogram` | 366 | - | - | 2 | static | - | 90 | 2 of 2 |
| 7 | `IsPixelInfoColorMatch` | 178 | 9 | - | - | static | - | 285 | 4 of 5 |
| 8 | `DestroyColorCube` | 466 | - | - | - | static | - | 292 | 0 of 2 |
| 9 | `IsHistogramImage` | 833 | - | - | - | public | - | 192 | 0 of 1 |
| 10 | `IsPaletteImage` | 868 | - | - | - | public | - | 12 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `CheckImageColors` (line 670)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 5 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 710, `remove_void_call`: `GetPixelInfo(image,&pixel);`
  - base line 711, `remove_void_call`: `GetPixelInfo(image,&target);`
  - base line 755, `post_inc_to_post_dec`: `node_info->list[i].count++;`
  - base line 774, `remove_void_call`: `GetPixelInfo(image,&node_info->list[i]);`
  - base line 778, `eq_to_ne`: `if (image->colorspace == CMYKColorspace)`

Commit message: `refactor(MagickCore/histogram.c): simplify CheckImageColors`

#### Step 2: `ClassifyImageColors` (line 206)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 255, `remove_void_call`: `GetPixelInfo(image,&pixel);`

Commit message: `refactor(MagickCore/histogram.c): simplify ClassifyImageColors`

#### Step 3: `UniqueColorsToImage` (line 1152)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/histogram.c): simplify UniqueColorsToImage`

#### Step 4: `GetNumberColors` (line 1030)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1078, `remove_void_call`: `GetPixelInfo(image,&pixel);`
  - base line 1093, `remove_void_call`: `ConcatenateColorComponent(&pixel,BlackPixelChannel,NoCompliance,`

Commit message: `refactor(MagickCore/histogram.c): simplify GetNumberColors`

### Wave 2

#### Step 5: `MinMaxStretchImage` (line 927)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 950, `sub_to_add`: `if (fabs(min-max) >= MagickEpsilon)`
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/histogram.c): simplify MinMaxStretchImage`

#### Step 6: `DefineImageHistogram` (line 366)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/histogram.c): simplify DefineImageHistogram`

#### Step 7: `IsPixelInfoColorMatch` (line 178)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/histogram.c): simplify IsPixelInfoColorMatch`

#### Step 8: `DestroyColorCube` (line 466)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/histogram.c): simplify DestroyColorCube`

### Wave 3

#### Step 9: `IsHistogramImage` (line 833)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/histogram.c): simplify IsHistogramImage`

#### Step 10: `IsPaletteImage` (line 868)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/histogram.c): simplify IsPaletteImage`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/histogram.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/histogram.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/histogram.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, night)

**Result: Code Health 5.97 → 6.34** (`cs-mcp` 1.5.12). Branch `campaign/M56-histogram`,
stacked on `campaign/M55-exception`. Five E commits:
- `GrowColorList` (ClassifyImageColors, careful, oracle).
- `ExtendColorList` and `SetColorListEntry` (CheckImageColors, not ready, guard). The
  two functions grow their lists differently (doubling versus i+1), so the helpers
  stay separate.
- `FormatColorTuple` (GetNumberColors, not ready, guard), 6.04 → 6.32.
  GetNumberColors leaves Complex Method and Large Method.
- `SetUniqueColorPixel` (UniqueColorsToImage, careful, oracle).

**What is left, at 6.34.**
- ClassifyImageColors and CheckImageColors (depth 5): the tree descent returns or
  `break`s from inside its allocation check. ClassifyImageColors' colour record would
  need five arguments.
- The duplicates (DefineImageHistogram, DestroyColorCube, IsHistogramImage,
  IsPaletteImage) are short tree walks and predicates whose copies differ in more than
  one value.
- MinMaxStretchImage and UniqueColorsToImage take 5 arguments. MinMaxStretchImage's
  are public API.
