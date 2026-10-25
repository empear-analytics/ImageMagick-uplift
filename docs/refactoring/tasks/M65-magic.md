# M65 - refactor `MagickCore/magic.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **7.88** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 824 lines |
| Functions with findings | 4 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/magic.c
```

The score must read 7.88. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 2 |
| Deep, Nested Complexity | 1 |
| Overall Code Complexity | file |
| Complex Method | 1 |
| Code Duplication | 2 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `ListMagicInfo` | 711 | - | 4 | 2 | public | - | 1 | 12 of 12 |
| 2 | `GetMagicInfo` | 303 | 12 | - | 3 | public | - | 14057 | 17 of 17 |
| 3 | `GetMagicInfoList` | 474 | - | - | - | public | - | 1 | 10 of 10 |
| 4 | `GetMagicList` | 571 | - | - | - | public | - | 0 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `ListMagicInfo` (line 711)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- Public function: its name and signature must not change.
- Only 1 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/magic.c): simplify ListMagicInfo`

#### Step 2: `GetMagicInfo` (line 303)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/magic.c): simplify GetMagicInfo`

#### Step 3: `GetMagicInfoList` (line 474)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 1 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/magic.c): simplify GetMagicInfoList`

#### Step 4: `GetMagicList` (line 571)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/magic.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/magic.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/magic.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
