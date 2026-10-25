# M38 - refactor `MagickCore/fx.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.14** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 4537 lines |
| Functions with findings | 25 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/fx.c
```

The score must read 1.14. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 2 |
| Bumpy Road Ahead | 13 |
| Deep, Nested Complexity | 6 |
| Overall Code Complexity | file |
| Complex Method | 13 |
| Complex Conditional | 42 |
| Large Method | 9 |
| Excess Number of Function Arguments | 5 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `GetFunction` | 1847 | 131 | 4 | 16 | static | - | 577 | 12 of 14 |
| 2 | `ExecuteRPN` | 3321 | - | 8 | 17 | static | 1 | 767 | 13 of 22 |
| 3 | `TranslateExpression` | 2716 | 67 | - | 9 | static | - | 821 | 3 of 8 |
| 4 | `GetOperand` | 2243 | - | 5 | 12 | static | - | 821 | 8 of 11 |
| 5 | `FxImage` | 4385 | 30 | 4 | 4 | public | 2 | 625 | 3 of 5 |
| 6 | `ImageStat` | 3069 | 50 | - | 3 | static | - | 26 | - |
| 7 | `GetProperty` | 1619 | 15 | 4 | 4 | static | - | 11 | - |
| 8 | `DumpRPN` | 1129 | 30 | - | 3 | static | - | 10 | - |
| 9 | `GetOperator` | 2604 | 18 | - | 4 | static | - | 531 | 2 of 3 |
| 10 | `GetChannelQualifier` | 1512 | 14 | 4 | - | static | - | 207 | 0 of 1 |
| 11 | `AcquireFxInfoPrivate` | 4230 | 19 | - | 2 | static | - | 821 | 0 of 3 |
| 12 | `GetToken` | 1233 | 13 | - | 3 | static | - | 821 | 1 of 1 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `GetFunction` (line 1847)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 16 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2095, `eq_to_ne`: `if (fe == fU || fe == fV || fe == fS) {`
  - base line 2174, `sub_to_add`: `ImgAttrs[iaQual-(int) FirstImgAttr].str,`

Commit message: `refactor(MagickCore/fx.c): simplify GetFunction`

#### Step 2: `ExecuteRPN` (line 3321)

- **Recipe G (guard clauses)** - nesting is 8, target is under 4.
- **Recipe E (extract function)** - 17 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 8 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3355, `remove_void_call`: `ConvertRGBToHSL (`
  - base line 3475, `ge_to_gt`: `regA = (regA >= regB) ? 1.0 : 0.0;`
  - base line 3557, `mul_to_div`: `fxFltType gamma = 2.0 * __j1((double) (MagickPI*regA)) / (MagickPI*regA);`
  - base line 3599, `gt_to_le`: `else if (regA > 1.0) regA = 1.0;`
  - base line 3749, `lt_to_ge`: `if ((int) pel->channel_qual < 0) {`
  - base line 3878, `add_to_sub`: `fx = imgx + regB;`
  - and 2 more (`tools/oracle/mutate.py --function ExecuteRPN`)
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/fx.c): simplify ExecuteRPN`

#### Step 3: `TranslateExpression` (line 2716)

- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 5 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2747, `remove_void_call`: `SkipSpaces (pfx);`
  - base line 2763, `remove_void_call`: `SkipSpaces (pfx);`
  - base line 2769, `remove_void_call`: `SkipSpaces (pfx);`
  - base line 2849, `remove_void_call`: `SkipSpaces (pfx);`
  - base line 2906, `lt_to_ge`: `if (UserSymNdx0 < 0) {`

Commit message: `refactor(MagickCore/fx.c): simplify TranslateExpression`

#### Step 4: `GetOperand` (line 2243)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 12 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2288, `remove_void_call`: `SkipSpaces (pfx);`
  - base line 2508, `eq_to_ne`: `if (pfx->token == tailptr) {`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/fx.c): simplify GetOperand`

### Wave 2

#### Step 5: `FxImage` (line 4385)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4482, `remove_void_call`: `SetPixelChannel (fx_image, channel, p[i], q);`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/fx.c): simplify FxImage`

#### Step 6: `ImageStat` (line 3069)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/fx.c): simplify ImageStat`

#### Step 7: `GetProperty` (line 1619)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/fx.c): simplify GetProperty`

#### Step 8: `DumpRPN` (line 1129)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/fx.c): simplify DumpRPN`

### Wave 3

#### Step 9: `GetOperator` (line 2604)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/fx.c): simplify GetOperator`

#### Step 10: `GetChannelQualifier` (line 1512)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1524, `ge_to_gt`: `if (op >= (int) FirstImgAttr && op <= (int) ((OperatorE)aNull) &&`

Commit message: `refactor(MagickCore/fx.c): simplify GetChannelQualifier`

#### Step 11: `AcquireFxInfoPrivate` (line 4230)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4304, `remove_void_call`: `DumpTables (stderr);`
  - base line 4305, `remove_void_call`: `DumpUserSymbols (pfx, stderr);`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/fx.c): simplify AcquireFxInfoPrivate`

#### Step 12: `GetToken` (line 1233)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/fx.c): simplify GetToken`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/fx.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/fx.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/fx.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
