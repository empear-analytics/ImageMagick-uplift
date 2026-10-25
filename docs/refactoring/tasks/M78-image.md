# M78 - refactor `MagickCore/image.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **2.09** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 4447 lines |
| Functions with findings | 36 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/image.c
```

The score must read 2.09. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 2 |
| Bumpy Road Ahead | 17 |
| Deep, Nested Complexity | 4 |
| Overall Code Complexity | file |
| Complex Method | 24 |
| Code Duplication | 15 |
| Large Method | 12 |
| Excess Number of Function Arguments | 6 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `SetImageInfo` | 2857 | 56 | 4 | 11 | public | - | 14958 | 79 of 79 |
| 2 | `SyncImageSettings` | 4121 | 50 | - | 6 | public | - | 15082 | 54 of 54 |
| 3 | `AppendImages` | 434 | 28 | - | 5 | public | 1 | 93 | 34 of 34 |
| 4 | `InterpretImageFilename` | 1703 | - | 4 | 5 | public | - | 12658 | 42 of 46 |
| 5 | `IsHighDynamicRangeImage` | 1866 | 15 | 4 | 2 | public | 1 | 205 | 17 of 19 |
| 6 | `AcquireImage` | 135 | 23 | - | 4 | public | - | 15098 | 38 of 38 |
| 7 | `SetImageMask` | 3292 | 23 | - | 3 | public | 1 | 265 | 19 of 20 |
| 8 | `GetImageMask` | 1447 | 21 | - | 3 | public | 1 | 6 | 17 of 19 |
| 9 | `SetImageRegionMask` | 3453 | 21 | - | 3 | public | 1 | 3 | 22 of 22 |
| 10 | `CopyImagePixels` | 1068 | - | 4 | 3 | public | 1 | 54 | 33 of 33 |
| 11 | `ResetImagePage` | 2228 | 13 | - | 4 | public | - | 319 | 15 of 15 |
| 12 | `SmushImages` | 3793 | 18 | - | 3 | public | - | 17 | 20 of 20 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `SetImageInfo` (line 2857)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 11 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/image.c): simplify SetImageInfo`

#### Step 2: `SyncImageSettings` (line 4121)

- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/image.c): simplify SyncImageSettings`

#### Step 3: `AppendImages` (line 434)

- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/image.c): simplify AppendImages`

#### Step 4: `InterpretImageFilename` (line 1703)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1768, `le_to_lt`: `specifier_start=cursor;`
  - base line 1820, `sub_to_add`: `extent,`
  - base line 1829, `sub_to_add`: `pattern[extent]='\0';`
  - base line 1829, `ge_to_lt`: `pattern[extent]='\0';`

Commit message: `refactor(MagickCore/image.c): simplify InterpretImageFilename`

### Wave 2

#### Step 5: `IsHighDynamicRangeImage` (line 1866)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1883, `ne_to_eq`: `%`
  - base line 1890, `lt_to_le`: `ExceptionInfo *exception)`

Commit message: `refactor(MagickCore/image.c): simplify IsHighDynamicRangeImage`

#### Step 6: `AcquireImage` (line 135)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/image.c): simplify AcquireImage`

#### Step 7: `SetImageMask` (line 3292)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3307, `ne_to_eq`: `%                                                                             %`

Commit message: `refactor(MagickCore/image.c): simplify SetImageMask`

#### Step 8: `GetImageMask` (line 1447)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1469, `ne_to_eq`: `*mask_image;`
  - base line 1526, `lt_to_le`: `ssize_t`

Commit message: `refactor(MagickCore/image.c): simplify GetImageMask`

### Wave 3

#### Step 9: `SetImageRegionMask` (line 3453)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- Only 3 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/image.c): simplify SetImageRegionMask`

#### Step 10: `CopyImagePixels` (line 1068)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/image.c): simplify CopyImagePixels`

#### Step 11: `ResetImagePage` (line 2228)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/image.c): simplify ResetImagePage`

#### Step 12: `SmushImages` (line 3793)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/image.c): simplify SmushImages`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/image.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/image.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/image.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
