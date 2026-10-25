# M75 - refactor `MagickCore/prepress.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **9.49** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 155 lines |
| Functions with findings | 1 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/prepress.c
```

The score must read 9.49. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 1 |
| Complex Method | 1 |
| Large Method | 1 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `GetImageTotalInkDensity` | 86 | 10 | - | 2 | public | 2 | 9 | 12 of 17 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `GetImageTotalInkDensity` (line 86)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- Only 9 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 139, `gt_to_ge`: `if (density > total_ink_density)`
  - base line 144, `gt_to_ge`: `if (density > total_ink_density)`
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/prepress.c): simplify GetImageTotalInkDensity`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/prepress.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/prepress.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/prepress.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-26, Mac, night)

**Result: Code Health 9.49, unchanged. No legal step was found.**
GetImageTotalInkDensity's only nested block is the per-pixel maximum. It contains an
`#if defined(MAGICKCORE_OPENMP_SUPPORT)` `#pragma omp critical`, and OpenMP is off in
this build, so the block cannot move (playbook: OpenMP regions and preprocessor
branches).
