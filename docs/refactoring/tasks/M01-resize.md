# M01 - refactor `MagickCore/resize.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.39** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 4705 lines |
| Functions with findings | 25 |
| Measured with | `cs-mcp` 1.5.12, `cs` not installed |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/resize.c
```

The score must read 1.39. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 3 |
| Bumpy Road Ahead | 11 |
| Deep, Nested Complexity | 6 |
| Overall Code Complexity | file |
| Complex Method | 17 |
| Complex Conditional | 17 |
| Large Method | 13 |
| Excess Number of Function Arguments | 12 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `ScaleImage` | 4114 | 98 | 7 | 23 | public | - | 59 | 182 of 212 |
| 2 | `AcquireResizeFilter` | 804 | 69 | - | 11 | public | 1 | 1654 | 65 of 75 |
| 3 | `MagnifyImage` | 2886 | 35 | 4 | 5 | public | 2 | 49 | 45 of 47 |
| 4 | `SampleImage` | 3925 | 26 | 4 | 5 | public | 1 | 112 | 33 of 35 |
| 5 | `HorizontalFilter` | 3337 | - | 5 | 5 | static | 2 | 896 | 85 of 89 |
| 6 | `VerticalFilter` | 3553 | - | 5 | 5 | static | 2 | 896 | 74 of 80 |
| 7 | `InterpolativeResizeImage` | 1750 | - | 4 | 3 | public | 2 | 59 | 24 of 28 |
| 8 | `ThumbnailImage` | 4599 | 19 | - | 3 | public | - | 132 | 22 of 31 |
| 9 | `Scale3X` | 2797 | 24 | - | 2 | static | - | 4 | 20 of 21 |
| 10 | `Fish2X` | 2445 | 31 | - | - | static | - | 4 | 15 of 19 |
| 11 | `CubicSpline` | 250 | 12 | - | 2 | static | - | 13 | 38 of 47 |
| 12 | `Epbx2X` | 2670 | 22 | - | - | static | - | 4 | 7 of 8 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `ScaleImage` (line 4114)

- **Recipe G (guard clauses)** - nesting is 7, target is under 4.
- **Recipe E (extract function)** - 23 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 11 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4249, `le_to_lt`: `if (GetPixelWriteMask(image,p) <= (QuantumRange/2))`
  - base line 4278, `lt_to_le`: `(number_rows < (ssize_t) image->rows))`
  - base line 4292, `le_to_lt`: `if (GetPixelWriteMask(image,p) <= (QuantumRange/2))`
  - base line 4314, `post_inc_to_post_dec`: `number_rows++;`
  - base line 4324, `lt_to_le`: `if ((next_row != MagickFalse) && (number_rows < (ssize_t) image->rows))`
  - base line 4338, `le_to_lt`: `if (GetPixelWriteMask(image,p) <= (QuantumRange/2))`
  - and 5 more (`tools/oracle/mutate.py --function ScaleImage`)
- 19 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resize.c): simplify ScaleImage`

#### Step 2: `AcquireResizeFilter` (line 804)

- **Recipe E (extract function)** - 11 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1091, `eq_to_ne`: `(resize_filter->window == Gaussian) ) {`
  - base line 1149, `div_to_mul`: `resize_filter->blur*=floor(resize_filter->support)/`
- 8 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resize.c): simplify AcquireResizeFilter`

#### Step 3: `MagnifyImage` (line 2886)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resize.c): simplify MagnifyImage`

#### Step 4: `SampleImage` (line 3925)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4031, `le_to_lt`: `if (GetPixelWriteMask(sample_image,q) <= (QuantumRange/2))`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resize.c): simplify SampleImage`

### Wave 2

#### Step 5: `HorizontalFilter` (line 3337)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (7 arguments).
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3487, `le_to_lt`: `(GetPixelWriteMask(resize_image,q) <= (QuantumRange/2)))`
  - base line 3491, `mul_to_div`: `k=y*(contribution[n-1].pixel-contribution[0].pixel+1)+`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resize.c): simplify HorizontalFilter`

#### Step 6: `VerticalFilter` (line 3553)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (7 arguments).
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3702, `le_to_lt`: `(GetPixelWriteMask(resize_image,q) <= (QuantumRange/2)))`
  - base line 3706, `mul_to_div`: `k=(ssize_t) ((contribution[j-start].pixel-contribution[0].pixel)*`
  - base line 3733, `mul_to_div`: `alpha=contribution[j].weight*QuantumScale*(double)`
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resize.c): simplify VerticalFilter`

#### Step 7: `InterpolativeResizeImage` (line 1750)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1847, `eq_to_ne`: `if (status == MagickFalse)`
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resize.c): simplify InterpolativeResizeImage`

#### Step 8: `ThumbnailImage` (line 4599)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 8 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4635, `mul_to_div`: `x_factor=(ssize_t) (image->columns*MagickSafeReciprocal((double)`
  - base line 4638, `gt_to_ge`: `if ((x_factor > 4) && (y_factor > 4))`
  - base line 4638, `gt_to_ge`: `if ((x_factor > 4) && (y_factor > 4))`
  - base line 4647, `gt_to_ge`: `if ((x_factor > 2) && (y_factor > 2))`
  - base line 4669, `remove_void_call`: `ResetImageProfileIterator(thumbnail_image);`
  - base line 4672, `ne_to_eq`: `if ((LocaleCompare(name,"icc") != 0) && (LocaleCompare(name,"icm") != 0))`
  - and 2 more (`tools/oracle/mutate.py --function ThumbnailImage`)
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resize.c): simplify ThumbnailImage`

### Wave 3

#### Step 9: `Scale3X` (line 2797)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Only 4 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resize.c): simplify Scale3X`

#### Step 10: `Fish2X` (line 2445)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Only 4 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2495, `gt_to_ge`: `CopyPixels(pixels,(ssize_t) (intensities[0] > intensities[1] ? 0 : 1),result,`
  - base line 2530, `gt_to_ge`: `if (ae && (!bd || intensities[1] > intensities[0]))`
  - base line 2535, `gt_to_ge`: `if (bd && (!ae || intensities[0] > intensities[1]))`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resize.c): simplify Fish2X`

#### Step 11: `CubicSpline` (line 250)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- 9 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resize.c): simplify CubicSpline`

#### Step 12: `Epbx2X` (line 2670)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Only 4 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/resize.c): simplify Epbx2X`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/resize.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/resize.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/resize.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-24)

**Result: Code Health 1.39 → 2.45, plateau** (`cs-mcp` 1.5.12 throughout, the version in
*Measured with*; `cs` not installed). 43 refactor commits and one playbook commit, in two
sessions on two machines:

| Machine | Commits | Score | Oracle catalogue (`selfcheck`) |
| --- | --- | --- | --- |
| WSL2 on the Windows desktop | 33, `56eff1c2a`..`4507d52f6` (PR #5, merged) | 1.39 → 2.35 | 15,878 cases, 0 nondeterministic |
| MacBook (macOS reference) | 10 + Recipe O, `f5fc6a7a5`..`6b3ee7913` | 2.35 → 2.45 | 15,884 cases, 2 nondeterministic (below) |

Preflight on the Mac: `resize.c` at the original base `20cb5c1246` scored 1.39 and the
branch tip 2.35, both through the MCP server. The readiness gate was run on the Mac with
`gate.py` over all 33 `resize.c` mutation reports in `build-oracle/work` (ERDC's full and
capped runs among them) and this checkout's `verdicts.json`: 85% adjusted for the file;
the per-function verdicts are quoted in each commit. The case map was stale and was
rebuilt on the Mac (`casemap.py`, 15,884 cases, 4,513 functions reached). PR #5 was merged
into `main` mid-session, so from the `Fish2X` commit on, `run --function` compared against
`3e17564a2` (the WSL tip); the whole catalogue was then run against the original
`20cb5c1246`, covering all of M01 end to end.

**Selfcheck deviation.** `matrix/f2913328b4` and `matrix/e1095950b7` (`-hough-lines` with
the matrix forced onto disk by `-limit memory 0 -limit map 0`) leave a `magick-XXXX`
temporary file whose random name lands in the compared outputs, so they can never match.
Neither executes any `resize.c` function (case map), so no per-function run includes
them. They are a catalogue defect in `cases.py` (`_matrix_gap_cases`), not a refactoring
issue, and are reported rather than fixed here.

**Whole catalogue on the Mac**, `oracle.py run --base 20cb5c1246` (all of M01 against the
original base): 15,884 cases, **3 diverged, none in `resize.c`**; no diverging case executes
any `resize.c` function.

- `matrix/f2913328b4`, `matrix/e1095950b7`: the two nondeterministic cases above (one of them
  timed out under load); both passed when rerun alone.
- `gaps/c5651ffad8` (`driver logcfg: eight log maps in one file`): stdout differs every time,
  and it is stable against itself. It diverges just the same when the candidate is built
  from the base's own `resize.c` (byte-identical source), so the cause is the candidate
  build directory (configured 2026-10-05; the bases were built today, and the same
  `imdriver.c` compiles to a different object there), not this branch. The case prints the
  order of eight log maps that tie in `LogInfoCompare`, which `verdicts.json` already
  records as a qsort tie that differs between builds.

#### Per function

| Function | Recipes | Code Health over its steps | What the review shows |
| --- | --- | --- | --- |
| `ScaleImage` (WSL) | E ×8, P ×2 | 1.39 → 1.44 | nesting 7 → 5, cc 98 → 31, bumps 23 → 5; left Complex Conditional |
| `ScaleScanlineX` (WSL, split from `ScaleImage`) | E | flat | cc 18 → 15 |
| `AcquireResizeFilter` (WSL) | E ×3, P ×3 | 1.44 → 1.45 | cc 69 → 59, bumps 11 → 10; left Complex Conditional |
| `AcquireResizeFilter` (Mac) | E | flat 2.35 | cc 59 → 42, 331 → 289 lines: the `filter:verbose` report moved to `ReportResizeFilter`, still inside its `omp single` block |
| `ReportResizeFilter` (Mac) | E, D | flat 2.35 | cc 18 → under 9 (left Complex Method): both type mappings through `GetWeightingFilterType` |
| `MagnifyImage` (WSL, Mac) | E (WSL), O (Mac) | 1.45 → 1.46, then flat | WSL: left Deep Nesting. Mac: cc 33 → 18, 212 → 123 lines, bumps 4 → 3 (method `switch` to `SelectMagnifyMethod`, Recipe O) |
| `SampleImage` (WSL) | E | 1.46 → 1.49 | left Deep Nesting, cc 26 → 22 |
| `HorizontalFilter`, `VerticalFilter` (WSL) | E ×2 each | 1.49 → 1.68 | the per-pixel kernels split out (`Filter*Pixel`) |
| `ThumbnailImage` (WSL) | E | flat | `SetThumbnailProperties` split out |
| `Scale3X` (WSL, Mac) | P (WSL), G (Mac) | 1.68 → 1.78, then flat | Mac: left Bumpy Road (2 bumps) |
| `Fish2X` (WSL, Mac) | P (WSL), P (Mac) | 1.78 → 1.93; **2.35 → 2.45** | Mac: left Complex Conditional, the file's last; cc 21 → 17 |
| `CubicSpline`, `url_encode`, `Epbx2X`, `SelectResizeFilterType`, `Eagle2X` (WSL) | E, P | 1.93 → 2.30 | |
| `ResizeImage` (WSL, Mac) | E (WSL), E and P (Mac) | flat | Mac: 98 → 88 lines, cc 14 → 12, left Complex Conditional |
| `Eagle3X` (WSL, Mac) | P, P | 2.30 → 2.35, then flat | Mac: cc 13 → 9, one short of leaving Complex Method |
| `Eagle3XB` (Mac) | D | flat | left Complex Method (cc 9 → 5) |

The file also left *Lines of Code in a Single File* (indication 2) on the Mac's last steps.

#### Why it plateaus at 2.45

Probes on a scratch copy (functions emptied or smells removed, never committed) showed the
score moves only when a whole category leaves the file. What is left, and why no legal
recipe removes it:

- **Brain Method** and **Deep, Nested Complexity** (+0.26 and about +0.5 if gone).
  `InterpolativeResizeImage`'s depth comes from its row and channel loops inside a
  `#pragma omp parallel for`, and both loops write the shared `status`: the playbook's
  OpenMP rule forbids extracting them, and shortening the rest (probe: 108 → 93 lines)
  leaves it a Brain Method. `ScaleImage`'s Y-scaling branch writes six outer variables and
  is where the gate's unmatched survivors sit (row and span bookkeeping, write masks); so
  is all of `ScaleScanlineX`.
- **Code Duplication** (+0.20 if a whole clone group goes). `Filter{Horizontal,Vertical}Pixel`
  and `{Horizontal,Vertical}Filter` differ in how the source index `k` is computed, an
  expression rather than a single value, so Recipe D does not apply; the four
  `IsScale3X*Edge` predicates differ in three indices each, and inlining them back scores
  lower (2.30).
- **Overall Code Complexity**: the mean cc; a probe needed about 150 more one-line
  functions to clear it.
- **Bumpy Road Ahead**, **Complex Method**, **Large Method** (13, 20 and 11 functions):
  the rest of each is OpenMP-bound (`SampleImage`, the two filters and their kernels,
  `MagnifyImage`'s loop), on survivor lines of *careful* functions (`AcquireResizeFilter`'s
  option parsing, Gaussian, lobes, Jinc and B,C blocks; `ThumbnailImage`; the scanline
  helpers), or in `LiquidRescaleImage`, which this build does not compile (no LQR delegate).
- **Excess Number of Function Arguments**: public signatures, and pixel kernels whose
  arguments are their context.

**Suggested next steps for the owner.** (1) Recipe S: moving the magnify family
(`MagnifyImage` and its kernels, about 1,180 lines) to its own file scores `resize.c` at
2.87 in a probe and takes its Duplication with it. (2) Cases that kill the unmatched
survivors in `AcquireResizeFilter` and `ScaleImage` would release their remaining blocks
to Recipes E and O. (3) The two nondeterministic matrix cases above.

**Possible bug, reported not fixed:** in `ScaleImage`, a failed read inside the Y-scaling
`while` loop sets `status` and `break`s out of the `while` only; the row then continues
with a second read attempt and an export before the outer loop stops.
