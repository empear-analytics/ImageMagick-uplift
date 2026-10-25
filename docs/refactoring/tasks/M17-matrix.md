# M17 - refactor `MagickCore/matrix.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **5.92** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1147 lines |
| Functions with findings | 6 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/matrix.c
```

The score must read 5.92. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 3 |
| Deep, Nested Complexity | 1 |
| Overall Code Complexity | file |
| Complex Method | 3 |
| Complex Conditional | 3 |
| Code Duplication | 2 |
| Large Method | 3 |
| Excess Number of Function Arguments | 1 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `GaussJordanElimination` | 716 | 37 | 5 | 6 | public | - | 202 | 17 of 19 |
| 2 | `MatrixToImage` | 884 | 21 | - | 3 | public | 1 | 23 | - |
| 3 | `AcquireMatrixInfo` | 190 | 16 | - | 3 | public | - | 126 | 0 of 3 |
| 4 | `WriteMatrixElements` | 135 | - | - | - | static | - | 15 | - |
| 5 | `ReadMatrixElements` | 479 | - | - | - | static | - | 10 | - |
| 6 | `LeastSquaresAddTerms` | 631 | - | - | - | public | - | 162 | 1 of 1 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `GaussJordanElimination` (line 716)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 849, `lt_to_ge`: `for (i=0; i < (ssize_t) rank; i++)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/matrix.c): simplify GaussJordanElimination`

#### Step 2: `MatrixToImage` (line 884)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/matrix.c): simplify MatrixToImage`

#### Step 3: `AcquireMatrixInfo` (line 190)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 226, `ne_to_eq`: `if ((status != MagickFalse) &&`
  - base line 250, `eq_to_ne`: `if (status == MagickFalse)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/matrix.c): simplify AcquireMatrixInfo`

#### Step 4: `WriteMatrixElements` (line 135)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/matrix.c): simplify WriteMatrixElements`

### Wave 2

#### Step 5: `ReadMatrixElements` (line 479)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/matrix.c): simplify ReadMatrixElements`

#### Step 6: `LeastSquaresAddTerms` (line 631)

- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/matrix.c): simplify LeastSquaresAddTerms`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/matrix.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/matrix.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/matrix.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, overnight)

**Result: Code Health 5.92 → 6.76** (`cs-mcp` 1.5.12). Branch `campaign/M17-matrix`,
stacked on `campaign/M16-channel` (PR #22). 4 refactor commits, all guard-verified: 3
Recipe P and 1 Recipe E. The mutation reports hold 14 mutants for the whole file and none
for `GaussJordanElimination`, so the oracle's cases (126 for `AcquireMatrixInfo`, 202 for
`GaussJordanElimination`) ran on every step but have no measured power there.

**One category leaves the file:** Complex Conditional. The predicates are
`IsMatrixMemoryAvailable`, `IsPivotMemoryMissing` and `IsSingularPivot`. Then
`GaussJordanElimination`'s copy-back moves into `CopyBackMatrix`.

| Steps | Recipe | Score |
| --- | --- | --- |
| the three compound tests as predicates | P | 5.92 → 6.73 |
| `GaussJordanElimination`'s copy-back | E | → 6.76 |

**Whole catalogue** at the branch tip against `8a4c272f1`: 15,884 cases, 1 diverged
(`gaps/c5651ffad8`, the known log-map tie).

**What is left, at 6.76.**
- `GaussJordanElimination` (nesting 5): its row swap and unscramble loops expand
  swap macros that the function defines inside itself and uses in both places, and its
  elimination loop needs five arguments.
- `MatrixToImage`: its range loop and its scale selection each set two variables
  together (Recipe O, which needs oracle-level evidence the function lacks), and its
  pixel loop needs six arguments.
- `AcquireMatrixInfo`'s memory and disk paths `return` from inside.
- The duplicated `ReadMatrixElements` and `WriteMatrixElements` have no case.

**Possible bug, reported, not fixed.** In `MatrixToImage`'s pixel loop, a failed
`GetMatrixElement()` takes `continue` before `q` is advanced, so later values land one
pixel early.
