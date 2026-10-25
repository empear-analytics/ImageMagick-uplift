# M52 - refactor `MagickCore/resource.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **4.27** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1599 lines |
| Functions with findings | 8 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/resource.c
```

The score must read 4.27. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 4 |
| Overall Code Complexity | file |
| Complex Method | 7 |
| Complex Conditional | 1 |
| Large Method | 6 |
| Overall Function Size | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `AcquireMagickResource` | 187 | 58 | - | 7 | public | - | 15035 | 7 of 15 |
| 2 | `SetMagickResourceLimit` | 1429 | 40 | - | 2 | public | - | 15883 | 5 of 7 |
| 3 | `ResourceComponentGenesis` | 1181 | 25 | - | 2 | public | - | 15883 | 3 of 4 |
| 4 | `AcquireUniqueFileResource` | 464 | 17 | - | 2 | public | - | 240 | 5 of 9 |
| 5 | `GetMagickResource` | 595 | 26 | - | - | public | - | 62 | 2 of 3 |
| 6 | `RelinquishMagickResource` | 995 | 24 | - | - | public | - | 15014 | 2 of 2 |
| 7 | `GetMagickResourceLimit` | 766 | 15 | - | - | public | - | 15098 | 2 of 3 |
| 8 | `IsFileResourceIdentityValid` | 715 | - | - | - | public | - | 14711 | 3 of 4 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `AcquireMagickResource` (line 187)

- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 8 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 230, `lt_to_le`: `if ((limit == MagickResourceInfinity) || (size < limit))`
  - base line 238, `le_to_lt`: `if (resource_info.disk <= (MagickOffsetMax-request))`
  - base line 242, `lt_to_ge`: `(resource_info.disk < (MagickOffsetType) limit))`
  - base line 276, `eq_to_ne`: `if ((limit == MagickResourceInfinity) || (size < limit))`
  - base line 276, `lt_to_le`: `if ((limit == MagickResourceInfinity) || (size < limit))`
  - base line 288, `lt_to_le`: `(resource_info.map < (MagickOffsetType) limit))`
  - and 2 more (`tools/oracle/mutate.py --function AcquireMagickResource`)

Commit message: `refactor(MagickCore/resource.c): simplify AcquireMagickResource`

#### Step 2: `SetMagickResourceLimit` (line 1429)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1540, `eq_to_ne`: `if (resource_info.thread_limit == 0)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resource.c): simplify SetMagickResourceLimit`

#### Step 3: `ResourceComponentGenesis` (line 1181)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1282, `lt_to_le`: `if (files < 0)`

Commit message: `refactor(MagickCore/resource.c): simplify ResourceComponentGenesis`

#### Step 4: `AcquireUniqueFileResource` (line 464)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 517, `lt_to_ge`: `for (j=0; j < (ssize_t) GetStringInfoLength(key); j++)`
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resource.c): simplify AcquireUniqueFileResource`

### Wave 2

#### Step 5: `GetMagickResource` (line 595)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resource.c): simplify GetMagickResource`

#### Step 6: `RelinquishMagickResource` (line 995)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/resource.c): simplify RelinquishMagickResource`

#### Step 7: `GetMagickResourceLimit` (line 766)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resource.c): simplify GetMagickResourceLimit`

#### Step 8: `IsFileResourceIdentityValid` (line 715)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 734, `ne_to_eq`: `if (status != MagickFalse)`

Commit message: `refactor(MagickCore/resource.c): simplify IsFileResourceIdentityValid`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/resource.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/resource.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/resource.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
