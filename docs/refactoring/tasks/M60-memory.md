# M60 - refactor `MagickCore/memory.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **6.87** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1714 lines |
| Functions with findings | 11 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/memory.c
```

The score must read 6.87. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 2 |
| Deep, Nested Complexity | 2 |
| Complex Method | 5 |
| Code Duplication | 6 |
| Large Method | 1 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `AcquireVirtualMemory` | 694 | 20 | 5 | 5 | public | - | 14738 | 18 of 18 |
| 2 | `AcquireMagickMemory` | 532 | 13 | 4 | 2 | public | - | 15883 | 1 of 1 |
| 3 | `ShredMagickMemory` | 1661 | 10 | - | - | public | - | 14996 | 13 of 15 |
| 4 | `ResizeMagickMemory` | 1423 | 9 | - | - | public | - | 12964 | 3 of 3 |
| 5 | `AcquireQuantumMemory` | 654 | - | - | - | public | - | 15883 | 3 of 3 |
| 6 | `GetMaxMemoryRequestFromPolicy` | 1033 | - | - | - | static | - | 15883 | 1 of 1 |
| 7 | `GetMaxProfileSizeFromPolicy` | 1088 | - | - | - | static | - | 191 | 1 of 1 |
| 8 | `ResizeQuantumMemory` | 1483 | - | - | - | public | - | 12964 | 3 of 3 |
| 9 | `CopyMagickMemory` | 826 | 13 | - | - | public | - | 0 | - |
| 10 | `GetMagickMemoryMethods` | 1002 | - | - | - | public | - | 0 | - |
| 11 | `SetMagickMemoryMethods` | 1565 | - | - | - | public | - | 0 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `AcquireVirtualMemory` (line 694)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/memory.c): simplify AcquireVirtualMemory`

#### Step 2: `AcquireMagickMemory` (line 532)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/memory.c): simplify AcquireMagickMemory`

#### Step 3: `ShredMagickMemory` (line 1661)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1691, `post_inc_to_post_dec`: `for (i=0; i < passes; i++)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/memory.c): simplify ShredMagickMemory`

#### Step 4: `ResizeMagickMemory` (line 1423)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/memory.c): simplify ResizeMagickMemory`

### Wave 2

#### Step 5: `AcquireQuantumMemory` (line 654)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/memory.c): simplify AcquireQuantumMemory`

#### Step 6: `GetMaxMemoryRequestFromPolicy` (line 1033)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/memory.c): simplify GetMaxMemoryRequestFromPolicy`

#### Step 7: `GetMaxProfileSizeFromPolicy` (line 1088)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/memory.c): simplify GetMaxProfileSizeFromPolicy`

#### Step 8: `ResizeQuantumMemory` (line 1483)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/memory.c): simplify ResizeQuantumMemory`

### Wave 3

#### Step 9: `CopyMagickMemory` (line 826)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 10: `GetMagickMemoryMethods` (line 1002)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 11: `SetMagickMemoryMethods` (line 1565)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/memory.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/memory.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/memory.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
