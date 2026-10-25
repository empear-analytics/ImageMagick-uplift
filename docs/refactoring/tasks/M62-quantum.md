# M62 - refactor `MagickCore/quantum.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **7.56** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1111 lines |
| Functions with findings | 8 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/quantum.c
```

The score must read 7.56. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 1 |
| Overall Code Complexity | file |
| Complex Method | 4 |
| Code Duplication | 4 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `GetQuantumType` | 528 | 10 | - | 3 | public | - | 11925 | 6 of 6 |
| 2 | `GetQuantumExtent` | 308 | 16 | - | - | public | - | 14435 | 4 of 7 |
| 3 | `SetQuantumImageType` | 858 | 15 | - | - | public | - | 13905 | 0 of 1 |
| 4 | `GetQuantumInfo` | 423 | 11 | - | - | public | - | 14648 | 3 of 6 |
| 5 | `SetQuantumDepth` | 669 | - | - | - | public | - | 14648 | 4 of 4 |
| 6 | `SetQuantumEndian` | 725 | - | - | - | public | - | 1925 | 0 of 1 |
| 7 | `SetQuantumFormat` | 819 | - | - | - | public | - | 3904 | - |
| 8 | `SetQuantumPad` | 1001 | - | - | - | public | - | 44 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `GetQuantumType` (line 528)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/quantum.c): simplify GetQuantumType`

#### Step 2: `GetQuantumExtent` (line 308)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 335, `mul_to_div`: `return((size_t) (channels*image->columns*((quantum_info->depth+7)/8))+`
  - base line 335, `add_to_sub`: `return((size_t) (channels*image->columns*((quantum_info->depth+7)/8))+`
  - base line 337, `add_to_sub`: `return((size_t) ((channels*image->columns*quantum_info->depth+7)/8)+`

Commit message: `refactor(MagickCore/quantum.c): simplify GetQuantumExtent`

#### Step 3: `SetQuantumImageType` (line 858)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantum.c): simplify SetQuantumImageType`

#### Step 4: `GetQuantumInfo` (line 423)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 449, `eq_to_ne`: `if ((quantum_info->minimum == 0.0) && (quantum_info->maximum == 0.0))`
  - base line 465, `eq_to_ne`: `quantum_info->min_is_white=LocaleCompare(option,"min-is-white") == 0 ?`
  - base line 468, `remove_void_call`: `ResetQuantumState(quantum_info);`

Commit message: `refactor(MagickCore/quantum.c): simplify GetQuantumInfo`

### Wave 2

#### Step 5: `SetQuantumDepth` (line 669)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/quantum.c): simplify SetQuantumDepth`

#### Step 6: `SetQuantumEndian` (line 725)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantum.c): simplify SetQuantumEndian`

#### Step 7: `SetQuantumFormat` (line 819)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/quantum.c): simplify SetQuantumFormat`

#### Step 8: `SetQuantumPad` (line 1001)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/quantum.c): simplify SetQuantumPad`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/quantum.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/quantum.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/quantum.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
