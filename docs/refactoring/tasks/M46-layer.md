# M46 - refactor `MagickCore/layer.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **3.04** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2098 lines |
| Functions with findings | 13 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/layer.c
```

The score must read 3.04. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 10 |
| Deep, Nested Complexity | 1 |
| Overall Code Complexity | file |
| Complex Method | 11 |
| Complex Conditional | 4 |
| Large Method | 7 |
| Excess Number of Function Arguments | 2 |
| Overall Function Size | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `OptimizeLayerFrames` | 945 | 64 | 6 | 16 | static | - | 45 | 97 of 97 |
| 2 | `CompareImagesBounds` | 618 | 27 | - | 8 | static | - | 67 | 49 of 49 |
| 3 | `MergeImageLayers` | 1947 | 31 | - | 4 | public | - | 74 | 46 of 46 |
| 4 | `CoalesceImages` | 231 | 21 | - | 4 | public | - | 39 | 24 of 24 |
| 5 | `CompareImagesLayers` | 772 | 18 | - | 2 | public | - | 19 | 17 of 17 |
| 6 | `CompositeLayers` | 1817 | 10 | - | 3 | public | - | 7 | 12 of 12 |
| 7 | `DisposeImages` | 399 | 13 | - | 2 | public | - | 6 | 10 of 10 |
| 8 | `OptimizeImageTransparency` | 1499 | 13 | - | 2 | public | - | 23 | 19 of 19 |
| 9 | `IsBoundsCleared` | 167 | 10 | - | 2 | static | - | 63 | 15 of 15 |
| 10 | `RemoveZeroDelayLayers` | 1712 | - | - | 2 | public | - | 5 | 6 of 6 |
| 11 | `ComparePixels` | 547 | 10 | - | - | static | - | 67 | 13 of 13 |
| 12 | `RemoveDuplicateLayers` | 1627 | 9 | - | - | public | - | 6 | 14 of 14 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `OptimizeLayerFrames` (line 945)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 16 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/layer.c): simplify OptimizeLayerFrames`

#### Step 2: `CompareImagesBounds` (line 618)

- **Recipe E (extract function)** - 8 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/layer.c): simplify CompareImagesBounds`

#### Step 3: `MergeImageLayers` (line 1947)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/layer.c): simplify MergeImageLayers`

#### Step 4: `CoalesceImages` (line 231)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/layer.c): simplify CoalesceImages`

### Wave 2

#### Step 5: `CompareImagesLayers` (line 772)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/layer.c): simplify CompareImagesLayers`

#### Step 6: `CompositeLayers` (line 1817)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- Only 7 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/layer.c): simplify CompositeLayers`

#### Step 7: `DisposeImages` (line 399)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/layer.c): simplify DisposeImages`

#### Step 8: `OptimizeImageTransparency` (line 1499)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/layer.c): simplify OptimizeImageTransparency`

### Wave 3

#### Step 9: `IsBoundsCleared` (line 167)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/layer.c): simplify IsBoundsCleared`

#### Step 10: `RemoveZeroDelayLayers` (line 1712)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- Public function: its name and signature must not change.
- Only 5 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/layer.c): simplify RemoveZeroDelayLayers`

#### Step 11: `ComparePixels` (line 547)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/layer.c): simplify ComparePixels`

#### Step 12: `RemoveDuplicateLayers` (line 1627)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/layer.c): simplify RemoveDuplicateLayers`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/layer.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/layer.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/layer.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, evening)

**Result: Code Health 3.04 → 3.77** (`cs-mcp` 1.5.12). Branch `campaign/M46-layer`,
stacked on `campaign/M45-annotate`. The branch also carries one style commit for
annotate.c, wrapping a 93-column line that M45 left in `IsUnauthorizedFontFile`. 3
refactor commits, all Recipe P and all oracle-verified, adding 4 predicates. Complex
Conditional leaves the file.

**The predicates:**
- `IsUnchangedFrameBounds` (CompareImagesLayers).
- `IsPartialPageFrame` and `IsBetterFrameBounds` (OptimizeLayerFrames, +0.47).
  `IsBetterFrameBounds` takes five arguments.
- `IsDifferentLayerGeometry` (RemoveDuplicateLayers).

**Tool change.** The predicate generator now also wraps at `==`, `!=` and commas, after
the annotate.c line that had no `&&` or `||` to break at.

**What is left, at 3.77 (just below the Red/Yellow line).**
- `CompareImagesBounds` (8 bumps): four near-identical scan loops. A shared helper
  would need seven parameters, because the two PixelInfo scratch values carry
  initialised fields into the loop. That needs Recipe B.
- `OptimizeLayerFrames` (16 bumps, nesting 6, 378 lines): the disposal-method search,
  with many shared locals.
- `MergeImageLayers` and `CoalesceImages`.
