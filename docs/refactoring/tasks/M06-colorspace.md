# M06 - refactor `MagickCore/colorspace.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **2.57** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2900 lines |
| Functions with findings | 9 |
| Measured with | `cs-mcp` 1.5.12, `cs` not installed |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/colorspace.c
```

The score must read 2.57. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 2 |
| Deep, Nested Complexity | 2 |
| Overall Code Complexity | file |
| Complex Method | 5 |
| Complex Conditional | 3 |
| Code Duplication | 4 |
| Large Method | 4 |
| Excess Number of Function Arguments | 4 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `TransformsRGBImage` | 1828 | 151 | 4 | 19 | static | 14 | 424 | 216 of 242 |
| 2 | `sRGBTransformImage` | 727 | 142 | 4 | 16 | static | 13 | 1205 | 206 of 220 |
| 3 | `ConvertRGBToGeneric` | 416 | 30 | - | - | public | - | 607 | 31 of 31 |
| 4 | `ConvertGenericToRGB` | 127 | 30 | - | - | public | - | 267 | 31 of 31 |
| 5 | `ConvertHSLToRGB` | 312 | 9 | - | - | public | - | 83 | 40 of 41 |
| 6 | `SetImageColorspace` | 1567 | - | - | - | public | - | 4321 | 6 of 7 |
| 7 | `SetImageGray` | 1647 | - | - | - | public | - | 26 | 5 of 6 |
| 8 | `SetImageMonochrome` | 1704 | - | - | - | public | - | 222 | 4 of 5 |
| 9 | `ConvertRGBToHSL` | 602 | - | - | - | public | - | 152 | 23 of 26 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `TransformsRGBImage` (line 1828)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 19 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 14 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 22 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2099, `ne_to_eq`: `if (artifact != (const char *) NULL)`
  - base line 2130, `remove_void_call`: `GetPixelInfo(image,&zero);`
  - base line 2233, `eq_to_ne`: `if (SetImageColorspace(image,sRGBColorspace,exception) == MagickFalse)`
  - base line 2256, `lt_to_ge`: `for (y=0; y < (ssize_t) image->rows; y++)`
  - base line 2267, `eq_to_ne`: `if (status == MagickFalse)`
  - base line 2276, `lt_to_ge`: `for (x=0; x < (ssize_t) image->columns; x++)`
  - and 16 more (`tools/oracle/mutate.py --function TransformsRGBImage`)
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/colorspace.c): simplify TransformsRGBImage`

#### Step 2: `sRGBTransformImage` (line 727)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 16 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 13 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 10 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 767, `ne_to_eq`: `if (artifact != (const char *) NULL)`
  - base line 800, `remove_void_call`: `GetPixelInfo(image,&zero);`
  - base line 844, `eq_to_ne`: `if (SetImageColorspace(image,colorspace,exception) == MagickFalse)`
  - base line 901, `eq_to_ne`: `if (SetImageColorspace(image,colorspace,exception) == MagickFalse)`
  - base line 1106, `mul_to_div`: `black=pow(10.0,(reference_black-reference_white)*(gamma/density)*0.002*`
  - base line 1165, `eq_to_ne`: `if (SetImageColorspace(image,colorspace,exception) == MagickFalse)`
  - and 4 more (`tools/oracle/mutate.py --function sRGBTransformImage`)
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/colorspace.c): simplify sRGBTransformImage`

#### Step 3: `ConvertRGBToGeneric` (line 416)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/colorspace.c): simplify ConvertRGBToGeneric`

#### Step 4: `ConvertGenericToRGB` (line 127)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/colorspace.c): simplify ConvertGenericToRGB`

### Wave 2

#### Step 5: `ConvertHSLToRGB` (line 312)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 328, `le_to_lt`: `if (lightness <= 0.5)`

Commit message: `refactor(MagickCore/colorspace.c): simplify ConvertHSLToRGB`

#### Step 6: `SetImageColorspace` (line 1567)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/colorspace.c): simplify SetImageColorspace`

#### Step 7: `SetImageGray` (line 1647)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/colorspace.c): simplify SetImageGray`

#### Step 8: `SetImageMonochrome` (line 1704)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/colorspace.c): simplify SetImageMonochrome`

### Wave 3

#### Step 9: `ConvertRGBToHSL` (line 602)

- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 640, `le_to_lt`: `if (*lightness <= 0.5)`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/colorspace.c): simplify ConvertRGBToHSL`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/colorspace.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/colorspace.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/colorspace.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, overnight)

**Result: Code Health 2.57 → 2.80** (`cs-mcp` 1.5.12). Branch `campaign/M06-colorspace`,
stacked on `campaign/M05-visual-effects` (PR #11). 4 refactor commits, all Recipe P and
oracle-verified.

**One category leaves the file:** Complex Conditional. The commits:
- `IsTransformMapMissing(x_map,y_map,z_map)` in `sRGBTransformImage`, then reused in
  `TransformsRGBImage`, whose test is the same text;
- `IsLinearGammaColorspace(colorspace)` in `SetImageColorspace`.

| Steps | Recipe | Score |
| --- | --- | --- |
| the transform-map allocation test, in both transforms | P | 2.57 → 2.68 |
| `SetImageColorspace`'s linear-gamma test | P | → 2.80 |

**Whole catalogue** at the branch tip against `8a4c272f1`: 15,884 cases, 1 diverged
(`gaps/c5651ffad8`, the known log-map tie).

**Why it stops here.**
- `sRGBTransformImage` (cc 142) and `TransformsRGBImage` (cc 151) are one `switch
  (colorspace)` each. Every case contains its own `#pragma omp parallel for`, and the
  map-building cases run their loops under pragmas too. A case cannot move into a
  function without moving its pragma, which the OpenMP rules forbid.
- A probe extracted all seven pixel loops of `sRGBTransformImage` in a scratch copy. The
  score stayed at 2.80, nesting at 4 and the bumps at 16. Those loops are not where
  CodeScene's findings come from, so the one such step already made (the CMYK loop,
  flat) was reverted, not committed.
- `ConvertRGBToGeneric` and `ConvertGenericToRGB` (Code Duplication, cc 30, nine
  arguments) are public, and the readiness gate has them not ready (0%).
- `SetImageGray` and `SetImageMonochrome` share a tail that differs in two places (a cast
  and the type), so Recipe D does not apply.

This is a plateau for the current catalogue. A recipe that let a case body (with its
pragma) move as a unit into a function called from the same place would open it. That
needs the owner's decision on the OpenMP rule, not a new recipe written overnight.

**Process note.** A whole-catalogue run must follow a build of the branch tip. After the
reverted probe, the candidate build still held the uncommitted loop extraction. That run
was stopped, the tip rebuilt, and the catalogue run again.
