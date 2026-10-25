# M53 - refactor `MagickCore/splay-tree.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **4.32** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1632 lines |
| Functions with findings | 17 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/splay-tree.c
```

The score must read 4.32. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 7 |
| Deep, Nested Complexity | 4 |
| Overall Code Complexity | file |
| Complex Method | 9 |
| Code Duplication | 14 |
| Large Method | 3 |
| Excess Number of Function Arguments | 1 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `ResetSplayTree` | 1376 | 19 | 5 | 3 | public | - | 5 | - |
| 2 | `DestroySplayTree` | 682 | 18 | 5 | 3 | public | - | 15547 | 2 of 5 |
| 3 | `DeleteNodeByValueFromSplayTree` | 486 | 18 | 4 | 3 | public | - | 12 | - |
| 4 | `RemoveNodeByValueFromSplayTree` | 1186 | 16 | 4 | 3 | public | - | 13 | - |
| 5 | `Splay` | 1509 | 20 | - | 3 | static | - | 15527 | 4 of 4 |
| 6 | `AddValueToSplayTree` | 154 | 13 | - | 2 | public | - | 15534 | 5 of 8 |
| 7 | `SplaySplayTree` | 1603 | 9 | - | 2 | static | - | 15534 | 0 of 4 |
| 8 | `DeleteNodeFromSplayTree` | 603 | 14 | - | - | public | - | 12891 | 7 of 9 |
| 9 | `RemoveNodeFromSplayTree` | 1295 | 12 | - | - | public | - | 14 | - |
| 10 | `CompareSplayTreeString` | 412 | - | - | - | public | - | 15497 | - |
| 11 | `GetNextKeyInSplayTree` | 767 | - | - | - | public | - | 11187 | 4 of 4 |
| 12 | `GetNextValueInSplayTree` | 821 | - | - | - | public | - | 111 | 3 of 3 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `ResetSplayTree` (line 1376)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 5 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/splay-tree.c): simplify ResetSplayTree`

#### Step 2: `DestroySplayTree` (line 682)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 707, `ne_to_eq`: `if ((splay_tree->relinquish_value != (void *(*)(void *)) NULL) &&`
  - base line 724, `ne_to_eq`: `(active->right->key != (void *) NULL))`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/splay-tree.c): simplify DestroySplayTree`

#### Step 3: `DeleteNodeByValueFromSplayTree` (line 486)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/splay-tree.c): simplify DeleteNodeByValueFromSplayTree`

#### Step 4: `RemoveNodeByValueFromSplayTree` (line 1186)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/splay-tree.c): simplify RemoveNodeByValueFromSplayTree`

### Wave 2

#### Step 5: `Splay` (line 1509)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).

Commit message: `refactor(MagickCore/splay-tree.c): simplify Splay`

#### Step 6: `AddValueToSplayTree` (line 154)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 179, `ne_to_eq`: `if ((splay_tree->relinquish_key != (void *(*)(void *)) NULL) &&`
  - base line 180, `ne_to_eq`: `(splay_tree->root->key != (void *) NULL))`
  - base line 203, `lt_to_le`: `if (compare < 0)`

Commit message: `refactor(MagickCore/splay-tree.c): simplify AddValueToSplayTree`

#### Step 7: `SplaySplayTree` (line 1603)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1607, `ne_to_eq`: `if (splay_tree->key != (void *) NULL)`
  - base line 1615, `gt_to_ge`: `compare=(splay_tree->key > key) ? 1 :`
  - base line 1616, `lt_to_ge`: `((splay_tree->key < key) ? -1 : 0);`
  - base line 1624, `remove_void_call`: `BalanceSplayTree(splay_tree);`

Commit message: `refactor(MagickCore/splay-tree.c): simplify SplaySplayTree`

#### Step 8: `DeleteNodeFromSplayTree` (line 603)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 625, `gt_to_le`: `compare=(splay_tree->root->key > key) ? 1 :`
  - base line 626, `lt_to_ge`: `((splay_tree->root->key < key) ? -1 : 0);`

Commit message: `refactor(MagickCore/splay-tree.c): simplify DeleteNodeFromSplayTree`

### Wave 3

#### Step 9: `RemoveNodeFromSplayTree` (line 1295)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/splay-tree.c): simplify RemoveNodeFromSplayTree`

#### Step 10: `CompareSplayTreeString` (line 412)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/splay-tree.c): simplify CompareSplayTreeString`

#### Step 11: `GetNextKeyInSplayTree` (line 767)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/splay-tree.c): simplify GetNextKeyInSplayTree`

#### Step 12: `GetNextValueInSplayTree` (line 821)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/splay-tree.c): simplify GetNextValueInSplayTree`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/splay-tree.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/splay-tree.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/splay-tree.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
