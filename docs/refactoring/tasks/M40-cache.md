# M40 - refactor `MagickCore/cache.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.45** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 6109 lines |
| Functions with findings | 67 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/cache.c
```

The score must read 1.45. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 1 |
| Bumpy Road Ahead | 13 |
| Deep, Nested Complexity | 6 |
| Overall Code Complexity | file |
| Complex Method | 21 |
| Complex Conditional | 13 |
| Code Duplication | 43 |
| Large Method | 11 |
| Excess Number of Function Arguments | 16 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `OpenPixelCache` | 3751 | 74 | 5 | 15 | static | - | 15011 | 7 of 13 |
| 2 | `ClonePixelCacheRepository` | 725 | 53 | 5 | 5 | static | 2 | 8449 | 2 of 6 |
| 3 | `GetImagePixelCache` | 1766 | 20 | 4 | 4 | static | - | 15011 | 4 of 8 |
| 4 | `GetVirtualPixelCacheNexus` | 2835 | - | 5 | 6 | public | - | 14288 | 18 of 26 |
| 5 | `ReadPixelCachePixels` | 4754 | 28 | - | 3 | static | - | 6544 | 5 of 7 |
| 6 | `WritePixelCacheMetacontent` | 5789 | 26 | - | 3 | static | - | 20 | - |
| 7 | `SetPixelCacheNexusPixels` | 5232 | 31 | - | 2 | static | - | 14746 | 2 of 5 |
| 8 | `ClipPixelCacheNexus` | 418 | 16 | 5 | - | static | - | 41 | 1 of 2 |
| 9 | `ReadPixelCacheMetacontent` | 4576 | 25 | - | 3 | static | - | 18 | - |
| 10 | `WritePixelCachePixels` | 5968 | 24 | - | 3 | static | - | 6812 | 2 of 3 |
| 11 | `ClonePixelCacheOnDisk` | 611 | 20 | - | 3 | static | - | 22 | 1 of 1 |
| 12 | `MaskPixelCacheNexus` | 3521 | 13 | 4 | - | static | - | 11 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `OpenPixelCache` (line 3751)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 15 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3782, `lt_to_le`: `if (cache_anonymous_memory < 0)`
  - base line 3792, `eq_to_ne`: `if (value == (char *) NULL)`
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/cache.c): simplify OpenPixelCache`

#### Step 2: `ClonePixelCacheRepository` (line 725)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 756, `eq_to_ne`: `(cache_info->columns == clone_info->columns) &&`
  - base line 757, `eq_to_ne`: `(cache_info->rows == clone_info->rows) &&`
  - base line 775, `mul_to_div`: `cache_info->columns*cache_info->rows*`
  - base line 867, `ne_to_eq`: `if ((cache_info->metacontent_extent != 0) &&`

Commit message: `refactor(MagickCore/cache.c): simplify ClonePixelCacheRepository`

#### Step 3: `GetImagePixelCache` (line 1766)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1821, `gt_to_ge`: `if ((cache_info->reference_count > 1) || (cache_info->mode == ReadMode))`
  - base line 1824, `gt_to_ge`: `if ((cache_info->reference_count > 1) || (cache_info->mode == ReadMode))`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/cache.c): simplify GetImagePixelCache`

#### Step 4: `GetVirtualPixelCacheNexus` (line 2835)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- The oracle missed 6 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2968, `remove_void_call`: `SetPixelChannel(image,(PixelChannel) i,(Quantum) 0,virtual_pixel);`
  - base line 2983, `remove_void_call`: `SetPixelChannel(image,(PixelChannel) i,(Quantum) 0,virtual_pixel);`
  - base line 2984, `remove_void_call`: `SetPixelAlpha(image,TransparentAlpha,virtual_pixel);`
  - base line 3003, `remove_void_call`: `SetPixelBlack(image,ClampToQuantum(image->background_color.black),`
  - base line 3185, `ne_to_eq`: `if ((s != (void *) NULL) && (r != (const void *) NULL))`
  - base line 3206, `mul_to_div`: `s+=(ptrdiff_t) (length*cache_info->metacontent_extent);`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/cache.c): simplify GetVirtualPixelCacheNexus`

### Wave 2

#### Step 5: `ReadPixelCachePixels` (line 4754)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4851, `ne_to_eq`: `if (IsFileDescriptorLimitExceeded() != MagickFalse)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/cache.c): simplify ReadPixelCachePixels`

#### Step 6: `WritePixelCacheMetacontent` (line 5789)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/cache.c): simplify WritePixelCacheMetacontent`

#### Step 7: `SetPixelCacheNexusPixels` (line 5232)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (9 arguments).
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/cache.c): simplify SetPixelCacheNexusPixels`

#### Step 8: `ClipPixelCacheNexus` (line 418)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 489, `remove_void_call`: `SetPixelAlpha(image,src_alpha,q);`

Commit message: `refactor(MagickCore/cache.c): simplify ClipPixelCacheNexus`

### Wave 3

#### Step 9: `ReadPixelCacheMetacontent` (line 4576)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/cache.c): simplify ReadPixelCacheMetacontent`

#### Step 10: `WritePixelCachePixels` (line 5968)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/cache.c): simplify WritePixelCachePixels`

#### Step 11: `ClonePixelCacheOnDisk` (line 611)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/cache.c): simplify ClonePixelCacheOnDisk`

#### Step 12: `MaskPixelCacheNexus` (line 3521)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/cache.c): simplify MaskPixelCacheNexus`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/cache.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/cache.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/cache.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
