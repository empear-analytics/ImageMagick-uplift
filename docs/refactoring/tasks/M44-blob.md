# M44 - refactor `MagickCore/blob.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.85** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 6757 lines |
| Functions with findings | 68 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/blob.c
```

The score must read 1.85. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 16 |
| Deep, Nested Complexity | 10 |
| Overall Code Complexity | file |
| Complex Method | 27 |
| Complex Conditional | 9 |
| Code Duplication | 50 |
| Large Method | 18 |
| Excess Number of Function Arguments | 1 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `OpenBlob` | 3303 | 73 | 6 | 15 | public | - | 14865 | 10 of 14 |
| 2 | `ReadBlob` | 3872 | 51 | 5 | 4 | public | - | 14308 | 3 of 8 |
| 3 | `WriteBlob` | 5894 | 48 | 5 | 4 | public | - | 13014 | 5 of 10 |
| 4 | `CloseBlob` | 620 | 49 | 4 | 4 | public | - | 15098 | 2 of 5 |
| 5 | `SetBlobExtent` | 5310 | 29 | 4 | 4 | public | - | 154 | 1 of 1 |
| 6 | `CustomStreamToImage` | 806 | 14 | 4 | 4 | public | - | 30 | - |
| 7 | `ImagesToBlob` | 2491 | 14 | 4 | 4 | public | - | 8 | - |
| 8 | `ImageToBlob` | 2089 | 13 | 4 | 4 | public | - | 46 | 1 of 2 |
| 9 | `FileToBlob` | 1449 | 29 | - | 4 | public | - | 215 | 1 of 4 |
| 10 | `ImageToCustomStream` | 2227 | 14 | 4 | 3 | public | - | 18 | - |
| 11 | `ImagesToCustomStream` | 2633 | 14 | 4 | 3 | public | - | 12 | - |
| 12 | `ReadBlobString` | 5005 | 21 | - | 4 | public | - | 122 | 3 of 3 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `OpenBlob` (line 3303)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 15 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3341, `eq_to_ne`: `(*image->filename == '\0'))`
  - base line 3626, `ne_to_eq`: `if (image_info->file != (FILE *) NULL)`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/blob.c): simplify OpenBlob`

#### Step 2: `ReadBlob` (line 3872)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3972, `le_to_lt`: `if (count <= 0)`
  - base line 4025, `ne_to_eq`: `blob_info->eof=gzeof(blob_info->file_info.gzfile) != 0 ? MagickTrue :`
  - base line 4053, `ne_to_eq`: `if ((count != (ssize_t) length) && (status != BZ_OK))`
  - base line 4087, `le_to_gt`: `if (count <= 0)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/blob.c): simplify ReadBlob`

#### Step 3: `WriteBlob` (line 5894)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- The oracle missed 5 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 5974, `ne_to_eq`: `if ((count != (ssize_t) length) &&`
  - base line 5996, `le_to_lt`: `if (count <= 0)`
  - base line 6043, `ne_to_eq`: `if ((count != (ssize_t) length) && (status != Z_OK))`
  - base line 6061, `le_to_lt`: `if (count <= 0)`
  - base line 6094, `add_to_sub`: `extent+=blob_info->quantum+length;`

Commit message: `refactor(MagickCore/blob.c): simplify WriteBlob`

#### Step 4: `CloseBlob` (line 620)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 757, `remove_void_call`: `BZ2_bzclose(blob_info->file_info.bzfile);`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/blob.c): simplify CloseBlob`

### Wave 2

#### Step 5: `SetBlobExtent` (line 5310)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/blob.c): simplify SetBlobExtent`

#### Step 6: `CustomStreamToImage` (line 806)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/blob.c): simplify CustomStreamToImage`

#### Step 7: `ImagesToBlob` (line 2491)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 8 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/blob.c): simplify ImagesToBlob`

#### Step 8: `ImageToBlob` (line 2089)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2148, `remove_void_call`: `SyncBlobStream(blob_info,image,image);`

Commit message: `refactor(MagickCore/blob.c): simplify ImageToBlob`

### Wave 3

#### Step 9: `FileToBlob` (line 1449)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/blob.c): simplify FileToBlob`

#### Step 10: `ImageToCustomStream` (line 2227)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/blob.c): simplify ImageToCustomStream`

#### Step 11: `ImagesToCustomStream` (line 2633)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/blob.c): simplify ImagesToCustomStream`

#### Step 12: `ReadBlobString` (line 5005)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/blob.c): simplify ReadBlobString`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/blob.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/blob.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/blob.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
