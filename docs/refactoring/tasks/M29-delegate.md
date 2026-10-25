# M29 - refactor `MagickCore/delegate.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **2.15** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2374 lines |
| Functions with findings | 12 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/delegate.c
```

The score must read 2.15. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 2 |
| Bumpy Road Ahead | 6 |
| Deep, Nested Complexity | 3 |
| Overall Code Complexity | file |
| Complex Method | 9 |
| Complex Conditional | 6 |
| Large Method | 5 |
| Excess Number of Function Arguments | 4 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `LoadDelegateCache` | 2106 | - | 6 | 12 | static | - | 357 | 51 of 51 |
| 2 | `InvokeDelegate` | 1745 | - | 4 | 11 | public | - | 12 | 35 of 44 |
| 3 | `InterpretDelegateProperties` | 956 | 50 | 4 | - | static | - | 15 | 14 of 18 |
| 4 | `ExternalDelegateCommand` | 393 | 32 | - | 7 | public | - | 35 | 38 of 38 |
| 5 | `GetMagickPropertyLetter` | 608 | 61 | - | - | static | - | 12237 | 20 of 24 |
| 6 | `GetDelegateInfo` | 1264 | 19 | - | 6 | public | - | 356 | 22 of 22 |
| 7 | `IsExecutableToken` | 361 | 18 | - | 2 | static | - | 34 | 18 of 18 |
| 8 | `ListDelegateInfo` | 2007 | 17 | - | 2 | public | - | 4 | 18 of 19 |
| 9 | `CopyDelegateFile` | 1668 | 14 | - | - | static | - | 12 | 13 of 16 |
| 10 | `GetDelegateInfoList` | 1387 | 9 | - | - | public | - | 4 | 11 of 13 |
| 11 | `GetDelegateCommand` | 1156 | - | - | - | public | - | 3 | 4 of 6 |
| 12 | `GetDelegateList` | 1485 | 10 | - | - | public | - | 0 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `LoadDelegateCache` (line 2106)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 12 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).

Commit message: `refactor(MagickCore/delegate.c): simplify LoadDelegateCache`

#### Step 2: `InvokeDelegate` (line 1745)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 11 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- 9 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/delegate.c): simplify InvokeDelegate`

#### Step 3: `InterpretDelegateProperties` (line 956)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/delegate.c): simplify InterpretDelegateProperties`

#### Step 4: `ExternalDelegateCommand` (line 393)

- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/delegate.c): simplify ExternalDelegateCommand`

### Wave 2

#### Step 5: `GetMagickPropertyLetter` (line 608)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/delegate.c): simplify GetMagickPropertyLetter`

#### Step 6: `GetDelegateInfo` (line 1264)

- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/delegate.c): simplify GetDelegateInfo`

#### Step 7: `IsExecutableToken` (line 361)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/delegate.c): simplify IsExecutableToken`

#### Step 8: `ListDelegateInfo` (line 2007)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 4 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/delegate.c): simplify ListDelegateInfo`

### Wave 3

#### Step 9: `CopyDelegateFile` (line 1668)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/delegate.c): simplify CopyDelegateFile`

#### Step 10: `GetDelegateInfoList` (line 1387)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 4 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/delegate.c): simplify GetDelegateInfoList`

#### Step 11: `GetDelegateCommand` (line 1156)

- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- Only 3 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1198, `ne_to_eq`: `for (i=0; commands[i] != (char *) NULL; i++)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/delegate.c): simplify GetDelegateCommand`

#### Step 12: `GetDelegateList` (line 1485)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/delegate.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/delegate.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/delegate.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, morning)

**Result: Code Health 2.15 → 2.38** (`cs-mcp` 1.5.12). Branch `campaign/M29-delegate`,
stacked on `campaign/M28-stream`. 6 refactor commits, all oracle-verified. Every
function in the file is ready in the merged mutation reports.

**Recipe P; Complex Conditional leaves the file:**
- `IsIncludeElementOpen` (the same predicate as color.c's).
- `IsTwoStepDelegate`.
- `IsWindowsDrivePath` and `IsDotRelativePath`.
- `IsPropertyEnd`.
- `IsListedDelegate`. This site was written `if(` with no space, so the predicate tool
  could not match it and the replacement was made by hand.

**Recipe E:** `GetIncludePath` (the same helper as color.c's).

**Not done:**
- `AcquireDelegateInfo` (probe: flat, 4 lines).
- `LoadDelegateCache`'s attribute `switch`. Its `command` case holds an uncompiled
  `MAGICKCORE_WINDOWS_SUPPORT` block.
- `GetDelegateInfo`'s matching chain. Its paired nested `if`s would have to merge into
  one condition, which is not a catalogue transformation.

**Whole catalogue:** see the batch PR.

**What is left, at 2.38.**
- `LoadDelegateCache` (12 bumps, Brain Method): the DOCTYPE ladder (it cost 0.04 in
  color.c), the include loop and the attribute `switch`.
- `InvokeDelegate` (11 bumps) and `ExternalDelegateCommand` (7). Each has a long chain
  of steps with shared state.
- `GetMagickPropertyLetter` (cc 61): one letter `switch`.
- Four 5-argument functions (Recipe B).
