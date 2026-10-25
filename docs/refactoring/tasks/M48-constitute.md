# M48 - refactor `MagickCore/constitute.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **3.33** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1619 lines |
| Functions with findings | 9 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/constitute.c
```

The score must read 3.33. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 6 |
| Deep, Nested Complexity | 3 |
| Overall Code Complexity | file |
| Complex Method | 6 |
| Complex Conditional | 3 |
| Code Duplication | 2 |
| Large Method | 5 |
| Excess Number of Function Arguments | 1 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `ReadImage` | 608 | 79 | 6 | 16 | public | - | 14955 | 12 of 20 |
| 2 | `WriteImage` | 1241 | 47 | 4 | 10 | public | - | 13074 | 5 of 9 |
| 3 | `SyncResolutionFromProperties` | 525 | 18 | - | 4 | static | - | 14911 | 0 of 4 |
| 4 | `WriteImages` | 1517 | 17 | - | 4 | public | 1 | 12229 | 3 of 8 |
| 5 | `ConstituteImage` | 166 | 30 | - | - | public | - | 1 | 0 of 1 |
| 6 | `ReadImages` | 1016 | - | 4 | - | public | - | 12610 | - |
| 7 | `GetImplicitDataImageType` | 1116 | 14 | - | - | static | - | 32 | - |
| 8 | `PingImages` | 357 | - | - | 2 | public | - | 21 | 1 of 1 |
| 9 | `SyncOrientationFromProperties` | 497 | - | - | 2 | static | - | 14911 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `ReadImage` (line 608)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 16 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 8 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 662, `remove_void_call`: `InheritException(exception,sans_exception);`
  - base line 693, `eq_to_ne`: `if (status == MagickFalse)`
  - base line 852, `remove_void_call`: `GetPathComponent(magick_filename,MagickPath,magick_path);`
  - base line 857, `ne_to_eq`: `if (IsBlobTemporary(image) != MagickFalse)`
  - base line 867, `remove_void_call`: `SyncOrientationFromProperties(next,&constitute_info,exception);`
  - base line 897, `eq_to_ne`: `(read_info->stream == (StreamHandler) NULL))`
  - and 2 more (`tools/oracle/mutate.py --function ReadImage`)

Commit message: `refactor(MagickCore/constitute.c): simplify ReadImage`

#### Step 2: `WriteImage` (line 1241)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 10 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1313, `remove_void_call`: `DisassociateImageStream(image);`
  - base line 1315, `ne_to_eq`: `if ((IsStringTrue(option) != MagickFalse) &&`
  - base line 1402, `eq_to_ne`: `if (sans_exception->severity == PolicyError)`
  - base line 1418, `remove_void_call`: `GetPathComponent(image->filename,ExtensionPath,extension);`

Commit message: `refactor(MagickCore/constitute.c): simplify WriteImage`

#### Step 3: `SyncResolutionFromProperties` (line 525)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 550, `eq_to_ne`: `(resolution_y == (const char *) NULL) &&`
  - base line 551, `ne_to_eq`: `(constitute_info->sync_from_tiff != MagickFalse))`
  - base line 555, `ne_to_eq`: `if ((resolution_x != (const char *) NULL) &&`
  - base line 577, `ne_to_eq`: `if (strchr(resolution_x,',') != (char *) NULL)`

Commit message: `refactor(MagickCore/constitute.c): simplify SyncResolutionFromProperties`

#### Step 4: `WriteImages` (line 1517)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1599, `ne_to_eq`: `if (write_info->blob != (void *) NULL)`
  - base line 1600, `remove_void_call`: `SyncBlobStream(write_info,p,images);`
  - base line 1601, `ne_to_eq`: `if (number_images != 1)`
  - base line 1605, `ne_to_eq`: `if (number_images != 1)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/constitute.c): simplify WriteImages`

### Wave 2

#### Step 5: `ConstituteImage` (line 166)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- Only 1 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/constitute.c): simplify ConstituteImage`

#### Step 6: `ReadImages` (line 1016)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/constitute.c): simplify ReadImages`

#### Step 7: `GetImplicitDataImageType` (line 1116)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/constitute.c): simplify GetImplicitDataImageType`

#### Step 8: `PingImages` (line 357)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/constitute.c): simplify PingImages`

### Wave 3

#### Step 9: `SyncOrientationFromProperties` (line 497)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.

Commit message: `refactor(MagickCore/constitute.c): simplify SyncOrientationFromProperties`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/constitute.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/constitute.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/constitute.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
