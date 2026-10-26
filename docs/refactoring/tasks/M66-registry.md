# M66 - refactor `MagickCore/registry.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **8.55** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 538 lines |
| Functions with findings | 2 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/registry.c
```

The score must read 8.55. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 1 |
| Overall Code Complexity | file |
| Complex Method | 2 |
| Large Method | 1 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `SetImageRegistry` | 457 | 16 | - | 2 | public | - | 199 | 8 of 12 |
| 2 | `GetImageRegistry` | 190 | 14 | - | - | public | - | 13138 | 3 of 4 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `SetImageRegistry` (line 457)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 481, `eq_to_ne`: `if (LocaleCompare(key,"date:precision") == 0)`
  - base line 482, `remove_void_call`: `SetMagickDatePrecision((size_t) StringToInteger(string));`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/registry.c): simplify SetImageRegistry`

#### Step 2: `GetImageRegistry` (line 190)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/registry.c): simplify GetImageRegistry`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/registry.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/registry.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/registry.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, night)

**Result: Code Health 8.55 → 8.79** (`cs-mcp` 1.5.12). Branch `campaign/M66-registry`,
stacked on `campaign/M65-magic`. Two E commits:
- `InstantiateRegistry` (SetImageRegistry, not ready, guard): the lazy creation of
  the splay tree, test included.
- `GetRegistryValueAsString` (GetImageRegistry, careful, oracle): the string form of an
  entry, value-returning.

**What is left, at 8.79.** SetImageRegistry (cc 13) validates image and image-info
values inside its switch with `ThrowMagickException` and returns, so those cases stay.
GetImageRegistry is at cc 10.
