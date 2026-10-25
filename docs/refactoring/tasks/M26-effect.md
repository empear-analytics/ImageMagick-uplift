# M26 - refactor `MagickCore/effect.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.38** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 4406 lines |
| Functions with findings | 18 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/effect.c
```

The score must read 1.38. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 6 |
| Bumpy Road Ahead | 14 |
| Deep, Nested Complexity | 11 |
| Overall Code Complexity | file |
| Complex Method | 9 |
| Complex Conditional | 1 |
| Code Duplication | 6 |
| Large Method | 14 |
| Excess Number of Function Arguments | 6 |
| Overall Function Size | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `PreviewImage` | 2611 | 72 | 4 | 6 | public | - | 29 | 77 of 77 |
| 2 | `AdaptiveBlurImage` | 129 | 44 | 6 | 9 | public | 2 | 12 | 86 of 86 |
| 3 | `AdaptiveSharpenImage` | 448 | 44 | 6 | 9 | public | 2 | 15 | 89 of 90 |
| 4 | `RotationalBlurImage` | 3143 | 39 | 6 | 8 | public | 2 | 41 | 60 of 60 |
| 5 | `KuwaharaImage` | 1789 | 30 | 5 | 4 | public | 2 | 12 | 52 of 52 |
| 6 | `BilateralBlurImage` | 900 | - | 6 | 8 | public | 2 | 14 | 91 of 91 |
| 7 | `SelectiveBlurImage` | 3420 | - | 7 | 7 | public | 2 | 12 | 91 of 91 |
| 8 | `DespeckleImage` | 1316 | 32 | - | 5 | public | - | 37 | 54 of 54 |
| 9 | `ShadeImage` | 3759 | - | 5 | 6 | public | 2 | 17 | 54 of 54 |
| 10 | `MotionBlurImage` | 2361 | - | 6 | 5 | public | 2 | 53 | 48 of 48 |
| 11 | `LocalContrastImage` | 2027 | 34 | - | 4 | public | 2 | 44 | 94 of 94 |
| 12 | `Hull` | 1219 | - | 4 | 4 | static | 2 | 37 | 56 of 56 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `PreviewImage` (line 2611)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/effect.c): simplify PreviewImage`

#### Step 2: `AdaptiveBlurImage` (line 129)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/effect.c): simplify AdaptiveBlurImage`

#### Step 3: `AdaptiveSharpenImage` (line 448)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/effect.c): simplify AdaptiveSharpenImage`

#### Step 4: `RotationalBlurImage` (line 3143)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 8 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/effect.c): simplify RotationalBlurImage`

### Wave 2

#### Step 5: `KuwaharaImage` (line 1789)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/effect.c): simplify KuwaharaImage`

#### Step 6: `BilateralBlurImage` (line 900)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 8 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/effect.c): simplify BilateralBlurImage`

#### Step 7: `SelectiveBlurImage` (line 3420)

- **Recipe G (guard clauses)** - nesting is 7, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/effect.c): simplify SelectiveBlurImage`

#### Step 8: `DespeckleImage` (line 1316)

- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/effect.c): simplify DespeckleImage`

### Wave 3

#### Step 9: `ShadeImage` (line 3759)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/effect.c): simplify ShadeImage`

#### Step 10: `MotionBlurImage` (line 2361)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/effect.c): simplify MotionBlurImage`

#### Step 11: `LocalContrastImage` (line 2027)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/effect.c): simplify LocalContrastImage`

#### Step 12: `Hull` (line 1219)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (8 arguments).
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.

Commit message: `refactor(MagickCore/effect.c): simplify Hull`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/effect.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/effect.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/effect.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, morning)

**Result: Code Health 1.38 → 1.40** (`cs-mcp` 1.5.12). Branch `campaign/M26-effect`,
stacked on `campaign/M25-quantize`. 4 refactor commits.

**The commits:**
- `IsSelectiveRowMissing` (P, oracle): Complex Conditional leaves the file.
- `GetAdaptiveKernelIndex`, the kernel-index clamp inside the parallel row loops: E in
  `AdaptiveBlurImage` (guard), then D in `AdaptiveSharpenImage` (oracle). Both functions
  go from cc 44 to 41; the score stays flat (rule 2).
- `ConvolveKernelGeometry` (E, step 7 in its value-returning form, oracle): the
  kernel-acquire-convolve tail of `GaussianBlurImage`. The Code Duplication findings on
  `GaussianBlurImage` and `BlurImage` go away.

**For the owner: two equivalent mutants.** Both kernel-index steps move lines that carry
mutants of a clamp: `j < 0` turned into `j <= 0`, and `j > width` turned into
`j >= width`. At the boundary the clamp assigns the value `j` already has, so the
mutants are equivalent and no case can kill them. The E step was nevertheless kept at
guard level. The D step, which needs the oracle, relies on that equivalence; its message
says so.

**Oracle baseline.** From the `ConvolveKernelGeometry` commit on, the oracle's default
baseline is `f8b59598e`: `origin/main` after the owner merged PRs #7–#30.

**Blocked:** `BlurImage` holds the same tail as `GaussianBlurImage` but cannot call the
helper. Its `blur_image` is also used in the uncompiled OpenCL branch, so the declaration
can neither be removed nor be left unused.

**What is left, at 1.40.**
- Fourteen functions with Bumpy Road. Nearly all their bumps are the bodies of parallel
  row loops (`AdaptiveBlurImage`, `BilateralBlurImage`, `SelectiveBlurImage`,
  `KuwaharaImage`, `LocalContrastImage` and others), which need Recipe K or T.
- The adaptive pair's per-channel loop is shared, but it reads eight variables, so it
  needs Recipe B.
- `PreviewImage` (cc 72): its preview `switch` threads about a dozen locals through the
  tile loop, which is too many even for Recipe O.
- Six functions with 5–8 arguments (Recipe B).
