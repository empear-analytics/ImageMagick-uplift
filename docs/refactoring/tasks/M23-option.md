# M23 - refactor `MagickCore/option.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **4.00** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 3550 lines |
| Functions with findings | 13 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/option.c
```

The score must read 4.00. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 2 |
| Deep, Nested Complexity | 2 |
| Overall Code Complexity | file |
| Lines of Declarations in a Single File | file |
| Complex Method | 5 |
| Complex Conditional | 7 |
| Code Duplication | 7 |
| Large Method | 4 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `GetCommandOptionFlags` | 2679 | 28 | 4 | 5 | public | - | 1406 | 13 of 13 |
| 2 | `ParseCommandOption` | 3248 | 28 | 4 | 5 | public | - | 15883 | 15 of 19 |
| 3 | `GetOptionInfo` | 2603 | 69 | - | - | static | - | 15883 | - |
| 4 | `ParseChannelOption` | 3124 | 24 | - | - | public | - | 265 | 3 of 3 |
| 5 | `IsCommandOption` | 2900 | 12 | - | - | public | - | 12884 | 2 of 3 |
| 6 | `ParsePixelChannelOption` | 3354 | - | - | - | public | - | 60 | 3 of 3 |
| 7 | `DeleteImageOption` | 2484 | - | - | - | public | - | 12845 | - |
| 8 | `DestroyImageOptions` | 2520 | - | - | - | public | - | 15553 | 1 of 2 |
| 9 | `GetImageOption` | 2559 | - | - | - | public | - | 15331 | 1 of 2 |
| 10 | `GetNextImageOption` | 2865 | - | - | - | public | - | 1 | - |
| 11 | `ResetImageOptionIterator` | 3479 | - | - | - | public | - | 1 | - |
| 12 | `RemoveImageOption` | 3403 | - | - | - | public | - | 0 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `GetCommandOptionFlags` (line 2679)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/option.c): simplify GetCommandOptionFlags`

#### Step 2: `ParseCommandOption` (line 3248)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3288, `ne_to_eq`: `(*p != '\0'))`
  - base line 3297, `ge_to_gt`: `if ((q-token) >= (MagickPathExtent-1))`
  - base line 3308, `ne_to_eq`: `(strchr(token+1,'_') != (char *) NULL)))`
  - base line 3311, `sub_to_add`: `(void) CopyMagickString(q,q+1,MagickPathExtent-strlen(q));`

Commit message: `refactor(MagickCore/option.c): simplify ParseCommandOption`

#### Step 3: `GetOptionInfo` (line 2603)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.

Commit message: `refactor(MagickCore/option.c): simplify GetOptionInfo`

#### Step 4: `ParseChannelOption` (line 3124)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/option.c): simplify ParseChannelOption`

### Wave 2

#### Step 5: `IsCommandOption` (line 2900)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/option.c): simplify IsCommandOption`

#### Step 6: `ParsePixelChannelOption` (line 3354)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/option.c): simplify ParsePixelChannelOption`

#### Step 7: `DeleteImageOption` (line 2484)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/option.c): simplify DeleteImageOption`

#### Step 8: `DestroyImageOptions` (line 2520)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/option.c): simplify DestroyImageOptions`

### Wave 3

#### Step 9: `GetImageOption` (line 2559)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/option.c): simplify GetImageOption`

#### Step 10: `GetNextImageOption` (line 2865)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 1 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/option.c): simplify GetNextImageOption`

#### Step 11: `ResetImageOptionIterator` (line 3479)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 1 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/option.c): simplify ResetImageOptionIterator`

#### Step 12: `RemoveImageOption` (line 3403)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/option.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/option.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/option.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
