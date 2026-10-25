# M37 - refactor `MagickCore/draw.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.28** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 7977 lines |
| Functions with findings | 30 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/draw.c
```

The score must read 1.28. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 1 |
| Bumpy Road Ahead | 18 |
| Deep, Nested Complexity | 10 |
| Overall Code Complexity | file |
| Complex Method | 25 |
| Complex Conditional | 20 |
| Large Method | 20 |
| Excess Number of Function Arguments | 5 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `RenderMVGContent` | 2520 | 483 | 6 | 74 | static | - | 476 | 470 of 473 |
| 2 | `TracePath` | 6736 | 167 | 5 | 11 | static | - | 50 | 167 of 167 |
| 3 | `DrawPrimitive` | 5483 | 90 | 5 | 12 | public | - | 436 | 99 of 99 |
| 4 | `TraceStrokePolygon` | 7467 | 75 | 4 | 10 | static | - | 47 | 227 of 227 |
| 5 | `DrawGradientImage` | 2014 | 46 | 6 | 9 | public | 1 | 73 | 95 of 105 |
| 6 | `ConvertPathToPolygon` | 521 | 41 | 5 | 7 | static | - | 358 | 56 of 56 |
| 7 | `DrawPolygonPrimitive` | 5119 | 49 | 4 | 5 | static | 2 | 362 | 66 of 68 |
| 8 | `DrawDashPolygon` | 1755 | 33 | 4 | 5 | static | - | 6 | 64 of 64 |
| 9 | `GetFillAlpha` | 4940 | - | 6 | 9 | static | - | 356 | 110 of 110 |
| 10 | `GetMVGMacros` | 2372 | 16 | 6 | 4 | static | - | 476 | 20 of 20 |
| 11 | `TraceBezier` | 6498 | 18 | - | 4 | static | - | 40 | 41 of 41 |
| 12 | `DrawAffineImage` | 1159 | 23 | - | 3 | public | 1 | 13 | 45 of 45 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `RenderMVGContent` (line 2520)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 74 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2603, `ge_to_gt`: `if (depth >= MagickMaxRecursionDepth)`
  - base line 3137, `eq_to_ne`: `if (weight == -1)`
  - base line 4676, `lt_to_le`: `if ((status == MagickFalse) && (exception->severity < ErrorException))`

Commit message: `refactor(MagickCore/draw.c): simplify RenderMVGContent`

#### Step 2: `TracePath` (line 6736)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 11 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/draw.c): simplify TracePath`

#### Step 3: `DrawPrimitive` (line 5483)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 12 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/draw.c): simplify DrawPrimitive`

#### Step 4: `TraceStrokePolygon` (line 7467)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 10 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/draw.c): simplify TraceStrokePolygon`

### Wave 2

#### Step 5: `DrawGradientImage` (line 2014)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 9 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2056, `remove_void_call`: `qsort(gradient->stops,gradient->number_stops,sizeof(StopInfo),`
  - base line 2064, `remove_void_call`: `GetPixelInfo(image,&zero);`
  - base line 2109, `remove_void_call`: `GetPixelInfoPixel(image,q,&pixel);`
  - base line 2150, `lt_to_le`: `if (offset < 0.0)`
  - base line 2157, `lt_to_le`: `if (offset < gradient->stops[i].offset)`
  - base line 2196, `lt_to_le`: `antialias=(repeat < length) && ((repeat+1.0) > length) ?`
  - and 3 more (`tools/oracle/mutate.py --function DrawGradientImage`)
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/draw.c): simplify DrawGradientImage`

#### Step 6: `ConvertPathToPolygon` (line 521)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/draw.c): simplify ConvertPathToPolygon`

#### Step 7: `DrawPolygonPrimitive` (line 5119)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 5211, `sub_to_add`: `bounds.x1=bounds.x1 < 0.0 ? 0.0 : bounds.x1 >= (double) image->columns-1.0 ?`
  - base line 5212, `sub_to_add`: `(double) image->columns-1.0 : bounds.x1;`

Commit message: `refactor(MagickCore/draw.c): simplify DrawPolygonPrimitive`

#### Step 8: `DrawDashPolygon` (line 1755)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/draw.c): simplify DrawDashPolygon`

### Wave 3

#### Step 9: `GetFillAlpha` (line 4940)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (7 arguments).

Commit message: `refactor(MagickCore/draw.c): simplify GetFillAlpha`

#### Step 10: `GetMVGMacros` (line 2372)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/draw.c): simplify GetMVGMacros`

#### Step 11: `TraceBezier` (line 6498)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/draw.c): simplify TraceBezier`

#### Step 12: `DrawAffineImage` (line 1159)

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

### Report (2026-10-25, Mac, late morning)

**Result: Code Health 1.28 → 1.77** (`cs-mcp` 1.5.12). Branch `campaign/M37-draw`,
stacked on `campaign/M36-image`. 12 refactor commits plus 1 whitespace-only style commit.

**Recipe P throughout: 22 predicates, and Complex Conditional leaves the file (20 sites).**
- Each site was checked against every open survivor kind (unmatched, unreached, gap,
  unresolved) on its exact lines.
- The 11 clean sites went in **oracle-verified** commits: ConvertPathToPolygon ×2,
  ConvertPrimitiveToPath, CheckPrimitiveExtent, RenderMVGContent ×2,
  DrawPolygonPrimitive, DrawPrimitive ×3, TraceStrokePolygon.
- The 9 sites with a survivor on their lines went in **guard-verified** commits:
  DrawDashPolygon, GetFillAlpha, DrawPrimitive ×2, RenderMVGContent ×5.
- Where one function had both kinds, they were split into two commits.

**Two notes on the predicates:**
- Four predicates take five arguments: `IsNewSubpathPoint`, `IsDashSegmentPending`,
  `IsRenderableClass`, and the gap ones in image.c. Each carries an Excess Arguments
  finding; the file gained every time.
- Several predicates take `char *` parameters. Each was checked: draw.c gains no String
  Heavy Function Arguments finding.

**Generator.** The predicates were written by a small generator that copies the
condition verbatim from the source and keeps its own line breaks. One call was left on a
93-column line; a whitespace-only style commit wrapped it.

**Oracle baseline:** from the ConvertPrimitiveToPath commit on, `8c405c0cc`
(`origin/main` after the owner merged #31 and #32).

**What is left, at 1.77.**
- `RenderMVGContent` (74 bumps): a keyword `switch` over the MVG language, whose cases
  share the parser's state (`q`, `token`, `graphic_context`, `n`, `primitive_type` and
  more).
- `DrawPrimitive`, `TracePath`, `TraceStrokePolygon`, `DrawGradientImage`, `GetFillAlpha`
  (9–12 bumps). Mostly parallel or point-generation loops that write several locals.
- `DrawBoundingRectangles` and `LogPrimitiveInfo`: no oracle case.
