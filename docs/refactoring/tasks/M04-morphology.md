# M04 - refactor `MagickCore/morphology.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.53** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 4765 lines |
| Functions with findings | 12 |
| Measured with | `cs-mcp` 1.5.12, `cs` not installed |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/morphology.c
```

The score must read 1.53. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 3 |
| Bumpy Road Ahead | 11 |
| Deep, Nested Complexity | 7 |
| Overall Code Complexity | file |
| Complex Method | 9 |
| Complex Conditional | 2 |
| Large Method | 8 |
| Excess Number of Function Arguments | 3 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `AcquireKernelBuiltIn` | 962 | 294 | 5 | 25 | public | - | 2526 | 458 of 511 |
| 2 | `MorphologyPrimitiveDirect` | 3216 | 75 | 8 | 14 | static | 2 | 88 | 153 of 176 |
| 3 | `MorphologyPrimitive` | 2540 | - | 9 | 17 | static | 4 | 2521 | 191 of 216 |
| 4 | `RotateKernelInfo` | 4232 | 45 | 4 | 5 | static | - | 1557 | 83 of 103 |
| 5 | `MorphologyApply` | 3608 | - | 6 | 10 | public | - | 2609 | 72 of 77 |
| 6 | `ParseKernelArray` | 219 | 38 | - | 5 | static | - | 1140 | 50 of 61 |
| 7 | `ParseKernelName` | 380 | 39 | - | 2 | static | - | 2526 | 30 of 34 |
| 8 | `MorphologyImage` | 4103 | - | 4 | 4 | public | - | 2609 | 14 of 16 |
| 9 | `ShowKernelInfo` | 4632 | 9 | 4 | 2 | public | - | 81 | 18 of 22 |
| 10 | `AcquireKernelInfo` | 497 | 12 | - | 3 | public | - | 2668 | 8 of 9 |
| 11 | `ScaleKernelInfo` | 4545 | 13 | - | 2 | public | - | 730 | 19 of 27 |
| 12 | `SameKernelInfo` | 2370 | 11 | - | - | static | - | 358 | 9 of 11 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `AcquireKernelBuiltIn` (line 962)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 25 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 44 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1065, `ge_to_gt`: `if ( args->rho >= 1.0 )`
  - base line 1067, `ge_to_gt`: `else if ( (type != DoGKernel) || (sigma >= sigma2) )`
  - base line 1117, `div_to_mul`: `B = (double) (1.0/(MagickPI*sigma*sigma*sigma*sigma));`
  - base line 1117, `mul_to_div`: `B = (double) (1.0/(MagickPI*sigma*sigma*sigma*sigma));`
  - base line 1117, `mul_to_div`: `B = (double) (1.0/(MagickPI*sigma*sigma*sigma*sigma));`
  - base line 1117, `mul_to_div`: `B = (double) (1.0/(MagickPI*sigma*sigma*sigma*sigma));`
  - and 38 more (`tools/oracle/mutate.py --function AcquireKernelBuiltIn`)
- 9 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/morphology.c): simplify AcquireKernelBuiltIn`

#### Step 2: `MorphologyPrimitiveDirect` (line 3216)

- **Recipe G (guard clauses)** - nesting is 8, target is under 4.
- **Recipe E (extract function)** - 14 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 18 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3351, `lt_to_le`: `if (((double) pixels[i]+(*k)) < pixel)`
  - base line 3363, `add_to_sub`: `if (!IsNaN(*k) && ((x+u-offset.x) >= 0))`
  - base line 3365, `lt_to_le`: `if (((double) pixels[i]+(*k)) < pixel)`
  - base line 3382, `lt_to_le`: `if (((double) pixels[i]+(*k)) < pixel)`
  - base line 3394, `add_to_sub`: `if (!IsNaN(*k) && ((x+u-offset.x) >= 0))`
  - base line 3394, `ge_to_gt`: `if (!IsNaN(*k) && ((x+u-offset.x) >= 0))`
  - and 12 more (`tools/oracle/mutate.py --function MorphologyPrimitiveDirect`)
- 5 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/morphology.c): simplify MorphologyPrimitiveDirect`

#### Step 3: `MorphologyPrimitive` (line 2540)

- **Recipe G (guard clauses)** - nesting is 9, target is under 4.
- **Recipe E (extract function)** - 17 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- Contains 4 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 20 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2709, `remove_void_call`: `SetPixelChannel(morphology_image,channel,p[center+i],q);`
  - base line 2746, `sub_to_add`: `if (fabs(pixel-(double) p[center+i]) >= MagickEpsilon)`
  - base line 2746, `add_to_sub`: `if (fabs(pixel-(double) p[center+i]) >= MagickEpsilon)`
  - base line 2746, `ge_to_lt`: `if (fabs(pixel-(double) p[center+i]) >= MagickEpsilon)`
  - base line 2749, `ne_to_eq`: `if (count != 0)`
  - base line 2780, `div_to_mul`: `return(status ? (ssize_t) (changed/GetImageChannels(image)) : 0);`
  - and 14 more (`tools/oracle/mutate.py --function MorphologyPrimitive`)
- 5 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/morphology.c): simplify MorphologyPrimitive`

#### Step 4: `RotateKernelInfo` (line 4232)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 19 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4245, `lt_to_le`: `if ( angle < 0 )`
  - base line 4248, `lt_to_le`: `if ( 337.5 < angle || angle <= 22.5 )`
  - base line 4248, `le_to_lt`: `if ( 337.5 < angle || angle <= 22.5 )`
  - base line 4276, `le_to_gt`: `if ( 135.0 < angle && angle <= 225.0 )`
  - base line 4276, `le_to_lt`: `if ( 135.0 < angle && angle <= 225.0 )`
  - base line 4278, `lt_to_le`: `if ( 225.0 < angle && angle <= 315.0 )`
  - and 13 more (`tools/oracle/mutate.py --function RotateKernelInfo`)
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/morphology.c): simplify RotateKernelInfo`

### Wave 2

#### Step 5: `MorphologyApply` (line 3608)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 10 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3668, `lt_to_le`: `if ( iterations < 0 )  /* negative interactions = infinite (well almost) */`
  - base line 3669, `gt_to_ge`: `kernel_limit = image->columns>image->rows ? image->columns : image->rows;`
  - base line 3737, `lt_to_le`: `if ( changed < 0 )`
  - base line 3926, `add_to_sub`: `method_changed = (size_t) ((ssize_t) method_changed+changed);`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/morphology.c): simplify MorphologyApply`

#### Step 6: `ParseKernelArray` (line 219)

- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 9 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 270, `lt_to_le`: `if ( p != (char *) NULL && p < end)`
  - base line 276, `remove_void_call`: `SetGeometryInfo(&args);`
  - base line 282, `lt_to_le`: `if ( args.rho < 1.0 )            /* if width too small */`
  - base line 296, `ge_to_gt`: `if ( kernel->x >= (ssize_t) kernel->width ||`
  - base line 297, `ge_to_gt`: `kernel->y >= (ssize_t) kernel->height )`
  - base line 339, `lt_to_le`: `( kernel->values[i] < 0)`
  - and 3 more (`tools/oracle/mutate.py --function ParseKernelArray`)
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/morphology.c): simplify ParseKernelArray`

#### Step 7: `ParseKernelName` (line 380)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 408, `lt_to_le`: `if ( type < 0 || type == UserDefinedKernel )`
  - base line 423, `remove_void_call`: `SetGeometryInfo(&args);`
  - base line 455, `lt_to_le`: `if ( args.rho < 1.0 )            /* if width too small */`
  - base line 457, `lt_to_le`: `if ( args.sigma < 1.0 )          /* if height too small */`

Commit message: `refactor(MagickCore/morphology.c): simplify ParseKernelName`

#### Step 8: `MorphologyImage` (line 4103)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4156, `eq_to_ne`: `if ( curr_kernel == kernel )`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/morphology.c): simplify MorphologyImage`

### Wave 3

#### Step 9: `ShowKernelInfo` (line 4632)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4660, `add_to_sub`: `else if ( fabs(k->positive_range+k->negative_range-1.0) < MagickEpsilon )`
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/morphology.c): simplify ShowKernelInfo`

#### Step 10: `AcquireKernelInfo` (line 497)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/morphology.c): simplify AcquireKernelInfo`

#### Step 11: `ScaleKernelInfo` (line 4545)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4564, `add_to_sub`: `pos_scale = fabs(kernel->positive_range + kernel->negative_range);`
  - base line 4585, `ge_to_gt`: `kernel->values[i] *= (kernel->values[i] >= 0) ? pos_scale : neg_scale;`
  - base line 4591, `ge_to_gt`: `kernel->maximum *= (kernel->maximum >= 0.0) ? pos_scale : neg_scale;`
  - base line 4592, `ge_to_gt`: `kernel->minimum *= (kernel->minimum >= 0.0) ? pos_scale : neg_scale;`
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/morphology.c): simplify ScaleKernelInfo`

#### Step 12: `SameKernelInfo` (line 2370)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/morphology.c): simplify SameKernelInfo`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/morphology.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/morphology.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/morphology.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
