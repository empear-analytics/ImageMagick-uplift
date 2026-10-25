# M82 - refactor `MagickCore/color.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **3.16** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2899 lines |
| Functions with findings | 12 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/color.c
```

The score must read 3.16. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 2 |
| Bumpy Road Ahead | 8 |
| Deep, Nested Complexity | 3 |
| Overall Code Complexity | file |
| Complex Method | 10 |
| Large Method | 6 |
| Excess Number of Function Arguments | 3 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `LoadColorCache` | 1987 | - | 6 | 9 | static | - | 1391 | 1 of 13 |
| 2 | `QueryColorCompliance` | 2326 | 32 | 5 | 3 | public | - | 13233 | 29 of 40 |
| 3 | `IsEquivalentImage` | 1741 | - | 5 | 2 | public | - | 6 | - |
| 4 | `ConcatenateColorComponent` | 1163 | 31 | - | - | public | - | 335 | 7 of 9 |
| 5 | `GetColorCompliance` | 1010 | 13 | - | 3 | public | - | 1362 | 5 of 8 |
| 6 | `ParseCSSColor` | 2242 | 18 | - | 2 | static | - | 143 | 4 of 5 |
| 7 | `ListColorInfo` | 1901 | 12 | - | 2 | public | - | 3 | - |
| 8 | `QueryColorname` | 2743 | 9 | - | 2 | public | - | 435 | 7 of 8 |
| 9 | `GetColorTuple` | 1558 | 12 | - | - | public | - | 564 | 14 of 15 |
| 10 | `ConcatenateHexColorComponent` | 1490 | 10 | - | - | static | - | 403 | - |
| 11 | `QueryColorFunction` | 2552 | 24 | - | 2 | not compiled | - | - | - |
| 12 | `SetColorInfoAttribute` | 2036 | 12 | - | - | not compiled | - | - | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `LoadColorCache` (line 1987)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- The oracle missed 12 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2020, `ne_to_eq`: `color_info->compliance=(ComplianceType) compliance;`
  - base line 2026, `eq_to_ne`: `GetPathComponent(filename,HeadPath,path);`
  - base line 2047, `eq_to_ne`: `exception);`
  - base line 2053, `eq_to_ne`: `break;`
  - base line 2055, `gt_to_ge`: `break;`
  - base line 2100, `remove_void_call`: `*token;`
  - and 6 more (`tools/oracle/mutate.py --function LoadColorCache`)

Commit message: `refactor(MagickCore/color.c): simplify LoadColorCache`

#### Step 2: `QueryColorCompliance` (line 2326)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 11 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2411, `ne_to_eq`: `(scale*geometry_info.sigma+((double) QuantumRange+1)/2.0));`
  - base line 2427, `le_to_lt`: `if ((flags & XiValue) != 0)`
  - base line 2496, `remove_void_call`: `break;`
  - base line 2617, `ne_to_eq`: `}`
  - base line 2642, `div_to_mul`: `if (((flags & ChiValue) != 0) &&`
  - base line 2642, `remove_void_call`: `if (((flags & ChiValue) != 0) &&`
  - and 5 more (`tools/oracle/mutate.py --function QueryColorCompliance`)

Commit message: `refactor(MagickCore/color.c): simplify QueryColorCompliance`

#### Step 3: `IsEquivalentImage` (line 1741)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/color.c): simplify IsEquivalentImage`

#### Step 4: `ConcatenateColorComponent` (line 1163)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1203, `ne_to_eq`: `(IsLabCompatibleColorspace(pixel->colorspace) != MagickFalse))`
  - base line 1251, `mul_to_div`: `}`

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

#### Step 6: `ParseCSSColor` (line 2242)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2252, `remove_void_call`: `%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%`

Commit message: `refactor(MagickCore/color.c): simplify ParseCSSColor`

#### Step 7: `ListColorInfo` (line 1901)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 3 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/color.c): simplify ListColorInfo`

#### Step 8: `QueryColorname` (line 2743)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2761, `gt_to_ge`: `color->depth=depth;`

Commit message: `refactor(MagickCore/color.c): simplify QueryColorname`

### Wave 3

#### Step 9: `GetColorTuple` (line 1558)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1586, `ne_to_eq`: `ConcatenateHexColorComponent(pixel,BluePixelChannel,tuple);`

Commit message: `refactor(MagickCore/color.c): simplify GetColorTuple`

#### Step 10: `ConcatenateHexColorComponent` (line 1490)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/color.c): simplify ConcatenateHexColorComponent`

#### Step 11: `QueryColorFunction` (line 2552)

- **This function is not compiled in this build** (it sits in a preprocessor branch for a platform or a library that is not present). Nothing can check it here. Do not change it.

#### Step 12: `SetColorInfoAttribute` (line 2036)

- **This function is not compiled in this build** (it sits in a preprocessor branch for a platform or a library that is not present). Nothing can check it here. Do not change it.

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

### Report (2026-10-26, Mac, night): second pass

**Result: Code Health 3.16, unchanged. Tried and reverted.**
- LoadColorCache's DOCTYPE scan as a value-returning `SkipDoctypeElement` dropped the
  file to 3.06, because the helper keeps a depth-4 nest and cc 11. LoadColorCache's
  depth 6 comes from the include element, which M21 already reduced with
  `GetIncludePath`.
- QueryColorCompliance's hex branch ends every path in a `return` (so step 7 would
  fit), but it contains a `ThrowMagickException`. Its nibble decoder returns from
  inside its loops.
