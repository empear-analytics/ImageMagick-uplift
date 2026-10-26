# M51 - refactor `MagickCore/resample.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **4.09** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1464 lines |
| Functions with findings | 6 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/resample.c
```

The score must read 4.09. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Brain Method | 1 |
| Bumpy Road Ahead | 1 |
| Deep, Nested Complexity | 1 |
| Overall Code Complexity | file |
| Complex Conditional | 4 |
| Code Duplication | 2 |
| Large Method | 3 |
| Excess Number of Function Arguments | 3 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `ResamplePixelColor` | 315 | - | 4 | 5 | public | - | 793 | 12 of 25 |
| 2 | `SetResampleFilterInterpolateMethod` | 1411 | - | - | - | public | - | 793 | - |
| 3 | `SetResampleFilterVirtualPixelMethod` | 1450 | - | - | - | public | - | 793 | - |
| 4 | `ScaleResampleFilter` | 1045 | - | - | - | public | - | 793 | 5 of 5 |
| 5 | `SetResampleFilter` | 1248 | - | - | - | public | 1 | 793 | 1 of 2 |
| 6 | `ClampUpAxes` | 726 | - | - | - | static | - | 777 | 10 of 11 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `ResamplePixelColor` (line 315)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- The oracle missed 13 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 361, `lt_to_le`: `|| u0 + resample_filter->Ulimit < 0.0`
  - base line 363, `lt_to_le`: `|| v0 + resample_filter->Vlimit < 0.0`
  - base line 374, `gt_to_ge`: `|| ( u0 - resample_filter->Ulimit > (double) resample_filter->image->columns-1.0`
  - base line 374, `sub_to_add`: `|| ( u0 - resample_filter->Ulimit > (double) resample_filter->image->columns-1.0`
  - base line 383, `gt_to_ge`: `|| v0 - resample_filter->Vlimit > (double) resample_filter->image->rows-1.0`
  - base line 385, `post_inc_to_post_dec`: `hit++;  /* outside the horizontally tiled images. */`
  - and 7 more (`tools/oracle/mutate.py --function ResamplePixelColor`)

Commit message: `refactor(MagickCore/resample.c): simplify ResamplePixelColor`

#### Step 2: `SetResampleFilterInterpolateMethod` (line 1411)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/resample.c): simplify SetResampleFilterInterpolateMethod`

#### Step 3: `SetResampleFilterVirtualPixelMethod` (line 1450)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/resample.c): simplify SetResampleFilterVirtualPixelMethod`

#### Step 4: `ScaleResampleFilter` (line 1045)

- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/resample.c): simplify ScaleResampleFilter`

### Wave 2

#### Step 5: `SetResampleFilter` (line 1248)

- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1371, `post_inc_to_post_dec`: `for(Q=0; Q<WLUT_WIDTH; Q++)`

Commit message: `refactor(MagickCore/resample.c): simplify SetResampleFilter`

#### Step 6: `ClampUpAxes` (line 726)

- No recipe in the catalogue reaches *Excess Number of Function Arguments* (10 arguments).
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resample.c): simplify ClampUpAxes`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/resample.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/resample.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/resample.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, evening)

**Result: Code Health 4.09 → 5.13** (`cs-mcp` 1.5.12). Branch `campaign/M51-resample`,
stacked on `campaign/M50-utility`. ResamplePixelColor is *not ready* (25 mutants, 12
killed, 11 unmatched), so all three commits are guard-verified. The oracle also ran
on each one: 793 cases, 0 diverged.
- **P (1 commit, 4.09 → 5.11).** Four predicates for the virtual-pixel limit tests:
  `IsAreaOutsideImage`, `IsAreaOutsideCorner`, `IsAreaOutsideDitherCorner` and
  `IsResampleUnweighted`. The originals use upstream's `if ( … )` style, with lines
  past 80 columns. The predicates are rewrapped at their operators, and their tokens
  were checked mechanically against the originals. Complex Conditional leaves the
  file. CodeScene now pairs the two corner predicates as duplicates; they differ in
  four literals, so Recipe D does not apply.
- **E (2 commits).** `BlendCheckerTileAverage` was flat at 5.11 and named its next
  step. `ResetPixelAccumulator` then reached 5.13.

**Reverted.** The virtual-pixel switch as `CountVirtualPixelHits` (value-returning)
scored 5.11 → 5.09, because the helper carries the switch's cc 25.

**What is left, at 5.13.**
- `ResamplePixelColor` is still a Brain Method (228 lines). Its whole-image average
  block leaves the switch with `break`. Its accumulation loop interleaves its braces
  with `#if FILTER_LUT` and `DEBUG_HIT_MISS`, so it is off limits.
- `ScaleResampleFilter` and `SetResampleFilter` are built from `#if EWA`, `EWA_CLAMP`,
  `FILTER_LUT` and `DEBUG_ELLIPSE` branches, most of them uncompiled.
- `ClampUpAxes` takes 10 arguments (Recipe B).
- The two `SetResampleFilter*Method` setters share only the assert-and-log prologue,
  which carries `GetMagickModule()`.
