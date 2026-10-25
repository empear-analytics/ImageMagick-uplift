# M16 - refactor `MagickCore/channel.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **3.28** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1395 lines |
| Functions with findings | 6 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/channel.c
```

The score must read 3.28. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 5 |
| Deep, Nested Complexity | 4 |
| Overall Code Complexity | file |
| Complex Method | 6 |
| Large Method | 4 |
| Excess Number of Function Arguments | 2 |
| Overall Function Size | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `SetImageAlphaChannel` | 994 | 89 | 5 | 11 | public | 6 | 1343 | 82 of 92 |
| 2 | `ChannelFxImage` | 196 | 51 | 5 | 7 | public | - | 60 | 32 of 32 |
| 3 | `CombineImages` | 498 | 36 | 4 | 5 | public | 2 | 27 | 33 of 33 |
| 4 | `SeparateImage` | 729 | 19 | 4 | 3 | public | 2 | 82 | 19 of 19 |
| 5 | `ChannelImage` | 116 | 13 | - | 2 | static | 1 | 56 | 14 of 14 |
| 6 | `FlattenPixelInfo` | 933 | 10 | - | - | static | - | 3 | 14 of 14 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `SetImageAlphaChannel` (line 994)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 11 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 6 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 9 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1015, `ne_to_eq`: `if ((image->alpha_trait & BlendPixelTrait) != 0)`
  - base line 1134, `eq_to_ne`: `if ((image->alpha_trait & BlendPixelTrait) == 0)`
  - base line 1202, `eq_to_ne`: `if ((image->alpha_trait & BlendPixelTrait) == 0)`
  - base line 1236, `lt_to_ge`: `for (y=0; y < (ssize_t) image->rows; y++)`
  - base line 1236, `lt_to_le`: `for (y=0; y < (ssize_t) image->rows; y++)`
  - base line 1244, `eq_to_ne`: `if ((status == MagickFalse) || (opaque == MagickFalse))`
  - and 3 more (`tools/oracle/mutate.py --function SetImageAlphaChannel`)
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/channel.c): simplify SetImageAlphaChannel`

#### Step 2: `ChannelFxImage` (line 196)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/channel.c): simplify ChannelFxImage`

#### Step 3: `CombineImages` (line 498)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/channel.c): simplify CombineImages`

#### Step 4: `SeparateImage` (line 729)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/channel.c): simplify SeparateImage`

### Wave 2

#### Step 5: `ChannelImage` (line 116)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (7 arguments).
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.

Commit message: `refactor(MagickCore/channel.c): simplify ChannelImage`

#### Step 6: `FlattenPixelInfo` (line 933)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- Only 3 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/channel.c): simplify FlattenPixelInfo`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/channel.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/channel.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/channel.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
