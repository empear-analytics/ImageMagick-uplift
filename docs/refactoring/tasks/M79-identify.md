# M79 - refactor `MagickCore/identify.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **3.21** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1827 lines |
| Functions with findings | 12 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/identify.c
```

The score must read 3.21. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 9 |
| Deep, Nested Complexity | 3 |
| Overall Code Complexity | file |
| Complex Method | 8 |
| Large Method | 4 |
| Excess Number of Function Arguments | 3 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `IdentifyImage` | 480 | 150 | 5 | 26 | public | - | 283 | 263 of 296 |
| 2 | `PrintChannelLocations` | 287 | 14 | 4 | 3 | static | - | 6 | 18 of 18 |
| 3 | `PrintChannelPerceptualHash` | 392 | 11 | 4 | 3 | static | - | 18 | 21 of 21 |
| 4 | `GetLocationStatistics` | 137 | 12 | - | 3 | static | - | 6 | 16 of 16 |
| 5 | `PrintChannelMoments` | 357 | - | - | - | static | - | 18 | 6 of 6 |
| 6 | `PrintChannelStatistics` | 449 | - | - | - | static | - | 197 | 10 of 10 |
| 7 | `GetIPTCTagName` | 707 | 55 | - | - | not compiled | - | - | - |
| 8 | `PrintChannelDepths` | 517 | 23 | - | 3 | not compiled | - | - | - |
| 9 | `PrintChannelMomentsTable` | 626 | 18 | - | 2 | not compiled | - | - | 1 of 1 |
| 10 | `AccumulateLocationExtrema` | 41 | 9 | - | 2 | not compiled | - | - | - |
| 11 | `PrintImageDirectory` | 950 | - | - | 2 | not compiled | - | - | - |
| 12 | `PrintConvexHullAndBoundingBox` | 1010 | - | - | 2 | not compiled | - | - | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `IdentifyImage` (line 480)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 26 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 33 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 543, `ne_to_eq`: `(void) FormatLocaleFile(file,"    Cyan: %.17g-bit\n",(double)`
  - base line 631, `ne_to_eq`: ``
  - base line 635, `remove_void_call`: `scale=(double) ((1UL << image->depth)-1);`
  - base line 652, `ne_to_eq`: `}`
  - base line 652, `ne_to_eq`: `}`
  - base line 681, `le_to_lt`: `label[MagickPathExtent];`
  - and 27 more (`tools/oracle/mutate.py --function IdentifyImage`)

Commit message: `refactor(MagickCore/identify.c): simplify IdentifyImage`

#### Step 2: `PrintChannelLocations` (line 287)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (7 arguments).
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/identify.c): simplify PrintChannelLocations`

#### Step 3: `PrintChannelPerceptualHash` (line 392)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/identify.c): simplify PrintChannelPerceptualHash`

#### Step 4: `GetLocationStatistics` (line 137)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/identify.c): simplify GetLocationStatistics`

### Wave 2

#### Step 5: `PrintChannelMoments` (line 357)

- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).

Commit message: `refactor(MagickCore/identify.c): simplify PrintChannelMoments`

#### Step 6: `PrintChannelStatistics` (line 449)

- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).

Commit message: `refactor(MagickCore/identify.c): simplify PrintChannelStatistics`

#### Step 7: `GetIPTCTagName` (line 707)

- **This function is not compiled in this build** (it sits in a preprocessor branch for a platform or a library that is not present). Nothing can check it here. Do not change it.

#### Step 8: `PrintChannelDepths` (line 517)

- **This function is not compiled in this build** (it sits in a preprocessor branch for a platform or a library that is not present). Nothing can check it here. Do not change it.

### Wave 3

#### Step 9: `PrintChannelMomentsTable` (line 626)

- **This function is not compiled in this build** (it sits in a preprocessor branch for a platform or a library that is not present). Nothing can check it here. Do not change it.

#### Step 10: `AccumulateLocationExtrema` (line 41)

- **This function is not compiled in this build** (it sits in a preprocessor branch for a platform or a library that is not present). Nothing can check it here. Do not change it.

#### Step 11: `PrintImageDirectory` (line 950)

- **This function is not compiled in this build** (it sits in a preprocessor branch for a platform or a library that is not present). Nothing can check it here. Do not change it.

#### Step 12: `PrintConvexHullAndBoundingBox` (line 1010)

- **This function is not compiled in this build** (it sits in a preprocessor branch for a platform or a library that is not present). Nothing can check it here. Do not change it.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/identify.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/identify.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/identify.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-26, Mac, night): second pass

**Result: Code Health 3.21, unchanged. Tried and reverted.** IdentifyImage (ready,
cc 150, 696 lines) has a non-verbose summary block whose every path ends in
`return(ferror(file) ...)`. Moving it under Recipe E step 7 (`PrintImageSummary`,
5 arguments, its own `buffer`) cut IdentifyImage to cc 134 and 634 lines. But the file
went 3.21 → 3.20, because the helper brings cc 17, three bumps and an argument-count
finding of its own.

**CodeScene parser quirk, worth reporting to the CodeScene developers.**
AccumulateLocationExtrema, the first function in identify.c, is measured from line 41
(right after the license header) instead of from its signature. The whole `#include`
block is counted into it, so CodeScene reports it as an 87-line Large Method. The same
happened with an experimental helper above image.c's AcquireImage (M78). Any file whose
first function is small is penalised by its include block.
