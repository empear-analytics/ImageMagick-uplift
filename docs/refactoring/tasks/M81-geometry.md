# M81 - refactor `MagickCore/geometry.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **3.38** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1956 lines |
| Functions with findings | 11 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/geometry.c
```

The score must read 3.38. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 1 |
| Bumpy Road Ahead | 6 |
| Deep, Nested Complexity | 2 |
| Overall Code Complexity | file |
| Complex Method | 7 |
| Large Method | 3 |
| Excess Number of Function Arguments | 5 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `ParseGeometry` | 919 | 84 | 4 | 14 | public | - | 14705 | 92 of 93 |
| 2 | `GetGeometry` | 204 | - | 4 | 7 | public | - | 12938 | 59 of 59 |
| 3 | `ParseMetaGeometry` | 1475 | 15 | - | 3 | public | - | 1587 | 74 of 74 |
| 4 | `ParseGravityGeometry` | 1315 | 17 | - | 2 | public | - | 579 | 52 of 52 |
| 5 | `GravityAdjustGeometry` | 549 | 25 | - | - | public | - | 1359 | 14 of 14 |
| 6 | `ParseAffineGeometry` | 815 | 11 | - | - | public | - | 5 | 6 of 6 |
| 7 | `GetPageGeometry` | 474 | - | - | 2 | public | - | 11939 | 6 of 6 |
| 8 | `IsSceneGeometry` | 671 | 9 | - | - | public | - | 14918 | 10 of 10 |
| 9 | `GetAspectScaleFactor` | 1582 | 13 | - | 3 | not compiled | - | - | - |
| 10 | `IsMetaGeometryAspectKept` | 1512 | - | - | - | not compiled | - | - | - |
| 11 | `LimitGeometryArea` | 1622 | - | - | - | not compiled | - | - | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `ParseGeometry` (line 919)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 14 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1159, `eq_to_ne`: `}`

Commit message: `refactor(MagickCore/geometry.c): simplify ParseGeometry`

#### Step 2: `GetGeometry` (line 204)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/geometry.c): simplify GetGeometry`

#### Step 3: `ParseMetaGeometry` (line 1475)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/geometry.c): simplify ParseMetaGeometry`

#### Step 4: `ParseGravityGeometry` (line 1315)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/geometry.c): simplify ParseGravityGeometry`

### Wave 2

#### Step 5: `GravityAdjustGeometry` (line 549)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/geometry.c): simplify GravityAdjustGeometry`

#### Step 6: `ParseAffineGeometry` (line 815)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 5 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/geometry.c): simplify ParseAffineGeometry`

#### Step 7: `GetPageGeometry` (line 474)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/geometry.c): simplify GetPageGeometry`

#### Step 8: `IsSceneGeometry` (line 671)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/geometry.c): simplify IsSceneGeometry`

### Wave 3

#### Step 9: `GetAspectScaleFactor` (line 1582)

- **This function is not compiled in this build** (it sits in a preprocessor branch for a platform or a library that is not present). Nothing can check it here. Do not change it.

#### Step 10: `IsMetaGeometryAspectKept` (line 1512)

- **This function is not compiled in this build** (it sits in a preprocessor branch for a platform or a library that is not present). Nothing can check it here. Do not change it.

#### Step 11: `LimitGeometryArea` (line 1622)

- **This function is not compiled in this build** (it sits in a preprocessor branch for a platform or a library that is not present). Nothing can check it here. Do not change it.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/geometry.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/geometry.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/geometry.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
