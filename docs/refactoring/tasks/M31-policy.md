# M31 - refactor `MagickCore/policy.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **3.56** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1619 lines |
| Functions with findings | 8 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/policy.c
```

The score must read 3.56. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Brain Method | 1 |
| Bumpy Road Ahead | 7 |
| Deep, Nested Complexity | 2 |
| Overall Code Complexity | file |
| Complex Method | 7 |
| Complex Conditional | 4 |
| Large Method | 4 |
| Excess Number of Function Arguments | 1 |
| String Heavy Function Arguments | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `IsRightsAuthorizedByName` | 787 | 46 | 5 | 5 | public | - | 15046 | 50 of 50 |
| 2 | `LoadPolicyCache` | 1104 | - | 6 | 10 | static | - | 15883 | 46 of 46 |
| 3 | `IsPathContainsSymlink` | 610 | 18 | - | 5 | static | - | 3 | 19 of 29 |
| 4 | `GetPolicyInfo` | 284 | 17 | - | 4 | static | - | 15883 | 19 of 19 |
| 5 | `ListPolicyInfo` | 983 | 19 | - | 3 | public | - | 5 | 19 of 19 |
| 6 | `AcquirePolicyCache` | 173 | 10 | - | 2 | static | - | 15883 | 12 of 12 |
| 7 | `IsPathAuthorized` | 695 | 9 | - | - | public | - | 14712 | 8 of 8 |
| 8 | `SetMagickSecurityPolicyValue` | 1525 | 24 | - | 5 | public | - | 0 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `IsRightsAuthorizedByName` (line 787)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/policy.c): simplify IsRightsAuthorizedByName`

#### Step 2: `LoadPolicyCache` (line 1104)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 10 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).

Commit message: `refactor(MagickCore/policy.c): simplify LoadPolicyCache`

#### Step 3: `IsPathContainsSymlink` (line 610)

- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Only 3 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 660, `sub_to_add`: `if ((offset > 0) && (partial[offset-1] != *DirectorySeparator))`
  - base line 689, `ne_to_eq`: `if (IsPolicyPathSeparator(*p) != MagickFalse)`
- 8 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/policy.c): simplify IsPathContainsSymlink`

#### Step 4: `GetPolicyInfo` (line 284)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/policy.c): simplify GetPolicyInfo`

### Wave 2

#### Step 5: `ListPolicyInfo` (line 983)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 5 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/policy.c): simplify ListPolicyInfo`

#### Step 6: `AcquirePolicyCache` (line 173)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/policy.c): simplify AcquirePolicyCache`

#### Step 7: `IsPathAuthorized` (line 695)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/policy.c): simplify IsPathAuthorized`

#### Step 8: `SetMagickSecurityPolicyValue` (line 1525)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/policy.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/policy.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/policy.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
