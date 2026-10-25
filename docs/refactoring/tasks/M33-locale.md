# M33 - refactor `MagickCore/locale.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **4.38** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1717 lines |
| Functions with findings | 15 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/locale.c
```

The score must read 4.38. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Brain Method | 1 |
| Bumpy Road Ahead | 3 |
| Deep, Nested Complexity | 1 |
| Overall Code Complexity | file |
| Complex Method | 3 |
| Complex Conditional | 1 |
| Code Duplication | 12 |
| Large Method | 1 |
| Excess Number of Function Arguments | 1 |
| String Heavy Function Arguments | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `LoadLocaleCache` | 1169 | - | 6 | 13 | static | - | 1495 | 52 of 65 |
| 2 | `IsLocaleTreeInstantiated` | 922 | 11 | - | 2 | static | - | 1495 | 6 of 12 |
| 3 | `ListLocaleInfo` | 1047 | 10 | - | 2 | public | - | 9 | 10 of 10 |
| 4 | `LocaleNCompare` | 1572 | 10 | - | - | public | - | 15883 | 10 of 10 |
| 5 | `AcquireCLocale` | 135 | - | - | - | static | - | 15883 | 1 of 1 |
| 6 | `DestroyCLocale` | 261 | - | - | - | static | - | 15883 | 2 of 2 |
| 7 | `FormatLocaleFileList` | 337 | - | - | - | public | - | 2626 | 0 of 1 |
| 8 | `FormatLocaleFile` | 382 | - | - | - | public | - | 2626 | - |
| 9 | `FormatLocaleStringList` | 428 | - | - | - | public | - | 15883 | 2 of 4 |
| 10 | `FormatLocaleString` | 480 | - | - | - | public | - | 15883 | - |
| 11 | `GetLocaleInfoList` | 594 | - | - | - | public | - | 9 | 10 of 10 |
| 12 | `GetLocaleList` | 690 | - | - | - | public | - | 2 | 9 of 11 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `LoadLocaleCache` (line 1169)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 13 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- The oracle missed 8 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1277, `ne_to_eq`: `while (((*token != '/') && (*(token+1) != '>')) && (*q != '\0'))`
  - base line 1277, `ne_to_eq`: `while (((*token != '/') && (*(token+1) != '>')) && (*q != '\0'))`
  - base line 1284, `eq_to_ne`: `if (LocaleCompare(keyword,"locale") == 0)`
  - base line 1286, `ne_to_eq`: `if (LocaleCompare(locale,token) != 0)`
  - base line 1290, `eq_to_ne`: `if (LocaleCompare(keyword,"file") == 0)`
  - base line 1302, `remove_void_call`: `GetPathComponent(filename,HeadPath,path);`
  - and 2 more (`tools/oracle/mutate.py --function LoadLocaleCache`)
- 5 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/locale.c): simplify LoadLocaleCache`

#### Step 2: `IsLocaleTreeInstantiated` (line 922)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- 6 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/locale.c): simplify IsLocaleTreeInstantiated`

#### Step 3: `ListLocaleInfo` (line 1047)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 9 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/locale.c): simplify ListLocaleInfo`

#### Step 4: `LocaleNCompare` (line 1572)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/locale.c): simplify LocaleNCompare`

### Wave 2

#### Step 5: `AcquireCLocale` (line 135)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/locale.c): simplify AcquireCLocale`

#### Step 6: `DestroyCLocale` (line 261)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/locale.c): simplify DestroyCLocale`

#### Step 7: `FormatLocaleFileList` (line 337)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/locale.c): simplify FormatLocaleFileList`

#### Step 8: `FormatLocaleFile` (line 382)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/locale.c): simplify FormatLocaleFile`

### Wave 3

#### Step 9: `FormatLocaleStringList` (line 428)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 475, `lt_to_ge`: `if (n < 0)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/locale.c): simplify FormatLocaleStringList`

#### Step 10: `FormatLocaleString` (line 480)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/locale.c): simplify FormatLocaleString`

#### Step 11: `GetLocaleInfoList` (line 594)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 9 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/locale.c): simplify GetLocaleInfoList`

#### Step 12: `GetLocaleList` (line 690)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 2 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 727, `remove_void_call`: `qsort((void *) messages,(size_t) i,sizeof(*messages),LocaleTagCompare);`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/locale.c): simplify GetLocaleList`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/locale.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/locale.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/locale.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, morning)

**Result: Code Health 4.38 → 4.43** (`cs-mcp` 1.5.12). Branch `campaign/M33-locale`,
stacked on `campaign/M32-type`. 2 refactor commits, both oracle-verified (LoadLocaleCache
is ready, 66 of 66 killed):
- `IsIncludeElementOpen` (P): Complex Conditional leaves the file.
- `GetIncludePath` (E).

locale.c already carries String Heavy Function Arguments, so the string parameters cost
nothing.

**Not done:** `AcquireLocaleInfo` (probe: flat).

**What is left, at 4.43.**
- `LoadLocaleCache` (13 bumps, Brain Method, 6 arguments). Its bumps are the element
  blocks (`<locale`, `<message`, subpath), each ending in `continue` to the token loop,
  and the message-trimming code, which moves the shared cursor `q`.
- The Code Duplication findings: the `FormatLocale*` and `Locale*Compare` pairs are
  public wrappers whose bodies differ in the call they make.
