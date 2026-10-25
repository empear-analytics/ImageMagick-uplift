# M49 - refactor `MagickCore/string.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **3.49** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2684 lines |
| Functions with findings | 37 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/string.c
```

The score must read 3.49. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 9 |
| Deep, Nested Complexity | 3 |
| Overall Code Complexity | file |
| Complex Method | 10 |
| Code Duplication | 26 |
| Large Method | 2 |
| Excess Number of Function Arguments | 1 |
| String Heavy Function Arguments | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `StringToStrings` | 2349 | 30 | 4 | 11 | public | - | 71 | 47 of 50 |
| 2 | `InterpretSiPrefixValue` | 1309 | 34 | 4 | 2 | public | - | 15883 | 11 of 12 |
| 3 | `StringToArgv` | 2041 | 28 | - | 6 | public | - | 56 | 37 of 37 |
| 4 | `PrintStringInfo` | 1485 | 13 | - | 5 | public | - | 3 | 28 of 28 |
| 5 | `StripMagickString` | 2563 | 13 | - | 2 | public | - | 12 | 18 of 18 |
| 6 | `StringToArrayOfDoubles` | 2156 | 12 | - | 2 | public | - | 455 | 13 of 13 |
| 7 | `EscapeString` | 891 | 10 | - | 2 | public | - | 16 | 7 of 14 |
| 8 | `StringToken` | 2255 | - | 4 | - | public | - | 35 | 4 of 4 |
| 9 | `CopyMagickString` | 728 | 9 | - | 2 | public | - | 15883 | 14 of 20 |
| 10 | `SubstituteString` | 2628 | 9 | - | 2 | public | - | 500 | 15 of 15 |
| 11 | `ConfigureFileToStringInfo` | 571 | 10 | - | - | public | - | 15883 | 16 of 17 |
| 12 | `AcquireStringInfoContainer` | 142 | - | - | - | static | - | 15883 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `StringToStrings` (line 2349)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 11 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2370, `lt_to_le`: `if (((int) ((unsigned char) *p) < 32) &&`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/string.c): simplify StringToStrings`

#### Step 2: `InterpretSiPrefixValue` (line 1309)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1321, `ge_to_gt`: `if ((*q >= 'E') && (*q <= 'z'))`

Commit message: `refactor(MagickCore/string.c): simplify InterpretSiPrefixValue`

#### Step 3: `StringToArgv` (line 2041)

- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/string.c): simplify StringToArgv`

#### Step 4: `PrintStringInfo` (line 1485)

- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 3 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/string.c): simplify PrintStringInfo`

### Wave 2

#### Step 5: `StripMagickString` (line 2563)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/string.c): simplify StripMagickString`

#### Step 6: `StringToArrayOfDoubles` (line 2156)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/string.c): simplify StringToArrayOfDoubles`

#### Step 7: `EscapeString` (line 891)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- 7 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/string.c): simplify EscapeString`

#### Step 8: `StringToken` (line 2255)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/string.c): simplify StringToken`

### Wave 3

#### Step 9: `CopyMagickString` (line 728)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 745, `sub_to_add`: `return((size_t) (p-source-1));`
  - base line 745, `sub_to_add`: `return((size_t) (p-source-1));`
  - base line 751, `sub_to_add`: `return((size_t) (p-source-1));`
  - base line 751, `sub_to_add`: `return((size_t) (p-source-1));`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/string.c): simplify CopyMagickString`

#### Step 10: `SubstituteString` (line 2628)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/string.c): simplify SubstituteString`

#### Step 11: `ConfigureFileToStringInfo` (line 571)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/string.c): simplify ConfigureFileToStringInfo`

#### Step 12: `AcquireStringInfoContainer` (line 142)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/string.c): simplify AcquireStringInfoContainer`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/string.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/string.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/string.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, evening)

**Result: Code Health 3.49 → 3.76** (`cs-mcp` 1.5.12). Branch `campaign/M49-string`,
stacked on `campaign/M48-constitute`. Five E commits, all oracle-verified (the target
functions are ready or careful, and no open survivor sits in a moved block):
- `GetSiPrefixExponent` (InterpretSiPrefixValue): the 25-case SI prefix switch,
  value-returning. InterpretSiPrefixValue goes from cc 34 to 9.
- `CountArgvArguments` (StringToArgv). StringToArgv leaves Large Method.
- `PrintStringInfoHexLine` (PrintStringInfo). PrintStringInfo leaves Complex Method.
  This gave the biggest step, 3.54 → 3.72.
- `CountTextLines` (StringToStrings, careful).
- `SkipValueSeparator` (StringToArrayOfDoubles), value-returning.

The oracle diverged on gaps/c5651ffad8 (the log-map tie) during the
InterpretSiPrefixValue run. It diverges the same way on the unedited file.

**Not moved, on purpose.** Two blocks contain `ThrowFatalException`: EscapeString's
counting loop and StringToArgv's allocation failure. The macro expands
`GetMagickModule()`, which records `__func__`, so moving either block would change the
function name in the fatal message. Blocks with this macro, or any
`Throw*Exception`, cannot use E unless the playbook decides that this message is
not observable behaviour.

**Bug, reported and not fixed: StripMagickString's return value.** The function
returns `(size_t) (q-p+1)` after the last loop has moved `p` to the new terminator
(`message + (q-p0+1)`). So it returns the number of leading characters stripped
(`p0-message`), not the length of the stripped string. Example: `"ab"` returns 0. No
caller in MagickCore, MagickWand, coders or utilities uses the value (all cast it to
`void`), but the function is public API.

**What is left, at 3.76.**
- `StringToStrings` (careful, cc 28, 131 lines, 11 bumps). The hex branch's line
  formatter writes both `p` and `q`, and both cleanup loops are unreached by any case.
- Code Duplication across 26 small accessors. They share the
  `assert`/`signature`/`LogMagickEvent` prologue. Folding that into a helper would move
  `assert` lines and the `GetMagickModule()` location, which raises the question above.
- The file-level String Heavy Function Arguments finding.
- `FormatMagickSize` (5 arguments, public API).
