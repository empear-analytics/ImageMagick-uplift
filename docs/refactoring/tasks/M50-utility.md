# M50 - refactor `MagickCore/utility.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **3.26** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2088 lines |
| Functions with findings | 11 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/utility.c
```

The score must read 3.26. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 9 |
| Deep, Nested Complexity | 3 |
| Overall Code Complexity | file |
| Complex Method | 11 |
| Complex Conditional | 4 |
| Large Method | 7 |
| Primitive Obsession | file |
| String Heavy Function Arguments | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `GetPathComponent` | 1262 | 62 | 5 | 9 | public | - | 15883 | 65 of 65 |
| 2 | `ExpandFilenames` | 773 | 53 | 5 | 9 | public | - | 12884 | 51 of 80 |
| 3 | `Base64Decode` | 355 | 24 | 4 | 4 | public | - | 32 | 21 of 21 |
| 4 | `ListFiles` | 1777 | 19 | - | 3 | public | - | 43 | 13 of 16 |
| 5 | `AcquireUniqueSymbolicLink` | 154 | 16 | - | 3 | public | - | 37 | 5 of 16 |
| 6 | `ShredFile` | 2016 | 16 | - | 2 | public | - | 240 | 24 of 24 |
| 7 | `GetPathComponents` | 1481 | 10 | - | 3 | public | - | 3 | 14 of 14 |
| 8 | `ExpandFilename` | 627 | 12 | - | 2 | public | - | 24 | 9 of 9 |
| 9 | `GetPathTemplate` | 1553 | 19 | - | - | public | - | 240 | 11 of 11 |
| 10 | `AppendImageFormat` | 289 | 10 | - | - | public | - | 244 | 13 of 13 |
| 11 | `GetExecutionPath` | 1034 | 15 | - | 4 | public | - | 0 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `GetPathComponent` (line 1262)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/utility.c): simplify GetPathComponent`

#### Step 2: `ExpandFilenames` (line 773)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 20 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 829, `gt_to_ge`: `if (parameters > 0)`
  - base line 834, `lt_to_ge`: `for (j=0; j < parameters; j++)`
  - base line 834, `post_inc_to_post_dec`: `for (j=0; j < parameters; j++)`
  - base line 847, `remove_void_call`: `GetPathComponent(option,MagickPath,magick);`
  - base line 863, `remove_void_call`: `GetPathComponent(option,SubimagePath,subimage);`
  - base line 864, `remove_void_call`: `ExpandFilename(path);`
  - and 14 more (`tools/oracle/mutate.py --function ExpandFilenames`)
- 9 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/utility.c): simplify ExpandFilenames`

#### Step 3: `Base64Decode` (line 355)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/utility.c): simplify Base64Decode`

#### Step 4: `ListFiles` (line 1777)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1839, `ge_to_gt`: `if (*number_entries >= max_entries)`
  - base line 1839, `ge_to_lt`: `if (*number_entries >= max_entries)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/utility.c): simplify ListFiles`

### Wave 2

#### Step 5: `AcquireUniqueSymbolicLink` (line 154)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 11 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 189, `eq_to_ne`: `if (passes == -1)`
  - base line 191, `eq_to_ne`: `if ((passes == 0) && (status != MagickFalse))`
  - base line 191, `ne_to_eq`: `if ((passes == 0) && (status != MagickFalse))`
  - base line 221, `eq_to_ne`: `if (destination_file == -1)`
  - base line 224, `eq_to_ne`: `if (source_file == -1)`
  - base line 231, `gt_to_ge`: `if ((fstat(source_file,&attributes) == 0) && (attributes.st_size > 0))`
  - and 5 more (`tools/oracle/mutate.py --function AcquireUniqueSymbolicLink`)

Commit message: `refactor(MagickCore/utility.c): simplify AcquireUniqueSymbolicLink`

#### Step 6: `ShredFile` (line 2016)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/utility.c): simplify ShredFile`

#### Step 7: `GetPathComponents` (line 1481)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 3 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/utility.c): simplify GetPathComponents`

#### Step 8: `ExpandFilename` (line 627)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/utility.c): simplify ExpandFilename`

### Wave 3

#### Step 9: `GetPathTemplate` (line 1553)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/utility.c): simplify GetPathTemplate`

#### Step 10: `AppendImageFormat` (line 289)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/utility.c): simplify AppendImageFormat`

#### Step 11: `GetExecutionPath` (line 1034)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/utility.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/utility.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/utility.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
