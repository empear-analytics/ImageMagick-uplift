# M20 - refactor `MagickCore/composite.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.33** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 3816 lines |
| Functions with findings | 12 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/composite.c
```

The score must read 1.33. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 4 |
| Bumpy Road Ahead | 11 |
| Deep, Nested Complexity | 8 |
| Overall Code Complexity | file |
| Complex Method | 8 |
| Complex Conditional | 10 |
| Code Duplication | 4 |
| Large Method | 11 |
| Excess Number of Function Arguments | 6 |
| Overall Function Size | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `CompositeImage` | 1412 | - | 6 | 43 | public | 4 | 1664 | 66 of 100 |
| 2 | `TextureImage` | 3651 | 32 | 5 | 7 | public | 1 | 9 | 2 of 5 |
| 3 | `BlendMeanImage` | 681 | 24 | 4 | 4 | static | 1 | 20 | 2 of 2 |
| 4 | `CompositeOverImage` | 905 | - | 5 | 7 | static | 2 | 553 | 5 of 8 |
| 5 | `BlendRMSEResidual` | 808 | 17 | 4 | 3 | static | 1 | 20 | 2 of 2 |
| 6 | `BlendMagnitudeImage` | 214 | 17 | 4 | 2 | static | 1 | 8 | 1 of 1 |
| 7 | `BlendMaxMagnitudeImage` | 300 | - | 4 | 2 | static | 1 | 8 | 1 of 1 |
| 8 | `BlendSumImage` | 403 | - | 4 | 2 | static | 1 | 20 | - |
| 9 | `SeamlessBlendImage` | 1279 | 20 | - | 2 | static | - | 12 | 4 of 4 |
| 10 | `SaliencyBlendImage` | 1149 | 18 | - | 2 | static | - | 8 | 3 of 3 |
| 11 | `BlendMaskAlphaChannel` | 616 | 11 | - | 2 | static | 1 | 20 | 0 of 1 |
| 12 | `BlendDivergentImage` | 490 | 20 | - | - | static | - | 8 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `CompositeImage` (line 1412)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 43 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 4 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 30 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1512, `lt_to_ge`: `if (colorspace_type < 0)`
  - base line 1525, `remove_void_call`: `SetGeometryInfo(&geometry_info);`
  - base line 1757, `mul_to_div`: `height=2.0*geometry_info.sigma;`
  - base line 1799, `remove_void_call`: `SetResampleFilter(resample_filter,GaussianFilter);`
  - base line 1803, `remove_void_call`: `GetPixelInfo(image,&pixel);`
  - base line 1806, `lt_to_ge`: `for (y=0; y < (ssize_t) source_image->rows; y++)`
  - and 24 more (`tools/oracle/mutate.py --function CompositeImage`)
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/composite.c): simplify CompositeImage`

#### Step 2: `TextureImage` (line 3651)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- Only 9 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3685, `ne_to_eq`: `((image->compose != OverCompositeOp) ||`
  - base line 3775, `add_to_sub`: `if ((x+(ssize_t) width) > (ssize_t) image->columns)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/composite.c): simplify TextureImage`

#### Step 3: `BlendMeanImage` (line 681)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.

Commit message: `refactor(MagickCore/composite.c): simplify BlendMeanImage`

#### Step 4: `CompositeOverImage` (line 905)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 978, `lt_to_le`: `((y-y_offset) < (ssize_t) source_image->rows))`
  - base line 998, `remove_void_call`: `GetPixelInfo(image,&canvas_pixel);`
  - base line 999, `remove_void_call`: `GetPixelInfo(source_image,&source_pixel);`

Commit message: `refactor(MagickCore/composite.c): simplify CompositeOverImage`

### Wave 2

#### Step 5: `BlendRMSEResidual` (line 808)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.

Commit message: `refactor(MagickCore/composite.c): simplify BlendRMSEResidual`

#### Step 6: `BlendMagnitudeImage` (line 214)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Only 8 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/composite.c): simplify BlendMagnitudeImage`

#### Step 7: `BlendMaxMagnitudeImage` (line 300)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Only 8 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/composite.c): simplify BlendMaxMagnitudeImage`

#### Step 8: `BlendSumImage` (line 403)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.

Commit message: `refactor(MagickCore/composite.c): simplify BlendSumImage`

### Wave 3

#### Step 9: `SeamlessBlendImage` (line 1279)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (8 arguments).
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/composite.c): simplify SeamlessBlendImage`

#### Step 10: `SaliencyBlendImage` (line 1149)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (8 arguments).
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Only 8 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/composite.c): simplify SaliencyBlendImage`

#### Step 11: `BlendMaskAlphaChannel` (line 616)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/composite.c): simplify BlendMaskAlphaChannel`

#### Step 12: `BlendDivergentImage` (line 490)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Only 8 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/composite.c): simplify BlendDivergentImage`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/composite.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/composite.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/composite.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, overnight)

**Result: Code Health 1.33 → 1.58** (`cs-mcp` 1.5.12). Branch `campaign/M20-composite`,
stacked on `campaign/M19-semaphore` (PR #25). 7 refactor commits, all Recipe P, one
function each: 2 oracle-verified (`CompositeImage`, `TextureImage`) and 5
guard-verified (the blend kernels have no mutant in the reports, and
`CompositeOverImage` is not ready).

**One category leaves the file:** Complex Conditional, ten sites, through five
predicates:
- `IsBlendRowMissing` and `IsBlendRowsMissing` (missing rows);
- `IsBlendChannelSkipped` (four kernels);
- `IsOutsideSourceColumn` (`CompositeOverImage` and `CompositeImage`);
- `IsTextureComposited`.

Six of the commits reuse a predicate that an earlier commit introduced. At guard level
that shows `--calls` +1, the case raised as a question in #16.

**Whole catalogue** at the branch tip against `8a4c272f1`: 15,884 cases, 1 diverged
(`gaps/c5651ffad8`, the known log-map tie).

**Process note.** The first two commits were redone before any push. The predicate tool
bounded a function by the next documentation header. Between the blend kernels, which
are static and have no headers, that slice ran into the following functions, and one
commit changed `BlendSumImage` alongside `BlendMagnitudeImage`. The tool now bounds a
function by brace matching. The branch was reset to the task-file commit and the steps
were made again, one function each. Earlier tasks' uses of the tool were checked: they
were public functions with headers, or expressions unique in the file.

**What is left, at 1.58.**
- `CompositeImage` (nesting 6, one `switch` over the compose operators inside its
  parallel loop).
- `CompositeOverImage` and `TextureImage` (nesting 5).
- The blend kernels' channel loops. They are near-duplicates whose bodies differ in the
  arithmetic, so Recipe D does not apply, and extracting each separately would be
  reported as duplication.
