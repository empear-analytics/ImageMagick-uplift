# M55 - refactor `MagickCore/exception.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **5.88** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1159 lines |
| Functions with findings | 15 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/exception.c
```

The score must read 5.88. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Overall Code Complexity | file |
| Complex Method | 2 |
| Complex Conditional | 1 |
| Code Duplication | 12 |
| Large Method | 1 |
| Excess Number of Function Arguments | 1 |
| String Heavy Function Arguments | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `ExceptionSeverityToTag` | 571 | 65 | - | - | static | - | 1468 | - |
| 2 | `ThrowException` | 994 | 13 | - | - | public | - | 1468 | 9 of 13 |
| 3 | `AcquireExceptionInfo` | 116 | - | - | - | public | - | 15883 | 1 of 1 |
| 4 | `DestroyExceptionElement` | 151 | - | - | - | static | - | 1459 | 0 of 1 |
| 5 | `DefaultErrorHandler` | 296 | - | - | - | static | - | 1085 | 2 of 2 |
| 6 | `DefaultFatalErrorHandler` | 338 | - | - | - | static | - | 9 | 3 of 4 |
| 7 | `DefaultWarningHandler` | 380 | - | - | - | static | - | 189 | 1 of 1 |
| 8 | `InitializeExceptionInfo` | 735 | - | - | - | public | - | 15883 | - |
| 9 | `MagickError` | 773 | - | - | - | public | - | 1085 | - |
| 10 | `MagickFatalError` | 809 | - | - | - | public | - | 9 | 1 of 1 |
| 11 | `SetFatalErrorHandler` | 912 | - | - | - | public | - | 1495 | 2 of 2 |
| 12 | `SetWarningHandler` | 949 | - | - | - | public | - | 3 | 2 of 3 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `ExceptionSeverityToTag` (line 571)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.

Commit message: `refactor(MagickCore/exception.c): simplify ExceptionSeverityToTag`

#### Step 2: `ThrowException` (line 994)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1007, `gt_to_ge`: `if (GetNumberOfElementsInLinkedList(exceptions) > MaxExceptionList)`
  - base line 1009, `lt_to_le`: `if (severity < ErrorException)`
  - base line 1015, `ge_to_lt`: `if (p->severity >= ErrorException)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/exception.c): simplify ThrowException`

#### Step 3: `AcquireExceptionInfo` (line 116)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/exception.c): simplify AcquireExceptionInfo`

#### Step 4: `DestroyExceptionElement` (line 151)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/exception.c): simplify DestroyExceptionElement`

### Wave 2

#### Step 5: `DefaultErrorHandler` (line 296)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/exception.c): simplify DefaultErrorHandler`

#### Step 6: `DefaultFatalErrorHandler` (line 338)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Only 9 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 348, `remove_void_call`: `MagickCoreTerminus();`

Commit message: `refactor(MagickCore/exception.c): simplify DefaultFatalErrorHandler`

#### Step 7: `DefaultWarningHandler` (line 380)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/exception.c): simplify DefaultWarningHandler`

#### Step 8: `InitializeExceptionInfo` (line 735)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/exception.c): simplify InitializeExceptionInfo`

### Wave 3

#### Step 9: `MagickError` (line 773)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/exception.c): simplify MagickError`

#### Step 10: `MagickFatalError` (line 809)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 9 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/exception.c): simplify MagickFatalError`

#### Step 11: `SetFatalErrorHandler` (line 912)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/exception.c): simplify SetFatalErrorHandler`

#### Step 12: `SetWarningHandler` (line 949)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 3 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/exception.c): simplify SetWarningHandler`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/exception.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/exception.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/exception.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
