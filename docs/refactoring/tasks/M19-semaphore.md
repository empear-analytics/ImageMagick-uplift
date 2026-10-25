# M19 - refactor `MagickCore/semaphore.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **8.81** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 483 lines |
| Functions with findings | 4 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/semaphore.c
```

The score must read 8.81. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Complex Method | 1 |
| Complex Conditional | 1 |
| Code Duplication | 3 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `AcquireSemaphoreMemory` | 129 | 9 | - | - | static | - | 15883 | 7 of 10 |
| 2 | `LockSemaphoreInfo` | 293 | - | - | - | public | - | 15883 | 1 of 1 |
| 3 | `RelinquishSemaphoreInfo` | 351 | - | - | - | public | - | 15883 | 2 of 3 |
| 4 | `UnlockSemaphoreInfo` | 449 | - | - | - | public | - | 15883 | 1 of 1 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `AcquireSemaphoreMemory` (line 129)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 143, `div_to_mul`: `if ((count == 0) || (quantum != (size/count)))`
  - base line 151, `lt_to_le`: `if ((size == 0) || (alignment < sizeof(void *)) || (extent < size))`
  - base line 151, `lt_to_le`: `if ((size == 0) || (alignment < sizeof(void *)) || (extent < size))`

Commit message: `refactor(MagickCore/semaphore.c): simplify AcquireSemaphoreMemory`

#### Step 2: `LockSemaphoreInfo` (line 293)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/semaphore.c): simplify LockSemaphoreInfo`

#### Step 3: `RelinquishSemaphoreInfo` (line 351)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/semaphore.c): simplify RelinquishSemaphoreInfo`

#### Step 4: `UnlockSemaphoreInfo` (line 449)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/semaphore.c): simplify UnlockSemaphoreInfo`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/semaphore.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/semaphore.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/semaphore.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
