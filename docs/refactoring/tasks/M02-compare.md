# M02 - refactor `MagickCore/compare.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.46** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 4988 lines |
| Functions with findings | 37 |
| Measured with | `cs-mcp` 1.5.12, `cs` not installed |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/compare.c
```

The score must read 1.46. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 1 |
| Bumpy Road Ahead | 26 |
| Deep, Nested Complexity | 24 |
| Overall Code Complexity | file |
| Complex Method | 31 |
| Complex Conditional | 3 |
| Code Duplication | 23 |
| Large Method | 25 |
| Excess Number of Function Arguments | 10 |
| Overall Function Size | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `GetSSIMSimularity` | 2076 | 35 | 6 | 6 | static | 2 | 20 | 76 of 78 |
| 2 | `GetDPCSimilarity` | 452 | 29 | 4 | 7 | static | 3 | 10 | 34 of 38 |
| 3 | `GetNCCSimilarity` | 1229 | 29 | 4 | 7 | static | 3 | 12 | 34 of 38 |
| 4 | `GetPHASESimilarity` | 1698 | 37 | 4 | 5 | static | 2 | 11 | 49 of 59 |
| 5 | `GetMEPPSimilarity` | 933 | 27 | 4 | 5 | static | 2 | 68 | 37 of 43 |
| 6 | `GetAESimilarity` | 307 | 26 | 4 | 5 | static | 2 | 11 | 38 of 39 |
| 7 | `GetFUZZSimilarity` | 648 | 25 | 4 | 5 | static | 2 | 10 | 37 of 40 |
| 8 | `GetMAESimilarity` | 791 | 25 | 4 | 5 | static | 2 | 10 | 36 of 37 |
| 9 | `GetMSESimilarity` | 1088 | 25 | 4 | 5 | static | 2 | 37 | 35 of 37 |
| 10 | `GetPASimilarity` | 1426 | 24 | 4 | 4 | static | 2 | 10 | 31 of 37 |
| 11 | `GetPDCSimilarity` | 1548 | 22 | 4 | 4 | static | 2 | 10 | 32 of 34 |
| 12 | `SimilarityImage` | 4712 | - | 4 | 7 | public | 2 | 10 | 27 of 47 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `GetSSIMSimularity` (line 2076)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/compare.c): simplify GetSSIMSimularity`

#### Step 2: `GetDPCSimilarity` (line 452)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 3 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/compare.c): simplify GetDPCSimilarity`

#### Step 3: `GetNCCSimilarity` (line 1229)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 3 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/compare.c): simplify GetNCCSimilarity`

#### Step 4: `GetPHASESimilarity` (line 1698)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1788, `le_to_lt`: `if ((GetPixelReadMask(image,pm) <= (QuantumRange/2)) ||`
  - base line 1897, `sub_to_add`: `pearson=(fabs(image_sum[j]-reconstruct_sum[j]) < MagickEpsilon) ?`
- 8 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/compare.c): simplify GetPHASESimilarity`

### Wave 2

#### Step 5: `GetMEPPSimilarity` (line 933)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- 6 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/compare.c): simplify GetMEPPSimilarity`

#### Step 6: `GetAESimilarity` (line 307)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/compare.c): simplify GetAESimilarity`

#### Step 7: `GetFUZZSimilarity` (line 648)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 712, `le_to_lt`: `if ((GetPixelReadMask(image,p) <= (QuantumRange/2)) ||`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/compare.c): simplify GetFUZZSimilarity`

#### Step 8: `GetMAESimilarity` (line 791)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/compare.c): simplify GetMAESimilarity`

### Wave 3

#### Step 9: `GetMSESimilarity` (line 1088)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1152, `le_to_lt`: `(GetPixelReadMask(reconstruct_image,q) <= (QuantumRange/2)))`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/compare.c): simplify GetMSESimilarity`

#### Step 10: `GetPASimilarity` (line 1426)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- 6 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/compare.c): simplify GetPASimilarity`

#### Step 11: `GetPDCSimilarity` (line 1548)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/compare.c): simplify GetPDCSimilarity`

#### Step 12: `SimilarityImage` (line 4712)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 10 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4757, `remove_void_call`: `SetGeometry(reconstruct,offset);`
  - base line 4814, `lt_to_le`: `(image->rows < reconstruct->rows))`
  - base line 4883, `le_to_gt`: `if (similarity <= channel_info.similarity)`
  - base line 4883, `le_to_lt`: `if (similarity <= channel_info.similarity)`
  - base line 4892, `ge_to_gt`: `if (similarity >= channel_info.similarity)`
  - base line 4917, `remove_void_call`: `SetPixelChannel(similarity_image,channel,ClampToQuantum((double)`
  - and 4 more (`tools/oracle/mutate.py --function SimilarityImage`)
- 10 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/compare.c): simplify SimilarityImage`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/compare.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/compare.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/compare.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-24, Mac, overnight)

**Result: Code Health 1.46 → 1.77, plateau** (`cs-mcp` 1.5.12). Branch
`campaign/M02-compare`, stacked on `campaign/M01-magnify` (PR #7) for Recipes T, M and B.
One playbook commit (**Recipe K**, Extract Accumulating Block with Out-Parameters) and 23
refactor commits, one recipe and one function each, all oracle-verified.

**What the file is.** Lines 2889-4569 (`#if MAGICKCORE_HDRI_SUPPORT &&
MAGICKCORE_FFTW_DELEGATE`) are not compiled here: the `SIM*` helpers and
`DPCSimilarityImage`, `NCCSimilarityImage`, `PhaseSimilarityImage`, `MSESimilarityImage`,
`PSNRSimilarityImage`, `RMSESimilarityImage`. They carry Code Duplication, Deep Nesting,
Bumpy Road, Excess Arguments and one Complex Conditional (`SimilarityImage`'s FFTW
branch), so none of those categories can leave the file; the score moves function by
function.

| Steps | Functions | Recipe | Score |
| --- | --- | --- | --- |
| per-pixel channel loop of the parallel row loop into `Accumulate*Pixel` | `GetAESimilarity`, `GetMAESimilarity`, `GetMSESimilarity`, `GetPASimilarity`, `GetPDCSimilarity` | E | 1.46 → 1.55 |
| the same, with two or three of the iteration's own accumulators | `GetFUZZSimilarity`, `GetMEPPSimilarity`, `GetDPCSimilarity` | K | → 1.61 |
| NCC's block is DPC's, renamed: one `AccumulateCorrelationPixel` | `GetNCCSimilarity` | D | → 1.63 |
| missing-row tests as predicates | `CompareImages`, `GetPHASESimilarity` | P | → 1.69 |
| hash-difference loop, running sum in and out | `GetPHASHSimilarity` | E | → 1.72 |
| the 15-case metric dispatch into `GetSimilarityForMetric`, then its copies | `GetImageDistortion` (E), `GetImageDistortions`, `GetSimilarityMetric` (D) | E, D | → 1.74 |
| `image`, `reconstruct_image`, `p`, `q` bundled as `const ComparePixels` | the eight `Accumulate*Pixel` helpers | B | → 1.77 |

Every step: build without warnings, `refactor_guard.py` OK, `--calls` OK or the extract
or deduplicate fingerprint, the oracle on the function's cases (0 diverged), the CodeScene
gate; every moved block checked by script to be identical apart from indentation (and the
`(*v)` rewrites Recipe K names). Ten functions leave Deep Nesting; `GetImageDistortion`,
`GetImageDistortions` and `GetSimilarityMetric` leave Large Method; `CompareImages` leaves
Complex Conditional.

**Whole catalogue** at the branch tip against `8a4c272f1`: 15,884 cases, 1 diverged,
`gaps/c5651ffad8` (the known log-map order tie).

**Tried and reverted.** `CompareImages`' three colour lookups into one function with
three out-parameters (Recipe O): flat, and a 5-argument helper with no next step.
`GetPHASESimilarity`'s Pearson step into a function (6 values in, 1 out): flat; the
function stays blocked by its five-array channel loop. A trait predicate across all
compiled loops (probe): +0.02 for some twenty commits, not taken.

**Why it plateaus at 1.77.**

- `GetSSIMSimularity` (nesting 6, cc 35): its kernel-window block writes five moment
  arrays, over Recipe K's limit of four; its SSIM step reads all five and two constants
  (ten arguments).
- `GetPHASESimilarity` (cc 34): its channel loop writes five sums, the same limit.
- `CompareImages` (cc 24): the pixel loop needs ten arguments; the colour lookups gained
  nothing.
- The `Accumulate*Pixel` helpers are reported as duplicates of each other: they differ in
  the metric (fuzz test, `fabs`, square, maximum), more than Recipe D allows.
- `IsImagesEqual` has **no oracle case** (0 of 23 mutants reached): skipped. At the
  guard-verified level every extraction's `--calls` WARN would stop the commit. A case
  that calls it (it is public API, used by `magick compare -metric` paths only through
  other functions) would release it.
- `SimilarityImage` (Brain Method, the file's only one; public, 256 lines): about fifty
  of its lines are the uncompiled FFTW branch, its pixel switch sits in the parallel
  loop next to a `critical` block that stays, and its channel loop would need five
  arguments for a gain of under 0.02 (probe).
- The FFTW region, as above.
