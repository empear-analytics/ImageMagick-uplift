# M18 - refactor `MagickCore/magick.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **6.96** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1867 lines |
| Functions with findings | 12 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/magick.c
```

The score must read 6.96. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 2 |
| Deep, Nested Complexity | 1 |
| Complex Method | 4 |
| Code Duplication | 8 |
| Large Method | 2 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `ListMagickInfo` | 1168 | 18 | 4 | 2 | public | - | 1 | 0 of 1 |
| 2 | `GetMagickInfo` | 605 | 9 | - | 2 | public | - | 14971 | 7 of 7 |
| 3 | `MagickCoreGenesis` | 1487 | 14 | - | - | public | - | 15883 | 5 of 8 |
| 4 | `GetMagickAdjoin` | 345 | - | - | - | public | - | 13279 | 1 of 1 |
| 5 | `GetMagickBlobSupport` | 375 | - | - | - | public | - | 14305 | - |
| 6 | `GetMagickEndianSupport` | 570 | - | - | - | public | - | 14945 | - |
| 7 | `GetMagickInfoList` | 695 | - | - | - | public | - | 1 | 3 of 4 |
| 8 | `GetMagickRawSupport` | 968 | - | - | - | public | - | 1659 | 1 of 1 |
| 9 | `GetMagickStealth` | 999 | - | - | - | public | - | 1 | - |
| 10 | `IsMagickConflict` | 1132 | - | - | - | public | - | 3545 | - |
| 11 | `MagickSignalHandler` | 1401 | 10 | - | - | static | - | 0 | - |
| 12 | `GetMagickList` | 789 | - | - | - | public | - | 0 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `ListMagickInfo` (line 1168)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 1 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1188, `remove_void_call`: `ClearMagickException(exception);`

Commit message: `refactor(MagickCore/magick.c): simplify ListMagickInfo`

#### Step 2: `GetMagickInfo` (line 605)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/magick.c): simplify GetMagickInfo`

#### Step 3: `MagickCoreGenesis` (line 1487)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1510, `remove_void_call`: `SetClientNameAndPath(path);`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/magick.c): simplify MagickCoreGenesis`

#### Step 4: `GetMagickAdjoin` (line 345)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/magick.c): simplify GetMagickAdjoin`

### Wave 2

#### Step 5: `GetMagickBlobSupport` (line 375)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/magick.c): simplify GetMagickBlobSupport`

#### Step 6: `GetMagickEndianSupport` (line 570)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/magick.c): simplify GetMagickEndianSupport`

#### Step 7: `GetMagickInfoList` (line 695)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 1 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 736, `remove_void_call`: `qsort((void *) formats,(size_t) i,sizeof(*formats),MagickInfoCompare);`

Commit message: `refactor(MagickCore/magick.c): simplify GetMagickInfoList`

#### Step 8: `GetMagickRawSupport` (line 968)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/magick.c): simplify GetMagickRawSupport`

### Wave 3

#### Step 9: `GetMagickStealth` (line 999)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 1 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/magick.c): simplify GetMagickStealth`

#### Step 10: `IsMagickConflict` (line 1132)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/magick.c): simplify IsMagickConflict`

#### Step 11: `MagickSignalHandler` (line 1401)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 12: `GetMagickList` (line 789)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/magick.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/magick.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/magick.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
