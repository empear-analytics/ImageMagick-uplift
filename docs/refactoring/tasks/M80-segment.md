# M80 - refactor `MagickCore/segment.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **3.47** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2000 lines |
| Functions with findings | 11 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/segment.c
```

The score must read 3.47. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 2 |
| Bumpy Road Ahead | 8 |
| Deep, Nested Complexity | 4 |
| Overall Code Complexity | file |
| Complex Method | 5 |
| Large Method | 4 |
| Excess Number of Function Arguments | 5 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `ConsolidateCrossings` | 705 | 22 | 4 | 7 | static | - | 29 | 52 of 52 |
| 2 | `Classify` | 249 | - | 4 | 9 | static | 2 | 29 | 108 of 108 |
| 3 | `InitializeIntervalTree` | 1345 | 10 | 5 | 2 | static | - | 29 | 22 of 22 |
| 4 | `ZeroCrossHistogram` | 1899 | 9 | - | 3 | static | - | 29 | 19 of 19 |
| 5 | `OptimalTau` | 1512 | 13 | - | - | static | - | 29 | 47 of 47 |
| 6 | `DefineRegion` | 821 | - | - | 2 | static | - | 29 | 11 of 11 |
| 7 | `ScaleSpace` | 1720 | - | - | 2 | static | - | 29 | 20 of 20 |
| 8 | `SegmentImage` | 1800 | 10 | - | - | public | - | 29 | 11 of 11 |
| 9 | `GetImageDynamicThreshold` | 936 | - | 4 | 8 | public | - | 0 | - |
| 10 | `ForceCrossingEndpoints` | 1543 | - | - | 2 | not compiled | - | - | - |
| 11 | `ClassifyFuzzyPixel` | 338 | - | - | - | not compiled | - | - | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `ConsolidateCrossings` (line 705)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/segment.c): simplify ConsolidateCrossings`

#### Step 2: `Classify` (line 249)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.

Commit message: `refactor(MagickCore/segment.c): simplify Classify`

#### Step 3: `InitializeIntervalTree` (line 1345)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/segment.c): simplify InitializeIntervalTree`

#### Step 4: `ZeroCrossHistogram` (line 1899)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/segment.c): simplify ZeroCrossHistogram`

### Wave 2

#### Step 5: `OptimalTau` (line 1512)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).

Commit message: `refactor(MagickCore/segment.c): simplify OptimalTau`

#### Step 6: `DefineRegion` (line 821)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.

Commit message: `refactor(MagickCore/segment.c): simplify DefineRegion`

#### Step 7: `ScaleSpace` (line 1720)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.

Commit message: `refactor(MagickCore/segment.c): simplify ScaleSpace`

#### Step 8: `SegmentImage` (line 1800)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/segment.c): simplify SegmentImage`

### Wave 3

#### Step 9: `GetImageDynamicThreshold` (line 936)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 10: `ForceCrossingEndpoints` (line 1543)

- **This function is not compiled in this build** (it sits in a preprocessor branch for a platform or a library that is not present). Nothing can check it here. Do not change it.

#### Step 11: `ClassifyFuzzyPixel` (line 338)

- **This function is not compiled in this build** (it sits in a preprocessor branch for a platform or a library that is not present). Nothing can check it here. Do not change it.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/segment.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/segment.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/segment.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-26, Mac, night): second pass

**Result: Code Health 3.47 → 3.61** (`cs-mcp` 1.5.12). Branch `campaign/M80-segment-e`,
stacked on `campaign/M79-identify-e`. Seven E commits:
- InitializeIntervalTree (ready, oracle): `AppendIntervalNode` and
  `AcquireRootInterval`, both value-returning.
- Classify (careful): `CountClusterPixel` and `ClassifyPixelByCluster`, both
  value-returning. These were taken at guard level, because this file's mutation
  report predates the M10 edits and its survivor lines now fall on comments.
  `ClassifyPixelByCluster` sits inside the row loop that OpenMP would parallelise; it
  writes only row-private state.
- ZeroCrossHistogram (ready, oracle): `MergeSmallDerivatives`. It leaves Complex Method.
- OptimalTau (not ready, guard): `GetAverageTau` (flat, named its next step) and
  `FindTauIndex`.

**What is left, at 3.61.**
- **Stale mutation report.** segment.c's survivor lines no longer match the code. A
  fresh mutation run would turn Classify and ConsolidateCrossings back into
  oracle-level targets.
- ConsolidateCrossings' three candidate checks (center, left, right) are a Recipe D,
  waiting for that run.
- GetImageDynamicThreshold has no case. Its blocks mirror Classify's, so reuse would
  be D.
- InitializeIntervalTree keeps depth 5: its allocation-failure branch returns from
  inside the loops.
