# M59 - refactor `MagickCore/linked-list.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **7.59** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1041 lines |
| Functions with findings | 8 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/linked-list.c
```

The score must read 7.59. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 3 |
| Overall Code Complexity | file |
| Complex Method | 2 |
| Code Duplication | 7 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `InsertValueInLinkedList` | 469 | 13 | - | 4 | public | - | 5 | - |
| 2 | `RemoveElementByValueFromLinkedList` | 778 | 10 | - | 2 | public | - | 5 | - |
| 3 | `RemoveElementFromLinkedList` | 850 | - | - | 2 | public | - | 6 | - |
| 4 | `GetHeadElementInLinkedList` | 260 | - | - | - | public | - | 15883 | - |
| 5 | `GetLastValueInLinkedList` | 290 | - | - | - | public | - | 1482 | 2 of 2 |
| 6 | `GetNumberOfElementsInLinkedList` | 370 | - | - | - | public | - | 15883 | - |
| 7 | `LinkedListToArray` | 688 | - | - | - | public | - | 14 | - |
| 8 | `ResetLinkedListIterator` | 981 | - | - | - | public | - | 15883 | 2 of 2 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `InsertValueInLinkedList` (line 469)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 5 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/linked-list.c): simplify InsertValueInLinkedList`

#### Step 2: `RemoveElementByValueFromLinkedList` (line 778)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 5 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/linked-list.c): simplify RemoveElementByValueFromLinkedList`

#### Step 3: `RemoveElementFromLinkedList` (line 850)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/linked-list.c): simplify RemoveElementFromLinkedList`

#### Step 4: `GetHeadElementInLinkedList` (line 260)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/linked-list.c): simplify GetHeadElementInLinkedList`

### Wave 2

#### Step 5: `GetLastValueInLinkedList` (line 290)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/linked-list.c): simplify GetLastValueInLinkedList`

#### Step 6: `GetNumberOfElementsInLinkedList` (line 370)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/linked-list.c): simplify GetNumberOfElementsInLinkedList`

#### Step 7: `LinkedListToArray` (line 688)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/linked-list.c): simplify LinkedListToArray`

#### Step 8: `ResetLinkedListIterator` (line 981)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/linked-list.c): simplify ResetLinkedListIterator`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/linked-list.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/linked-list.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/linked-list.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
