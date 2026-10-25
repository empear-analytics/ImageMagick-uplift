# M21 - refactor `MagickCore/color.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **2.25** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2793 lines |
| Functions with findings | 14 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/color.c
```

The score must read 2.25. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 2 |
| Bumpy Road Ahead | 7 |
| Deep, Nested Complexity | 3 |
| Overall Code Complexity | file |
| Complex Method | 8 |
| Complex Conditional | 7 |
| Code Duplication | 4 |
| Large Method | 5 |
| Excess Number of Function Arguments | 3 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `QueryColorCompliance` | 2324 | 90 | 5 | 13 | public | - | 13233 | 29 of 40 |
| 2 | `LoadColorCache` | 1990 | - | 6 | 11 | static | - | 1391 | 1 of 13 |
| 3 | `IsEquivalentImage` | 1738 | - | 5 | 2 | public | - | 6 | - |
| 4 | `ConcatenateColorComponent` | 1161 | 35 | - | - | public | - | 335 | 7 of 9 |
| 5 | `GetColorCompliance` | 1010 | 13 | - | 3 | public | - | 1362 | 5 of 8 |
| 6 | `ParseCSSColor` | 2240 | 18 | - | 2 | static | - | 143 | 4 of 5 |
| 7 | `QueryColorname` | 2740 | 13 | - | 2 | public | - | 435 | 7 of 8 |
| 8 | `ListColorInfo` | 1899 | 12 | - | 2 | public | - | 3 | - |
| 9 | `GetColorTuple` | 1556 | 12 | - | - | public | - | 564 | 14 of 15 |
| 10 | `ConcatenateHexColorComponent` | 1488 | 10 | - | - | static | - | 403 | - |
| 11 | `GetColorInfoList` | 1316 | - | - | - | public | - | 3 | - |
| 12 | `IsEquivalentAlpha` | 1685 | - | - | - | public | - | 13 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `QueryColorCompliance` (line 2324)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 13 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 11 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2411, `ne_to_eq`: `if ((n != 4) && (n != 8) && (n != 16) && (n != 32) && (n != 64))`
  - base line 2427, `le_to_lt`: `if ((c >= '0') && (c <= '9'))`
  - base line 2496, `remove_void_call`: `LocaleLower(colorspace);`
  - base line 2617, `ne_to_eq`: `(color->alpha_trait != UndefinedPixelTrait))`
  - base line 2642, `div_to_mul`: `ConvertHCLToRGB(fmod(fmod(geometry_info.rho,360.0)+360.0,360.0)/`
  - base line 2642, `remove_void_call`: `ConvertHCLToRGB(fmod(fmod(geometry_info.rho,360.0)+360.0,360.0)/`
  - and 5 more (`tools/oracle/mutate.py --function QueryColorCompliance`)

Commit message: `refactor(MagickCore/color.c): simplify QueryColorCompliance`

#### Step 2: `LoadColorCache` (line 1990)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 11 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- The oracle missed 9 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2020, `ne_to_eq`: `for (q=(char *) xml; *q != '\0'; )`
  - base line 2026, `eq_to_ne`: `if (*token == '\0')`
  - base line 2047, `eq_to_ne`: `if ((*q == '"') || (*q == '\''))`
  - base line 2053, `eq_to_ne`: `if (*q == ']')`
  - base line 2100, `remove_void_call`: `GetPathComponent(filename,HeadPath,path);`
  - base line 2135, `eq_to_ne`: `(LocaleCompare(keyword,"</policy>") == 0))`
  - and 3 more (`tools/oracle/mutate.py --function LoadColorCache`)
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/color.c): simplify LoadColorCache`

#### Step 3: `IsEquivalentImage` (line 1738)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/color.c): simplify IsEquivalentImage`

#### Step 4: `ConcatenateColorComponent` (line 1161)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1203, `ne_to_eq`: `if ((compliance != NoCompliance) &&`
  - base line 1251, `mul_to_div`: `GetMagickPrecision(),RadiansToDegrees(scale*QuantumScale*color));`

Commit message: `refactor(MagickCore/color.c): simplify ConcatenateColorComponent`

### Wave 2

#### Step 5: `GetColorCompliance` (line 1010)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1034, `ne_to_eq`: `for (q=colorname; *q != '\0'; q++)`
  - base line 1050, `ne_to_eq`: `if (p != (ElementInfo *) NULL)`
  - base line 1069, `remove_void_call`: `SetHeadElementInLinkedList(color_cache,p);`

Commit message: `refactor(MagickCore/color.c): simplify GetColorCompliance`

#### Step 6: `ParseCSSColor` (line 2240)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2252, `remove_void_call`: `SetGeometryInfo(geometry_info);`

Commit message: `refactor(MagickCore/color.c): simplify ParseCSSColor`

#### Step 7: `QueryColorname` (line 2740)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2761, `gt_to_ge`: `if (pixel.depth > 16)`

Commit message: `refactor(MagickCore/color.c): simplify QueryColorname`

#### Step 8: `ListColorInfo` (line 1899)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 3 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/color.c): simplify ListColorInfo`

### Wave 3

#### Step 9: `GetColorTuple` (line 1556)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1586, `ne_to_eq`: `if ((color.depth > 8) && (IsSVGCompliant(pixel) != MagickFalse))`

Commit message: `refactor(MagickCore/color.c): simplify GetColorTuple`

#### Step 10: `ConcatenateHexColorComponent` (line 1488)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/color.c): simplify ConcatenateHexColorComponent`

#### Step 11: `GetColorInfoList` (line 1316)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 3 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/color.c): simplify GetColorInfoList`

#### Step 12: `IsEquivalentAlpha` (line 1685)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/color.c): simplify IsEquivalentAlpha`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/color.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/color.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/color.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
