# M83 - refactor `MagickCore/attribute.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **2.97** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2504 lines |
| Functions with findings | 17 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/attribute.c
```

The score must read 2.97. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 15 |
| Deep, Nested Complexity | 4 |
| Overall Code Complexity | file |
| Complex Method | 15 |
| Complex Conditional | 1 |
| Large Method | 10 |
| Excess Number of Function Arguments | 1 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `GetImageDepth` | 888 | 37 | 5 | 8 | public | 2 | 307 | 39 of 45 |
| 2 | `SetImageDepth` | 2078 | 34 | 4 | 5 | public | 2 | 10842 | 14 of 25 |
| 3 | `FloydSteinbergImageDepth` | 1953 | 19 | 5 | 3 | static | - | 4 | 44 of 44 |
| 4 | `GetEdgeBackgroundColor` | 604 | 17 | 4 | 4 | static | 1 | 34 | 27 of 27 |
| 5 | `GetImageBoundingBox` | 393 | 37 | - | 3 | public | 1 | 105 | 51 of 51 |
| 6 | `GetImageMinimumBoundingBox` | 1206 | 19 | - | 4 | public | - | 30 | 85 of 85 |
| 7 | `GetImageConvexHull` | 770 | 15 | - | 4 | public | - | 34 | 21 of 21 |
| 8 | `SetImageType` | 2301 | 33 | - | - | public | - | 266 | 23 of 25 |
| 9 | `GetEdgeBackgroundCensus` | 136 | 18 | - | 2 | static | - | 13 | 17 of 17 |
| 10 | `IdentifyImageGray` | 1568 | 16 | - | 2 | public | - | 1171 | 17 of 17 |
| 11 | `IdentifyImageMonochrome` | 1659 | 12 | - | 2 | public | - | 222 | 12 of 12 |
| 12 | `IsImageOpaque` | 1867 | 10 | - | 2 | public | - | 51 | 10 of 11 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `GetImageDepth` (line 888)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 8 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 6 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 916, `ne_to_eq`: `while (current_depth[id] < MAGICKCORE_QUANTUM_DEPTH)`
  - base line 924, `lt_to_le`: `atDepth=MagickTrue;`
  - base line 961, `lt_to_ge`: ``
  - base line 961, `post_inc_to_post_dec`: ``
  - base line 1106, `lt_to_ge`: `traits;`
  - base line 1106, `post_inc_to_post_dec`: `traits;`

Commit message: `refactor(MagickCore/attribute.c): simplify GetImageDepth`

#### Step 2: `SetImageDepth` (line 2078)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 9 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2108, `eq_to_ne`: `}`
  - base line 2117, `lt_to_ge`: `continue;`
  - base line 2117, `post_inc_to_post_dec`: `continue;`
  - base line 2119, `ne_to_eq`: `}`
  - base line 2122, `ne_to_eq`: `if (status != MagickFalse)`
  - base line 2125, `ne_to_eq`: `}`
  - and 3 more (`tools/oracle/mutate.py --function SetImageDepth`)
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/attribute.c): simplify SetImageDepth`

#### Step 3: `FloydSteinbergImageDepth` (line 1953)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Only 4 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/attribute.c): simplify FloydSteinbergImageDepth`

#### Step 4: `GetEdgeBackgroundColor` (line 604)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.

Commit message: `refactor(MagickCore/attribute.c): simplify GetEdgeBackgroundColor`

### Wave 2

#### Step 5: `GetImageBoundingBox` (line 393)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/attribute.c): simplify GetImageBoundingBox`

#### Step 6: `GetImageMinimumBoundingBox` (line 1206)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/attribute.c): simplify GetImageMinimumBoundingBox`

#### Step 7: `GetImageConvexHull` (line 770)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/attribute.c): simplify GetImageConvexHull`

#### Step 8: `SetImageType` (line 2301)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2315, `ne_to_eq`: `continue;`
  - base line 2388, `eq_to_ne`: `{`

Commit message: `refactor(MagickCore/attribute.c): simplify SetImageType`

### Wave 3

#### Step 9: `GetEdgeBackgroundCensus` (line 136)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (8 arguments).

Commit message: `refactor(MagickCore/attribute.c): simplify GetEdgeBackgroundCensus`

#### Step 10: `IdentifyImageGray` (line 1568)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/attribute.c): simplify IdentifyImageGray`

#### Step 11: `IdentifyImageMonochrome` (line 1659)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/attribute.c): simplify IdentifyImageMonochrome`

#### Step 12: `IsImageOpaque` (line 1867)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1882, `ne_to_eq`: `assert(image->signature == MagickCoreSignature);`

Commit message: `refactor(MagickCore/attribute.c): simplify IsImageOpaque`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/attribute.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/attribute.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/attribute.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
