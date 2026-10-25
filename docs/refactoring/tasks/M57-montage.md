# M57 - refactor `MagickCore/montage.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **6.01** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 899 lines |
| Functions with findings | 2 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/montage.c
```

The score must read 6.01. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 1 |
| Deep, Nested Complexity | 1 |
| Overall Code Complexity | file |
| Complex Method | 1 |
| Large Method | 1 |
| Excess Number of Function Arguments | 1 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `MontageImageList` | 321 | 91 | 5 | 18 | public | - | 49 | 29 of 42 |
| 2 | `GetMontageGeometry` | 272 | - | - | - | static | - | 38 | 0 of 1 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `MontageImageList` (line 321)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 18 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 13 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 454, `lt_to_ge`: `for (i=0; i < (ssize_t) number_images; i++)`
  - base line 458, `remove_void_call`: `qsort((void *) image_list,(size_t) number_images,sizeof(*image_list),`
  - base line 528, `ne_to_eq`: `if (montage_info->font != (char *) NULL)`
  - base line 581, `remove_void_call`: `GetMontageGeometry(montage_info->tile,number_images,&x_offset,&y_offset,`
  - base line 599, `gt_to_ge`: `if (x_offset > (ssize_t) bounds.width)`
  - base line 605, `ne_to_eq`: `if (montage_info->tile != (char *) NULL)`
  - and 7 more (`tools/oracle/mutate.py --function MontageImageList`)

Commit message: `refactor(MagickCore/montage.c): simplify MontageImageList`

#### Step 2: `GetMontageGeometry` (line 272)

- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 284, `div_to_mul`: `*tiles_per_row=(size_t) ceil((double) number_images/(*tiles_per_column));`

Commit message: `refactor(MagickCore/montage.c): simplify GetMontageGeometry`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/montage.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/montage.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/montage.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, night)

**Result: Code Health 6.01 → 6.61** (`cs-mcp` 1.5.12). Branch `campaign/M57-montage`,
stacked on `campaign/M56-histogram`. MontageImageList is *not ready* (43 mutants, 29
killed), so all 11 commits are guard-level Recipe E. The oracle also ran on each one:
49 cases, 0 diverged.

| Helper | What moved | Form |
|---|---|---|
| `SortImageListByScene` | scene-order check and qsort | plain |
| `GetLargestTileExtent` | widen extract_info to the largest thumbnail | value-returning, struct by value |
| `AcquireMontageDrawInfo` | label draw attributes | value-returning |
| `GetMaximumLabelLines` | most label lines | value-returning |
| `GetMontageDirectoryExtent` | directory size | value-returning |
| `AddTileShadow` | drop shadow | value-returning (image) |
| `AddTileBorder` | tile border (6 arguments) | value-returning (image) |
| `AnnotateMontageTitle` | page title (7 arguments) | plain |
| `AddTileFrame` | ornamental frame (6 arguments) | value-returning (image) |
| `ParseMontageFrame` | -frame geometry | value-returning, struct by value |
| `ReadMontageTexture` | -texture image | value-returning |

**What we learned.**
- Passing a struct by value (`extract_info`, `frame_info`, `metrics`) keeps every
  `name.field` exactly as written, and a value-returning helper hands the struct back.
- In this file, helpers with 6–7 arguments still raised the score even though
  CodeScene names each of them under Excess Number of Function Arguments.

**Reverted.** The directory fill loop as `FillMontageDirectory`. It contains a
`ThrowMagickException(…GetMagickModule()…)`, and moving it would change the function
name in the exception's `@ file/function/line` suffix. Throw blocks stay where they are.

**What is left, at 6.61.** MontageImageList still has 367 lines and cc 67.
- The per-page bounding-box loop writes `bounds`, `x_offset`, `y_offset`, `width` and
  `max_height` together.
- The label annotation inside the tile loop would need more than ten arguments.
- The thumbnail loop and the per-page allocation return or throw from inside.
- `GetMontageGeometry` takes 6 arguments.
