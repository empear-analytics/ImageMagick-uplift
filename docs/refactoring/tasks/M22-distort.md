# M22 - refactor `MagickCore/distort.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.74** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 3467 lines |
| Functions with findings | 8 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/distort.c
```

The score must read 1.74. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 3 |
| Bumpy Road Ahead | 4 |
| Deep, Nested Complexity | 3 |
| Overall Code Complexity | file |
| Complex Method | 4 |
| Complex Conditional | 7 |
| Code Duplication | 3 |
| Large Method | 4 |
| Excess Number of Function Arguments | 3 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `DistortImage` | 1771 | - | 5 | 17 | public | 2 | 502 | 23 of 43 |
| 2 | `GenerateCoefficients` | 372 | - | 4 | 11 | static | - | 533 | 26 of 38 |
| 3 | `SparseColorImage` | 3058 | - | 6 | 7 | public | 2 | 31 | 12 of 29 |
| 4 | `poly_basis_dx` | 209 | 22 | - | - | static | - | 17 | 0 of 1 |
| 5 | `poly_basis_str` | 181 | 22 | - | - | static | - | 6 | - |
| 6 | `poly_basis_fn` | 153 | 22 | - | - | static | - | 17 | 0 of 5 |
| 7 | `DistortResizeImage` | 1561 | 11 | - | 2 | public | - | 72 | 2 of 3 |
| 8 | `poly_number_terms` | 144 | - | - | - | static | - | 21 | 1 of 1 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `DistortImage` (line 1771)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 17 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 17 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1999, `div_to_mul`: `geometry.height = CastDoubleToSizeT(ceil( 2.0*coeff[3]/cos(coeff[0]/2.0) ));`
  - base line 2101, `remove_void_call`: `InvertAffineCoefficients(coeff, inverse);`
  - base line 2102, `remove_void_call`: `CoefficientsToAffineArgs(inverse);`
  - base line 2146, `remove_void_call`: `InvertPerspectiveCoefficients(coeff, inverse);`
  - base line 2154, `post_inc_to_post_dec`: `for ( ; i < 7; i++)`
  - base line 2257, `ne_to_eq`: `if ((i != 0) && (i%4 == 0))`
  - and 11 more (`tools/oracle/mutate.py --function DistortImage`)
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/distort.c): simplify DistortImage`

#### Step 2: `GenerateCoefficients` (line 372)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 11 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- The oracle missed 11 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 536, `sub_to_add`: `coeff[2] = arguments[0] - arguments[2];`
  - base line 599, `remove_void_call`: `LeastSquaresAddTerms(matrix,vectors,terms,uv2,3UL,2UL);`
  - base line 881, `mul_to_div`: `number_arguments < cp_size*4 ) {`
  - base line 967, `mul_to_div`: `+ coeff[7]*arguments[5] + 1.0;`
  - base line 1242, `ge_to_lt`: `if ( number_arguments >= 2 )`
  - base line 1250, `ge_to_lt`: `if ( number_arguments >= 4 )`
  - and 5 more (`tools/oracle/mutate.py --function GenerateCoefficients`)
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/distort.c): simplify GenerateCoefficients`

#### Step 3: `SparseColorImage` (line 3058)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 17 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3135, `add_to_sub`: `coeff[x], coeff[x+1], coeff[x+2]),x+=3;`
  - base line 3141, `add_to_sub`: `coeff[x], coeff[x+1], coeff[x+2]),x+=3;`
  - base line 3164, `ne_to_eq`: `if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)`
  - base line 3169, `eq_to_ne`: `(image->colorspace == CMYKColorspace))`
  - base line 3177, `add_to_sub`: `coeff[x+2], coeff[x+3]),x+=4;`
  - base line 3243, `remove_void_call`: `GetPixelInfo(sparse_image,&pixel);`
  - and 11 more (`tools/oracle/mutate.py --function SparseColorImage`)

Commit message: `refactor(MagickCore/distort.c): simplify SparseColorImage`

#### Step 4: `poly_basis_dx` (line 209)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 224, `mul_to_div`: `case 11:  return( x*x*y );`

Commit message: `refactor(MagickCore/distort.c): simplify poly_basis_dx`

### Wave 2

#### Step 5: `poly_basis_str` (line 181)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/distort.c): simplify poly_basis_str`

#### Step 6: `poly_basis_fn` (line 153)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- The oracle missed 5 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 163, `mul_to_div`: `case  6:  return( x*x*x );`
  - base line 164, `mul_to_div`: `case  7:  return( x*x*y );`
  - base line 165, `mul_to_div`: `case  8:  return( x*y*y );`
  - base line 167, `mul_to_div`: `case 10:  return( x*x*x*x );`
  - base line 172, `mul_to_div`: `case 15:  return( x*x*x*x*x );`

Commit message: `refactor(MagickCore/distort.c): simplify poly_basis_fn`

#### Step 7: `DistortResizeImage` (line 1561)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/distort.c): simplify DistortResizeImage`

#### Step 8: `poly_number_terms` (line 144)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/distort.c): simplify poly_number_terms`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/distort.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/distort.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/distort.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
