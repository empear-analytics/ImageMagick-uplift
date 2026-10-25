# M58 - refactor `MagickCore/list.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **6.23** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1505 lines |
| Functions with findings | 19 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/list.c
```

The score must read 6.23. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 3 |
| Deep, Nested Complexity | 1 |
| Overall Code Complexity | file |
| Complex Method | 2 |
| Code Duplication | 16 |
| Large Method | 2 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `CloneImages` | 206 | 20 | 5 | 4 | public | - | 48 | 28 of 28 |
| 2 | `DeleteImages` | 372 | 20 | - | 4 | public | - | 22 | 28 of 28 |
| 3 | `GetImageFromList` | 629 | - | - | 2 | public | - | 166 | 8 of 8 |
| 4 | `DeleteImageFromList` | 326 | - | - | - | public | - | 12940 | 1 of 1 |
| 5 | `DestroyImageList` | 486 | - | - | - | public | - | 13263 | 4 of 4 |
| 6 | `GetFirstImageInList` | 585 | - | - | - | public | - | 14911 | 2 of 2 |
| 7 | `GetImageIndexInList` | 681 | - | - | - | public | - | 853 | 3 of 3 |
| 8 | `GetImageListLength` | 720 | - | - | - | public | - | 13542 | 4 of 4 |
| 9 | `GetLastImageInList` | 761 | - | - | - | public | - | 13556 | 2 of 2 |
| 10 | `GetNextImageInList` | 795 | - | - | - | public | - | 14928 | 2 of 2 |
| 11 | `GetPreviousImageInList` | 827 | - | - | - | public | - | 12076 | 1 of 1 |
| 12 | `PrependImageToList` | 991 | - | - | - | public | - | 1 | 2 of 2 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `CloneImages` (line 206)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/list.c): simplify CloneImages`

#### Step 2: `DeleteImages` (line 372)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/list.c): simplify DeleteImages`

#### Step 3: `GetImageFromList` (line 629)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/list.c): simplify GetImageFromList`

#### Step 4: `DeleteImageFromList` (line 326)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/list.c): simplify DeleteImageFromList`

### Wave 2

#### Step 5: `DestroyImageList` (line 486)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/list.c): simplify DestroyImageList`

#### Step 6: `GetFirstImageInList` (line 585)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/list.c): simplify GetFirstImageInList`

#### Step 7: `GetImageIndexInList` (line 681)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/list.c): simplify GetImageIndexInList`

#### Step 8: `GetImageListLength` (line 720)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/list.c): simplify GetImageListLength`

### Wave 3

#### Step 9: `GetLastImageInList` (line 761)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/list.c): simplify GetLastImageInList`

#### Step 10: `GetNextImageInList` (line 795)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/list.c): simplify GetNextImageInList`

#### Step 11: `GetPreviousImageInList` (line 827)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/list.c): simplify GetPreviousImageInList`

#### Step 12: `PrependImageToList` (line 991)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 1 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/list.c): simplify PrependImageToList`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/list.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/list.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/list.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
