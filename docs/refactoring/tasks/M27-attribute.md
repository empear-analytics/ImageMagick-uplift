# M27 - refactor `MagickCore/attribute.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **2.58** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2445 lines |
| Functions with findings | 16 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/attribute.c
```

The score must read 2.58. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 14 |
| Deep, Nested Complexity | 4 |
| Overall Code Complexity | file |
| Complex Method | 14 |
| Complex Conditional | 2 |
| Code Duplication | 3 |
| Large Method | 10 |
| Excess Number of Function Arguments | 1 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `GetImageDepth` | 892 | 51 | 5 | 12 | public | 2 | 307 | 39 of 45 |
| 2 | `SetImageDepth` | 2076 | 38 | 4 | 5 | public | 3 | 10842 | 14 of 25 |
| 3 | `FloydSteinbergImageDepth` | 1951 | 19 | 5 | 3 | static | - | 4 | 44 of 44 |
| 4 | `GetEdgeBackgroundColor` | 602 | 17 | 4 | 4 | static | 1 | 34 | 27 of 27 |
| 5 | `GetImageMinimumBoundingBox` | 1204 | 25 | - | 6 | public | - | 30 | 85 of 85 |
| 6 | `GetImageBoundingBox` | 391 | 40 | - | 3 | public | 1 | 105 | 51 of 51 |
| 7 | `SetImageType` | 2299 | 37 | - | - | public | - | 266 | 23 of 25 |
| 8 | `GetImageConvexHull` | 768 | 15 | - | 4 | public | - | 34 | 21 of 21 |
| 9 | `GetEdgeBackgroundCensus` | 136 | 18 | - | 2 | static | - | 13 | 17 of 17 |
| 10 | `IdentifyImageGray` | 1566 | 16 | - | 2 | public | 1 | 1171 | 17 of 17 |
| 11 | `IdentifyImageMonochrome` | 1657 | 12 | - | 2 | public | 1 | 222 | 12 of 12 |
| 12 | `IsImageOpaque` | 1865 | 10 | - | 2 | public | 1 | 51 | 10 of 11 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `GetImageDepth` (line 892)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 12 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- 6 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/attribute.c): simplify GetImageDepth`

#### Step 2: `SetImageDepth` (line 2076)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 3 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 9 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2108, `eq_to_ne`: `if (image->storage_class == PseudoClass)`
  - base line 2117, `lt_to_ge`: `for (i=0; i < (ssize_t) image->colors; i++)`
  - base line 2117, `post_inc_to_post_dec`: `for (i=0; i < (ssize_t) image->colors; i++)`
  - base line 2119, `ne_to_eq`: `if ((GetPixelRedTraits(image) & UpdatePixelTrait) != 0)`
  - base line 2122, `ne_to_eq`: `if ((GetPixelGreenTraits(image) & UpdatePixelTrait) != 0)`
  - base line 2125, `ne_to_eq`: `if ((GetPixelBlueTraits(image) & UpdatePixelTrait) != 0)`
  - and 3 more (`tools/oracle/mutate.py --function SetImageDepth`)
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/attribute.c): simplify SetImageDepth`

#### Step 3: `FloydSteinbergImageDepth` (line 1951)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Only 4 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/attribute.c): simplify FloydSteinbergImageDepth`

#### Step 4: `GetEdgeBackgroundColor` (line 602)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.

Commit message: `refactor(MagickCore/attribute.c): simplify GetEdgeBackgroundColor`

### Wave 2

#### Step 5: `GetImageMinimumBoundingBox` (line 1204)

- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/attribute.c): simplify GetImageMinimumBoundingBox`

#### Step 6: `GetImageBoundingBox` (line 391)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/attribute.c): simplify GetImageBoundingBox`

#### Step 7: `SetImageType` (line 2299)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Public function: its name and signature must not change.
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/attribute.c): simplify SetImageType`

#### Step 8: `GetImageConvexHull` (line 768)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/attribute.c): simplify GetImageConvexHull`

### Wave 3

#### Step 9: `GetEdgeBackgroundCensus` (line 136)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (8 arguments).

Commit message: `refactor(MagickCore/attribute.c): simplify GetEdgeBackgroundCensus`

#### Step 10: `IdentifyImageGray` (line 1566)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/attribute.c): simplify IdentifyImageGray`

#### Step 11: `IdentifyImageMonochrome` (line 1657)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/attribute.c): simplify IdentifyImageMonochrome`

#### Step 12: `IsImageOpaque` (line 1865)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

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

### Report (2026-10-25, Mac, morning)

**Result: Code Health 2.58 → 2.97** (`cs-mcp` 1.5.12). Branch `campaign/M27-attribute`,
stacked on `campaign/M26-effect`. 11 refactor commits, all oracle-verified: 1 P, 7 E,
3 D. Every function touched is ready in the merged mutation reports. The baseline is
`f8b59598e` (`origin/main` after PR #30).

**GetImageBoundingBox:** `IsEmptyBoundingBox` (+0.15). The second site, in the parallel
loop, would need five arguments and stays.

**GetImageDepth** (bumps 12 → 8, cc 51 → 37):
- `GetMaximumDepth`, by E and then D. A third copy sits in the `!MAGICKCORE_HDRI_SUPPORT`
  branch, which the HDRI oracle build does not compile, and stays untouched.
- `RaiseColormapEntryDepth`.

**SetImageDepth** (cc 38 → 34): `SetColormapEntryDepth`, the body of a parallel loop over
colormap entries. Each iteration writes only its own entry.

**SetImageType** (cc 37 → 33): `SetOpaqueAlphaIfMissing`, by E (+0.13) and then D (three
copies). The ColorSeparationAlpha guard assigns `status` and stays.

**GetImageMinimumBoundingBox** (bumps 6 → 4, cc 25 → 19, 189 → 163 lines):
- `GetMinimumBoxAngle` and `MoveBoxToOrigin` (+0.07). Each moves a variable that is dead
  afterwards (`distance`, `point`), because the edge loop reassigns it at `i == 0`
  before reading it.
- `SetMinimumBoundingBoxProperties`.

**Flakes:** the hough matrix-on-disk pair diverged once during the SetImageDepth step and
gave 0 of 2 in three reruns.

**Whole catalogue:** run before the batch PR.

**What is left, at 2.97.**
- `SetImageType`'s remaining guards (sRGB, gray, CMYK, DirectClass). A probe showed that
  folding them leaves the score flat and the function still far above the cc threshold.
- The pixel loops of `GetImageDepth`, `SetImageDepth`, `GetImageBoundingBox` and the
  identify functions. All are parallel, need Recipe K or T, and some are in the
  uncompiled `!HDRI` branches.
- The calipers loop in `GetImageMinimumBoundingBox`. Its inner loops each write two or
  more of the iteration's locals.
- `GetEdgeBackgroundCensus` takes 8 arguments (Recipe B).
