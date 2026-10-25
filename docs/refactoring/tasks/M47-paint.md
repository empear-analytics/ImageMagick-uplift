# M47 - refactor `MagickCore/paint.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **3.06** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1300 lines |
| Functions with findings | 6 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/paint.c
```

The score must read 3.06. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Brain Method | 2 |
| Bumpy Road Ahead | 6 |
| Deep, Nested Complexity | 3 |
| Overall Code Complexity | file |
| Complex Method | 4 |
| Complex Conditional | 1 |
| Code Duplication | 2 |
| Large Method | 6 |
| Excess Number of Function Arguments | 5 |
| Overall Function Size | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `OilPaintImage` | 706 | 28 | 5 | 5 | public | 2 | 15 | 50 of 50 |
| 2 | `FloodfillPaintImage` | 112 | - | 5 | 9 | public | 1 | 41 | 84 of 84 |
| 3 | `GradientImage` | 425 | 32 | - | 3 | public | - | 73 | 66 of 68 |
| 4 | `OpaquePaintImage` | 927 | - | 4 | 3 | public | 2 | 36 | 24 of 24 |
| 5 | `TransparentPaintImage` | 1077 | 14 | - | 3 | public | 2 | 14 | 13 of 13 |
| 6 | `TransparentPaintImageChroma` | 1208 | 20 | - | 3 | public | 2 | 0 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `OilPaintImage` (line 706)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/paint.c): simplify OilPaintImage`

#### Step 2: `FloodfillPaintImage` (line 112)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/paint.c): simplify FloodfillPaintImage`

#### Step 3: `GradientImage` (line 425)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/paint.c): simplify GradientImage`

#### Step 4: `OpaquePaintImage` (line 927)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/paint.c): simplify OpaquePaintImage`

### Wave 2

#### Step 5: `TransparentPaintImage` (line 1077)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/paint.c): simplify TransparentPaintImage`

#### Step 6: `TransparentPaintImageChroma` (line 1208)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/paint.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/paint.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/paint.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
