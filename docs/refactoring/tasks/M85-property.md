# M85 - refactor `MagickCore/property.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.92** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 4940 lines |
| Functions with findings | 19 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/property.c
```

The score must read 1.92. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 12 |
| Deep, Nested Complexity | 8 |
| Overall Code Complexity | file |
| Complex Method | 12 |
| Complex Conditional | 3 |
| Code Duplication | 7 |
| Large Method | 11 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `InterpretImageProperties` | 3697 | 139 | 6 | 15 | public | - | 12374 | 60 of 100 |
| 2 | `GetEXIFProperty` | 862 | 131 | 7 | 14 | static | - | 14911 | 158 of 158 |
| 3 | `GetMagickProperty` | 2908 | 125 | 4 | 16 | public | - | 189 | 102 of 102 |
| 4 | `SetImageProperty` | 4415 | 99 | 4 | 15 | public | - | 11529 | 45 of 83 |
| 5 | `Get8BIMProperty` | 606 | 46 | 4 | 6 | static | - | 221 | 51 of 51 |
| 6 | `GetMagickPropertyLetter` | 2495 | 79 | - | 2 | static | - | 12237 | 19 of 33 |
| 7 | `GetXMPProperty` | 1850 | 20 | 6 | 4 | static | - | 14911 | 30 of 30 |
| 8 | `TracePSClippath` | 1952 | 30 | 4 | 4 | static | - | 5 | 35 of 35 |
| 9 | `TraceSVGClippath` | 2169 | 20 | 4 | 3 | static | - | 12 | 24 of 24 |
| 10 | `GetICCProperty` | 1748 | 15 | - | 5 | static | - | 14911 | 21 of 21 |
| 11 | `GetImageProperty` | 2306 | 25 | - | 2 | public | - | 14947 | 26 of 26 |
| 12 | `GetIPTCProperty` | 437 | 14 | - | 2 | static | - | 14911 | 21 of 21 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `InterpretImageProperties` (line 3697)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 15 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 40 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3779, `ne_to_eq`: `MagickPathExtent,sizeof(*interpret_text)); \`
  - base line 3779, `ne_to_eq`: `MagickPathExtent,sizeof(*interpret_text)); \`
  - base line 3782, `ne_to_eq`: `status=MagickFalse; \`
  - base line 3782, `ne_to_eq`: `status=MagickFalse; \`
  - base line 3786, `ne_to_eq`: `} \`
  - base line 3791, `ne_to_eq`: `{ \`
  - and 34 more (`tools/oracle/mutate.py --function InterpretImageProperties`)

Commit message: `refactor(MagickCore/property.c): simplify InterpretImageProperties`

#### Step 2: `GetEXIFProperty` (line 862)

- **Recipe G (guard clauses)** - nesting is 7, target is under 4.
- **Recipe E (extract function)** - 14 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/property.c): simplify GetEXIFProperty`

#### Step 3: `GetMagickProperty` (line 2908)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 16 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/property.c): simplify GetMagickProperty`

#### Step 4: `SetImageProperty` (line 4415)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 15 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 38 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4475, `eq_to_ne`: `(LocaleCompare("skewness",property) == 0) ||`
  - base line 4500, `lt_to_ge`: `if (value == (const char *) NULL)`
  - base line 4522, `lt_to_ge`: `*/`
  - base line 4522, `lt_to_le`: `*/`
  - base line 4538, `ne_to_eq`: `#endif`
  - base line 4540, `gt_to_le`: `case 'b':`
  - and 32 more (`tools/oracle/mutate.py --function SetImageProperty`)

Commit message: `refactor(MagickCore/property.c): simplify SetImageProperty`

### Wave 2

#### Step 5: `Get8BIMProperty` (line 606)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/property.c): simplify Get8BIMProperty`

#### Step 6: `GetMagickPropertyLetter` (line 2495)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 14 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2515, `ne_to_eq`: `%  The format of the GetMagickProperty method is:`
  - base line 2515, `ne_to_eq`: `%  The format of the GetMagickProperty method is:`
  - base line 2518, `ne_to_eq`: `%        const char *property,ExceptionInfo *exception)`
  - base line 2519, `ne_to_eq`: `%`
  - base line 2582, `ne_to_eq`: `break;`
  - base line 2694, `ne_to_eq`: `break;`
  - and 8 more (`tools/oracle/mutate.py --function GetMagickPropertyLetter`)

Commit message: `refactor(MagickCore/property.c): simplify GetMagickPropertyLetter`

#### Step 7: `GetXMPProperty` (line 1850)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/property.c): simplify GetXMPProperty`

#### Step 8: `TracePSClippath` (line 1952)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Only 5 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/property.c): simplify TracePSClippath`

### Wave 3

#### Step 9: `TraceSVGClippath` (line 2169)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/property.c): simplify TraceSVGClippath`

#### Step 10: `GetICCProperty` (line 1748)

- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/property.c): simplify GetICCProperty`

#### Step 11: `GetImageProperty` (line 2306)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/property.c): simplify GetImageProperty`

#### Step 12: `GetIPTCProperty` (line 437)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/property.c): simplify GetIPTCProperty`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/property.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/property.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/property.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-26, Mac, night): Recipe H

**Result: Code Health 1.92 → 2.02** (`cs-mcp` 1.5.12). Branch `campaign/M85-property-h`,
stacked on `campaign/M84-hoist`. GetEXIFProperty's 87 lines of `#define` (formats, tags,
`EXIFGPSFractions`, `EXIFMultipleValues`) were hoisted (Recipe H, preprocessed output
`IDENTICAL`). The three compound tests M41 had to leave then became predicates
(oracle; GetEXIFProperty is ready, 158 of 158 killed): `IsGPSFractionTag`,
`IsSubdirectoryTag` and `IsNextIFDOffset`. Complex Conditional leaves property.c.

**What is left.** GetEXIFProperty is still cc 116 and 736 lines at depth 7. Its
per-format `switch` uses the two hoisted multi-line macros, which write the property
buffer and advance several cursors, so it needs Recipe O or K.
