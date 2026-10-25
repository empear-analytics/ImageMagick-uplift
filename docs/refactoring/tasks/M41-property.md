# M41 - refactor `MagickCore/property.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.48** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 4880 lines |
| Functions with findings | 20 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/property.c
```

The score must read 1.48. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 12 |
| Deep, Nested Complexity | 8 |
| Overall Code Complexity | file |
| Complex Method | 12 |
| Complex Conditional | 16 |
| Code Duplication | 7 |
| Large Method | 11 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `InterpretImageProperties` | 3702 | 143 | 6 | 15 | public | - | 12374 | 60 of 100 |
| 2 | `GetEXIFProperty` | 860 | 131 | 7 | 14 | static | - | 14911 | 158 of 158 |
| 3 | `GetMagickProperty` | 2906 | 125 | 4 | 16 | public | - | 189 | 102 of 102 |
| 4 | `SetImageProperty` | 4413 | 108 | 4 | 15 | public | - | 11529 | 45 of 83 |
| 5 | `Get8BIMProperty` | 604 | 51 | 4 | 6 | static | - | 221 | 51 of 51 |
| 6 | `GetMagickPropertyLetter` | 2493 | 79 | - | 2 | static | - | 12237 | 19 of 33 |
| 7 | `TracePSClippath` | 1951 | 36 | 4 | 4 | static | - | 5 | 35 of 35 |
| 8 | `GetXMPProperty` | 1849 | 24 | 6 | 4 | static | - | 14911 | 30 of 30 |
| 9 | `TraceSVGClippath` | 2167 | 20 | 4 | 3 | static | - | 12 | 24 of 24 |
| 10 | `GetICCProperty` | 1747 | 15 | - | 5 | static | - | 14911 | 21 of 21 |
| 11 | `GetImageProperty` | 2304 | 27 | - | 2 | public | - | 14947 | 26 of 26 |
| 12 | `GetIPTCProperty` | 437 | 14 | - | 2 | static | - | 14911 | 21 of 21 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `InterpretImageProperties` (line 3702)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 15 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 34 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3791, `ne_to_eq`: `while ((isspace((int) ((unsigned char) *p)) != 0) && (*p != '\0'))`
  - base line 3795, `eq_to_ne`: `if ((*p == '@') && (IsPathAccessible(p+1) != MagickFalse))`
  - base line 3795, `ne_to_eq`: `if ((*p == '@') && (IsPathAccessible(p+1) != MagickFalse))`
  - base line 3801, `ne_to_eq`: `if (interpret_text != (char *) NULL)`
  - base line 3807, `ne_to_eq`: `if (image_info != (ImageInfo *) NULL)`
  - base line 3881, `eq_to_ne`: `if (LocaleNCompare("&lt;",p,4) == 0)`
  - and 28 more (`tools/oracle/mutate.py --function InterpretImageProperties`)
- 6 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/property.c): simplify InterpretImageProperties`

#### Step 2: `GetEXIFProperty` (line 860)

- **Recipe G (guard clauses)** - nesting is 7, target is under 4.
- **Recipe E (extract function)** - 14 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/property.c): simplify GetEXIFProperty`

#### Step 3: `GetMagickProperty` (line 2906)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 16 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/property.c): simplify GetMagickProperty`

#### Step 4: `SetImageProperty` (line 4413)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 15 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 38 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4475, `eq_to_ne`: `if (LocaleCompare("background",property) == 0)`
  - base line 4500, `lt_to_ge`: `if (colorspace < 0)`
  - base line 4522, `lt_to_ge`: `if (compression < 0)`
  - base line 4522, `lt_to_le`: `if (compression < 0)`
  - base line 4538, `ne_to_eq`: `if ((flags & GreaterValue) != 0)`
  - base line 4540, `gt_to_le`: `if (image->delay > (size_t) floor(geometry_info.rho+0.5))`
  - and 32 more (`tools/oracle/mutate.py --function SetImageProperty`)

Commit message: `refactor(MagickCore/property.c): simplify SetImageProperty`

### Wave 2

#### Step 5: `Get8BIMProperty` (line 604)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/property.c): simplify Get8BIMProperty`

#### Step 6: `GetMagickPropertyLetter` (line 2493)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 9 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2518, `ne_to_eq`: `if ((image_info != (ImageInfo *) NULL) &&`
  - base line 2582, `ne_to_eq`: `(image->rows != 0 ? image->rows : image->magick_rows));`
  - base line 2694, `ne_to_eq`: `(image->columns != 0 ? image->columns : image->magick_columns));`
  - base line 2734, `eq_to_ne`: `if (image->extent == 0)`
  - base line 2800, `eq_to_ne`: `(image->quality == 0 ? 92 : image->quality));`
  - base line 2816, `eq_to_ne`: `(image_info->number_scenes == 0 ? 2147483647 :`
  - and 3 more (`tools/oracle/mutate.py --function GetMagickPropertyLetter`)
- 5 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/property.c): simplify GetMagickPropertyLetter`

#### Step 7: `TracePSClippath` (line 1951)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Only 5 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/property.c): simplify TracePSClippath`

#### Step 8: `GetXMPProperty` (line 1849)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/property.c): simplify GetXMPProperty`

### Wave 3

#### Step 9: `TraceSVGClippath` (line 2167)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/property.c): simplify TraceSVGClippath`

#### Step 10: `GetICCProperty` (line 1747)

- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/property.c): simplify GetICCProperty`

#### Step 11: `GetImageProperty` (line 2304)

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
