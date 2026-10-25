# M77 - refactor `MagickCore/draw.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.77** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 8114 lines |
| Functions with findings | 32 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/draw.c
```

The score must read 1.77. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 1 |
| Bumpy Road Ahead | 18 |
| Deep, Nested Complexity | 10 |
| Overall Code Complexity | file |
| Complex Method | 24 |
| Large Method | 20 |
| Excess Number of Function Arguments | 8 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `RenderMVGContent` | 2522 | 461 | 6 | 74 | static | - | 476 | 470 of 473 |
| 2 | `TracePath` | 6738 | 167 | 5 | 11 | static | - | 50 | 167 of 167 |
| 3 | `DrawPrimitive` | 5486 | 77 | 5 | 12 | public | - | 436 | 99 of 99 |
| 4 | `TraceStrokePolygon` | 7469 | 73 | 4 | 10 | static | - | 47 | 227 of 227 |
| 5 | `DrawGradientImage` | 2016 | 46 | 6 | 9 | public | 1 | 73 | 95 of 105 |
| 6 | `ConvertPathToPolygon` | 523 | 37 | 5 | 7 | static | - | 358 | 56 of 56 |
| 7 | `DrawPolygonPrimitive` | 5122 | 46 | 4 | 5 | static | 1 | 362 | 66 of 68 |
| 8 | `GetFillAlpha` | 4943 | - | 6 | 9 | static | - | 356 | 110 of 110 |
| 9 | `DrawDashPolygon` | 1756 | 30 | 4 | 5 | static | - | 6 | 64 of 64 |
| 10 | `GetMVGMacros` | 2374 | 16 | 6 | 4 | static | - | 476 | 20 of 20 |
| 11 | `TraceBezier` | 6500 | 18 | - | 4 | static | - | 40 | 41 of 41 |
| 12 | `DrawAffineImage` | 1161 | 23 | - | 3 | public | 1 | 13 | 45 of 45 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `RenderMVGContent` (line 2522)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 74 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2603, `ge_to_gt`: `static inline MagickBooleanType IsRenderableClass(DrawInfo **graphic_context,`
  - base line 3137, `eq_to_ne`: `if (token == next_token)`
  - base line 4676, `lt_to_le`: `status&=(MagickStatusType) CheckPrimitiveExtent(&mvg_info,ExpandAffine(`

Commit message: `refactor(MagickCore/draw.c): simplify RenderMVGContent`

#### Step 2: `TracePath` (line 6738)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 11 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/draw.c): simplify TracePath`

#### Step 3: `DrawPrimitive` (line 5486)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 12 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/draw.c): simplify DrawPrimitive`

#### Step 4: `TraceStrokePolygon` (line 7469)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 10 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/draw.c): simplify TraceStrokePolygon`

### Wave 2

#### Step 5: `DrawGradientImage` (line 2016)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 10 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2053, `ne_to_eq`: `*gradient_vector;`
  - base line 2056, `remove_void_call`: `length;`
  - base line 2064, `remove_void_call`: `PointInfo`
  - base line 2109, `remove_void_call`: ``
  - base line 2150, `lt_to_le`: `}`
  - base line 2157, `lt_to_le`: `if ((offset > 1.0) || (i == (ssize_t) gradient->number_stops))`
  - and 4 more (`tools/oracle/mutate.py --function DrawGradientImage`)

Commit message: `refactor(MagickCore/draw.c): simplify DrawGradientImage`

#### Step 6: `ConvertPathToPolygon` (line 523)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/draw.c): simplify ConvertPathToPolygon`

#### Step 7: `DrawPolygonPrimitive` (line 5122)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 5211, `sub_to_add`: `(bounds.y1 >= (double) image->rows) ||`
  - base line 5212, `sub_to_add`: `(bounds.x2 <= 0.0) || (bounds.y2 <= 0.0)) ? MagickTrue : MagickFalse);`

Commit message: `refactor(MagickCore/draw.c): simplify DrawPolygonPrimitive`

#### Step 8: `GetFillAlpha` (line 4943)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (7 arguments).

Commit message: `refactor(MagickCore/draw.c): simplify GetFillAlpha`

### Wave 3

#### Step 9: `DrawDashPolygon` (line 1756)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/draw.c): simplify DrawDashPolygon`

#### Step 10: `GetMVGMacros` (line 2374)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/draw.c): simplify GetMVGMacros`

#### Step 11: `TraceBezier` (line 6500)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/draw.c): simplify TraceBezier`

#### Step 12: `DrawAffineImage` (line 1161)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/draw.c): simplify DrawAffineImage`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/draw.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/draw.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/draw.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
