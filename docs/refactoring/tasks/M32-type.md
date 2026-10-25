# M32 - refactor `MagickCore/type.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **3.60** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1419 lines |
| Functions with findings | 9 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/type.c
```

The score must read 3.60. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Brain Method | 1 |
| Bumpy Road Ahead | 3 |
| Deep, Nested Complexity | 1 |
| Overall Code Complexity | file |
| Complex Method | 4 |
| Complex Conditional | 7 |
| Code Duplication | 4 |
| Large Method | 3 |
| Excess Number of Function Arguments | 2 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `LoadTypeCache` | 1071 | - | 6 | 13 | static | - | 89 | 49 of 49 |
| 2 | `GetTypeInfoByFamily` | 322 | 52 | - | 5 | public | - | 74 | 44 of 44 |
| 3 | `LoadFontConfigFonts` | 715 | 43 | - | 3 | public | - | 89 | 52 of 52 |
| 4 | `ListTypeInfo` | 937 | 13 | - | - | public | - | 16 | 13 of 13 |
| 5 | `DestroyTypeNode` | 163 | 10 | - | - | static | - | 89 | - |
| 6 | `GetTypeInfoList` | 549 | - | - | - | public | - | 16 | 11 of 11 |
| 7 | `TypeComponentGenesis` | 1384 | - | - | - | public | - | 15883 | 1 of 1 |
| 8 | `TypeComponentTerminus` | 1409 | - | - | - | public | - | 15883 | 6 of 6 |
| 9 | `GetTypeList` | 643 | - | - | - | public | - | 0 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `LoadTypeCache` (line 1071)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 13 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).

Commit message: `refactor(MagickCore/type.c): simplify LoadTypeCache`

#### Step 2: `GetTypeInfoByFamily` (line 322)

- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/type.c): simplify GetTypeInfoByFamily`

#### Step 3: `LoadFontConfigFonts` (line 715)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/type.c): simplify LoadFontConfigFonts`

#### Step 4: `ListTypeInfo` (line 937)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/type.c): simplify ListTypeInfo`

### Wave 2

#### Step 5: `DestroyTypeNode` (line 163)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/type.c): simplify DestroyTypeNode`

#### Step 6: `GetTypeInfoList` (line 549)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/type.c): simplify GetTypeInfoList`

#### Step 7: `TypeComponentGenesis` (line 1384)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/type.c): simplify TypeComponentGenesis`

#### Step 8: `TypeComponentTerminus` (line 1409)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/type.c): simplify TypeComponentTerminus`

### Wave 3

#### Step 9: `GetTypeList` (line 643)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/type.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/type.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/type.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
