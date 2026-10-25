# M54 - refactor `MagickCore/token.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **4.58** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1069 lines |
| Functions with findings | 4 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/token.c
```

The score must read 4.58. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 3 |
| Deep, Nested Complexity | 3 |
| Overall Code Complexity | file |
| Complex Method | 4 |
| Complex Conditional | 2 |
| Large Method | 3 |
| String Heavy Function Arguments | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `GlobExpression_` | 355 | 69 | 5 | 7 | static | - | 226 | 2 of 16 |
| 2 | `GetNextToken` | 176 | 63 | 4 | 9 | public | - | 15883 | 15 of 18 |
| 3 | `Tokenizer` | 914 | 33 | 4 | 4 | public | - | 13 | - |
| 4 | `IsGlob` | 675 | 11 | - | - | public | - | 14906 | 1 of 1 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `GlobExpression_` (line 355)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 14 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 369, `eq_to_ne`: `if (GetUTFCode(pattern) == 0)`
  - base line 376, `eq_to_ne`: `(strchr(pattern,'?') == NULL))`
  - base line 384, `remove_void_call`: `GetPathComponent(pattern,SubimagePath,path);`
  - base line 394, `ne_to_eq`: `if ((ecode == 0) && (pcode != '*') && (pcode != '{'))`
  - base line 424, `eq_to_ne`: `if (GetUTFCode(expression) == 0)`
  - base line 453, `lt_to_ge`: `while (p < q)`
  - and 8 more (`tools/oracle/mutate.py --function GlobExpression_`)

Commit message: `refactor(MagickCore/token.c): simplify GlobExpression_`

#### Step 2: `GetNextToken` (line 176)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 258, `ge_to_gt`: `if ((size_t) (p-start) >= (extent-1))`
  - base line 302, `sub_to_add`: `if ((size_t) (p-start) >= (extent-1))`
  - base line 322, `sub_to_add`: `return((size_t) (p-start+1));`

Commit message: `refactor(MagickCore/token.c): simplify GetNextToken`

#### Step 3: `Tokenizer` (line 914)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/token.c): simplify Tokenizer`

#### Step 4: `IsGlob` (line 675)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/token.c): simplify IsGlob`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/token.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/token.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/token.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
