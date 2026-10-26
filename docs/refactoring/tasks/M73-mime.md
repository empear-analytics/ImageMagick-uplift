# M73 - refactor `MagickCore/mime.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **5.10** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1108 lines |
| Functions with findings | 5 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/mime.c
```

The score must read 5.10. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Brain Method | 1 |
| Bumpy Road Ahead | 3 |
| Deep, Nested Complexity | 2 |
| Overall Code Complexity | file |
| Complex Method | 2 |
| Code Duplication | 2 |
| Large Method | 2 |
| Excess Number of Function Arguments | 1 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `GetMimeInfo` | 228 | 39 | 4 | 11 | public | - | 10 | 47 of 47 |
| 2 | `LoadMimeCache` | 803 | - | 5 | 5 | static | - | 14 | 25 of 32 |
| 3 | `ListMimeInfo` | 712 | 13 | - | 4 | public | - | 1 | 18 of 18 |
| 4 | `GetMimeInfoList` | 444 | - | - | - | public | - | 1 | 11 of 12 |
| 5 | `GetMimeList` | 541 | - | - | - | public | - | 3 | 10 of 12 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `GetMimeInfo` (line 228)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 11 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/mime.c): simplify GetMimeInfo`

#### Step 2: `LoadMimeCache` (line 803)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- 7 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/mime.c): simplify LoadMimeCache`

#### Step 3: `ListMimeInfo` (line 712)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 1 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/mime.c): simplify ListMimeInfo`

#### Step 4: `GetMimeInfoList` (line 444)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 1 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/mime.c): simplify GetMimeInfoList`

### Wave 2

#### Step 5: `GetMimeList` (line 541)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 3 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 582, `remove_void_call`: `qsort((void *) aliases,(size_t) i,sizeof(*aliases),MimeCompare);`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/mime.c): simplify GetMimeList`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/mime.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/mime.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/mime.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-26, Mac, night)

**Result: Code Health 5.10 → 5.87** (`cs-mcp` 1.5.12). Branch `campaign/M73-mime`,
stacked on `campaign/M72-distribute-cache`. Seven oracle-verified commits (every
target function is ready):
- GetMimeInfo: `ReadMimeShortValue` and `ReadMimeLongValue` (E, value-returning), then
  `MatchMimeValue` (D). The byte, short and long cases end with an identical mask test,
  now one value-returning helper. GetMimeInfo goes from cc 39 to 26 and from 155 to 98
  lines.
- LoadMimeCache: `ParseMimeMagic` and `GetIncludePath` (the same name and shape as in
  type.c and delegate.c). LoadMimeCache goes from 161 to 101 lines.
- ListMimeInfo: `PrintMimePathHeader` and `PadMimeType`. ListMimeInfo leaves Bumpy Road
  Ahead. One upstream dead store (`j=0`) and its declaration were removed with the
  move, and the commit says so.

**What is left, at 5.87.**
- GetMimeInfo (5 bumps): the priority and pattern tests `continue` the walk, and the
  string case `break`s from its inner loop.
- LoadMimeCache stays a Brain Method: its attribute chain sets one field per attribute,
  and its include block recurses.
- GetMimeInfoList and GetMimeList are the usual list-to-array pair.
