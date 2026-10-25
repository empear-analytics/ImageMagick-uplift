# M30 - refactor `MagickCore/log.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **3.32** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1978 lines |
| Functions with findings | 13 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/log.c
```

The score must read 3.32. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 2 |
| Bumpy Road Ahead | 7 |
| Deep, Nested Complexity | 3 |
| Overall Code Complexity | file |
| Complex Method | 3 |
| Complex Conditional | 2 |
| Code Duplication | 6 |
| Large Method | 4 |
| Excess Number of Function Arguments | 3 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `LoadLogCache` | 911 | - | 6 | 14 | static | - | 15883 | 54 of 54 |
| 2 | `ListLogInfo` | 804 | 15 | 4 | 3 | public | - | 28 | 17 of 18 |
| 3 | `LogMagickEventList` | 1587 | 20 | - | 4 | public | - | 22 | 28 of 32 |
| 4 | `TranslateEvent` | 1365 | - | 4 | 2 | static | - | 21 | 18 of 35 |
| 5 | `FormatLogFilename` | 1274 | 14 | - | 2 | static | - | 6 | 5 of 5 |
| 6 | `GetLogInfo` | 443 | - | - | 2 | static | - | 15883 | 11 of 11 |
| 7 | `ParseLogHandlers` | 1770 | - | - | 2 | static | - | 15883 | 8 of 8 |
| 8 | `GetLogEventMask` | 402 | - | - | - | public | - | 15613 | 1 of 1 |
| 9 | `GetLogInfoList` | 530 | - | - | - | public | - | 28 | 10 of 12 |
| 10 | `LogComponentGenesis` | 1172 | - | - | - | public | - | 15883 | 1 of 1 |
| 11 | `SetLogMethod` | 1927 | - | - | - | public | - | 1 | 3 of 3 |
| 12 | `GetLogList` | 625 | - | - | - | public | - | 0 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `LoadLogCache` (line 911)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 14 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).

Commit message: `refactor(MagickCore/log.c): simplify LoadLogCache`

#### Step 2: `ListLogInfo` (line 804)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/log.c): simplify ListLogInfo`

#### Step 3: `LogMagickEventList` (line 1587)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/log.c): simplify LogMagickEventList`

#### Step 4: `TranslateEvent` (line 1365)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- 17 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/log.c): simplify TranslateEvent`

### Wave 2

#### Step 5: `FormatLogFilename` (line 1274)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/log.c): simplify FormatLogFilename`

#### Step 6: `GetLogInfo` (line 443)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.

Commit message: `refactor(MagickCore/log.c): simplify GetLogInfo`

#### Step 7: `ParseLogHandlers` (line 1770)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/log.c): simplify ParseLogHandlers`

#### Step 8: `GetLogEventMask` (line 402)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/log.c): simplify GetLogEventMask`

### Wave 3

#### Step 9: `GetLogInfoList` (line 530)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 571, `remove_void_call`: `qsort((void *) preferences,(size_t) i,sizeof(*preferences),LogInfoCompare);`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/log.c): simplify GetLogInfoList`

#### Step 10: `LogComponentGenesis` (line 1172)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/log.c): simplify LogComponentGenesis`

#### Step 11: `SetLogMethod` (line 1927)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 1 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/log.c): simplify SetLogMethod`

#### Step 12: `GetLogList` (line 625)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/log.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/log.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/log.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
