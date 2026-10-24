# M10 - refactor `MagickCore/segment.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **2.30** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1935 lines |
| Functions with findings | 9 |
| Measured with | `cs-mcp` 1.5.12, `cs` not installed |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/segment.c
```

The score must read 2.30. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 3 |
| Bumpy Road Ahead | 8 |
| Deep, Nested Complexity | 5 |
| Overall Code Complexity | file |
| Complex Method | 4 |
| Complex Conditional | 4 |
| Large Method | 4 |
| Excess Number of Function Arguments | 4 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `ConsolidateCrossings` | 703 | 28 | 5 | 7 | static | - | 29 | 53 of 59 |
| 2 | `Classify` | 246 | - | 5 | 11 | static | 3 | 29 | 111 of 118 |
| 3 | `OptimalTau` | 1509 | - | 4 | 5 | static | - | 29 | 50 of 65 |
| 4 | `InitializeIntervalTree` | 1343 | 10 | 5 | 2 | static | - | 29 | 22 of 22 |
| 5 | `ZeroCrossHistogram` | 1897 | 9 | - | 3 | static | - | 29 | 19 of 19 |
| 6 | `DefineRegion` | 820 | - | - | 2 | static | - | 29 | 11 of 13 |
| 7 | `ScaleSpace` | 1718 | - | - | 2 | static | - | 29 | 20 of 22 |
| 8 | `SegmentImage` | 1796 | 10 | - | - | public | - | 29 | 12 of 13 |
| 9 | `GetImageDynamicThreshold` | 933 | - | 4 | 8 | public | - | 0 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `ConsolidateCrossings` (line 703)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- 6 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/segment.c): simplify ConsolidateCrossings`

#### Step 2: `Classify` (line 246)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 11 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- Contains 3 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 512, `gt_to_ge`: `if (number_clusters > 256)`
  - base line 576, `remove_void_call`: `SetPixelIndex(image,(Quantum) 0,q);`
  - base line 629, `div_to_mul`: `ratio=numerator/distance_squared;`
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/segment.c): simplify Classify`

#### Step 3: `OptimalTau` (line 1509)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1604, `lt_to_ge`: `for (j=0; j < 255; j++)`
  - base line 1605, `ne_to_eq`: `if (zero_crossing[i].crossings[j] != 0)`
  - base line 1607, `minus_to_noop`: `zero_crossing[i].crossings[0]=(-zero_crossing[i].crossings[j]);`
  - base line 1688, `remove_void_call`: `FreeNodes(root);`
- 11 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/segment.c): simplify OptimalTau`

#### Step 4: `InitializeIntervalTree` (line 1343)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/segment.c): simplify InitializeIntervalTree`

### Wave 2

#### Step 5: `ZeroCrossHistogram` (line 1897)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/segment.c): simplify ZeroCrossHistogram`

#### Step 6: `DefineRegion` (line 820)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 834, `gt_to_ge`: `if (extents->index > 255)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/segment.c): simplify DefineRegion`

#### Step 7: `ScaleSpace` (line 1718)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/segment.c): simplify ScaleSpace`

#### Step 8: `SegmentImage` (line 1796)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/segment.c): simplify SegmentImage`

### Wave 3

#### Step 9: `GetImageDynamicThreshold` (line 933)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

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

### Report (2026-10-25, Mac, overnight)

**Result: Code Health 2.30 → 3.47** (`cs-mcp` 1.5.12). Branch `campaign/M10-segment`,
stacked on `campaign/M09-shear` (PR #15). 11 refactor commits, one recipe and one
function each:
- 7 Recipe E: 5 oracle-verified and 2 guard-verified (blocks on `Classify`'s survivor
  lines);
- 3 Recipe P: 1 oracle-verified and 2 guard-verified (`GetImageDynamicThreshold` has no
  case);
- 1 Recipe D, oracle-verified.

**One category leaves the file:** Complex Conditional. `IsPixelInCluster` serves both of
`Classify`'s cluster tests and `GetImageDynamicThreshold`'s; `HasObjectAndBackgroundClusters`
is the other predicate. Brain Method leaves `OptimalTau`.

| Steps | Recipe | Score |
| --- | --- | --- |
| the cluster-membership test, three sites, and the cluster-count test | P | 2.30 → 2.78 |
| `ConsolidateCrossings`' three crossing counts into one helper | E, D | → 2.81 |
| `OptimalTau`: forcing the end points, the extremum search, the extrema write | E | → 3.28 |
| `Classify`: the verbose statistics, the colormap fill, the fuzzy membership | E | → 3.47 |

**Whole catalogue** at the branch tip against `8a4c272f1`: 15,884 cases, 1 diverged
(`gaps/c5651ffad8`, the known log-map tie).

**A rule question for the owner.** `GetImageDynamicThreshold` reuses `IsPixelInCluster`.
At guard level that shows `--calls` +1, not the +2 of a new helper. It is one more call
to a predicate whose body holds no call, so nothing can have been dropped, but the
playbook's guard fingerprint does not name the case. Commit `3c2e71137` says so.

**What is left, at 3.47.**
- Brain Method: `Classify` and `GetImageDynamicThreshold`. Their cluster-allocation loops
  return (or throw through `ThrowClassifyException`) mid-loop, their pruning loops write
  the list head and the count together, and their counting loops add to the outer count.
- Deep Nesting: `InitializeIntervalTree` (its allocation failures return from inside the
  splitting loop) and `ConsolidateCrossings` (now 4: the three neighbour searches differ
  by an index offset or a direction, so no single helper covers them).
- Excess Arguments: `Classify`, `SegmentImage` (public), `OptimalTau`, and the 5-argument
  `ClassifyFuzzyPixel`.

**Process notes.**
- The verify script now touches the file before building. Without that, a re-run on an
  unchanged file did not recompile it, and so could not show its warnings.
- A function body is now bounded by the next documentation header, not the next
  column-0 `}`: `Classify` defines a macro inside itself whose last line is a `}`.
