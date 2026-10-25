# M24 - refactor `MagickCore/transform.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.99** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2600 lines |
| Functions with findings | 14 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/transform.c
```

The score must read 1.99. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 12 |
| Deep, Nested Complexity | 11 |
| Overall Code Complexity | file |
| Complex Method | 14 |
| Complex Conditional | 3 |
| Code Duplication | 4 |
| Large Method | 12 |
| Excess Number of Function Arguments | 1 |
| Overall Function Size | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `SpliceImage` | 1730 | 63 | 4 | 8 | public | 4 | 36 | 77 of 77 |
| 2 | `ChopImage` | 192 | 46 | 5 | 6 | public | 4 | 27 | 73 of 73 |
| 3 | `CropImage` | 542 | 45 | 4 | 6 | public | 2 | 1165 | 67 of 83 |
| 4 | `CropImageToTiles` | 800 | 41 | 4 | 6 | public | - | 208 | 66 of 66 |
| 5 | `TransverseImage` | 2307 | 21 | 4 | 3 | public | 2 | 13 | 27 of 27 |
| 6 | `FlipImage` | 1203 | 20 | 4 | 3 | public | 2 | 28 | 23 of 23 |
| 7 | `FlopImage` | 1348 | 20 | 4 | 3 | public | 2 | 33 | 22 of 22 |
| 8 | `TransposeImage` | 2158 | 19 | 4 | 3 | public | 2 | 24 | 22 of 22 |
| 9 | `TrimImage` | 2465 | 32 | - | 3 | public | - | 85 | 29 of 29 |
| 10 | `CopyImageRegion` | 1499 | 14 | 4 | 2 | static | 1 | 26 | 19 of 19 |
| 11 | `AutoOrientImage` | 103 | 12 | - | - | public | - | 13 | 1 of 1 |
| 12 | `TransformImage` | 2078 | 9 | - | - | public | - | 25 | 4 of 4 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `SpliceImage` (line 1730)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 8 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 4 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/transform.c): simplify SpliceImage`

#### Step 2: `ChopImage` (line 192)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 4 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/transform.c): simplify ChopImage`

#### Step 3: `CropImage` (line 542)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 745, `ne_to_eq`: `if (status != MagickFalse)`
  - base line 754, `remove_void_call`: `AppendImageProfileProperty(crop_image,"hdrgm","hdrgm:Transform",`
- 14 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/transform.c): simplify CropImage`

#### Step 4: `CropImageToTiles` (line 800)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/transform.c): simplify CropImageToTiles`

### Wave 2

#### Step 5: `TransverseImage` (line 2307)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/transform.c): simplify TransverseImage`

#### Step 6: `FlipImage` (line 1203)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/transform.c): simplify FlipImage`

#### Step 7: `FlopImage` (line 1348)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/transform.c): simplify FlopImage`

#### Step 8: `TransposeImage` (line 2158)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/transform.c): simplify TransposeImage`

### Wave 3

#### Step 9: `TrimImage` (line 2465)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/transform.c): simplify TrimImage`

#### Step 10: `CopyImageRegion` (line 1499)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (9 arguments).
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.

Commit message: `refactor(MagickCore/transform.c): simplify CopyImageRegion`

#### Step 11: `AutoOrientImage` (line 103)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/transform.c): simplify AutoOrientImage`

#### Step 12: `TransformImage` (line 2078)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/transform.c): simplify TransformImage`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/transform.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/transform.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/transform.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, morning)

**Result: Code Health 1.99 → 2.65** (`cs-mcp` 1.5.12). Branch `campaign/M24-transform`,
stacked on `campaign/M23-option` (PR #29). 12 refactor commits, all oracle-verified.
Every function touched is ready or careful in the merged mutation reports, and no
survivor sits on a moved line.

**Shared channel loop (+0.37).** The per-pixel channel copy loop appeared eleven times:
`ChopImage` ×2, `CropImage`, `FlipImage`, `FlopImage`, `SpliceImage` ×4,
`TransposeImage` and `TransverseImage`. The copies are identical up to whitespace and the
destination's name.
- One Recipe E commit (ChopImage's first loop) introduced
  `CopyPixelChannelsTo(image,destination,p,q)`.
- Seven Recipe D commits replaced the other ten copies, one function each.
- Every loop is inside a parallel row loop. The helper writes only through the row's
  `q`, as the loop did.

**Predicates:** three Recipe P commits (`IsChopOutsideImage`, `IsCropOutsideImage`,
`IsSingleRegionCrop`). Complex Conditional leaves the file.

**Gravity switch:** one Recipe E commit moved `SpliceImage`'s gravity `switch` into
`ApplySpliceGravity`. It returns the updated `splice_geometry`. The score stayed flat;
the commit is kept under rule 2 (cc 47 → 36, 228 → 178 lines).

**Possible bug, reported, not fixed:** in that switch, `WestGravity` adds
`splice_geometry.width/2` to `y`. Every other vertical offset uses `height`.

**Whole catalogue** at the branch tip against `8a4c272f1`: 15,884 cases, 1 diverged
(`gaps/c5651ffad8`, the known log-map tie).

**What is left, at 2.65.**
- The remaining bumps are almost all the parallel row loops' bodies (the column loops
  and the progress blocks with their `omp atomic`), in every transform. They need Recipe
  K or T, or a progress helper that cannot carry the pragma.
- Flip, Flop, Transpose and Transverse are still reported as duplicates. Their row-loop
  skeletons are alike, but each differs in its pixel addressing.
- `CopyImageRegion` takes 9 arguments (Recipe B).
- `ExcerptImage` and `ConsolidateCMYKImages` have no oracle case.
