# M14 - refactor `MagickCore/geometry.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.73** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1832 lines |
| Functions with findings | 11 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/geometry.c
```

The score must read 1.73. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 2 |
| Bumpy Road Ahead | 5 |
| Deep, Nested Complexity | 3 |
| Overall Code Complexity | file |
| Complex Method | 6 |
| Complex Conditional | 15 |
| Code Duplication | 2 |
| Large Method | 4 |
| Excess Number of Function Arguments | 2 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `ParseGeometry` | 922 | 130 | 4 | 17 | public | - | 14705 | 92 of 93 |
| 2 | `ParseMetaGeometry` | 1473 | - | 4 | 10 | public | - | 1587 | 74 of 74 |
| 3 | `GetGeometry` | 202 | - | 4 | 8 | public | - | 12938 | 59 of 59 |
| 4 | `ParseGravityGeometry` | 1313 | 24 | - | 5 | public | - | 579 | 52 of 52 |
| 5 | `GravityAdjustGeometry` | 547 | 25 | - | - | public | - | 1359 | 14 of 14 |
| 6 | `ParsePageGeometry` | 1683 | 11 | - | - | public | - | 337 | 10 of 10 |
| 7 | `ParseAffineGeometry` | 813 | 11 | - | - | public | - | 5 | 6 of 6 |
| 8 | `GetPageGeometry` | 473 | - | - | 2 | public | - | 11939 | 6 of 6 |
| 9 | `IsSceneGeometry` | 669 | 9 | - | - | public | - | 14918 | 10 of 10 |
| 10 | `SetGeometry` | 1791 | - | - | - | public | - | 3917 | 1 of 1 |
| 11 | `SetGeometryInfo` | 1825 | - | - | - | public | - | 14801 | 1 of 1 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `ParseGeometry` (line 922)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 17 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1159, `eq_to_ne`: `if ((*p == ',') || (*p == '/') || (*p == ':'))`

Commit message: `refactor(MagickCore/geometry.c): simplify ParseGeometry`

#### Step 2: `ParseMetaGeometry` (line 1473)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 10 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/geometry.c): simplify ParseMetaGeometry`

#### Step 3: `GetGeometry` (line 202)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 8 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/geometry.c): simplify GetGeometry`

#### Step 4: `ParseGravityGeometry` (line 1313)

- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/geometry.c): simplify ParseGravityGeometry`

### Wave 2

#### Step 5: `GravityAdjustGeometry` (line 547)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/geometry.c): simplify GravityAdjustGeometry`

#### Step 6: `ParsePageGeometry` (line 1683)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/geometry.c): simplify ParsePageGeometry`

#### Step 7: `ParseAffineGeometry` (line 813)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 5 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/geometry.c): simplify ParseAffineGeometry`

#### Step 8: `GetPageGeometry` (line 473)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/geometry.c): simplify GetPageGeometry`

### Wave 3

#### Step 9: `IsSceneGeometry` (line 669)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/geometry.c): simplify IsSceneGeometry`

#### Step 10: `SetGeometry` (line 1791)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/geometry.c): simplify SetGeometry`

#### Step 11: `SetGeometryInfo` (line 1825)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/geometry.c): simplify SetGeometryInfo`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/geometry.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/geometry.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/geometry.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, overnight)

**Result: Code Health 1.73 → 3.38** (`cs-mcp` 1.5.12). Branch `campaign/M14-geometry`,
stacked on `campaign/M13-colormap` (PR #19). 21 refactor commits, one recipe and one
function each:
- 11 Recipe P: 10 oracle-verified, and 1 guard-verified (`ParsePageGeometry`'s one
  unmatched survivor sits on its test);
- 10 Recipe E, all oracle-verified.

The readiness gate, merging every geometry report, puts `ParseGeometry`, `GetGeometry`,
`ParseMetaGeometry` and `ParseGravityGeometry` at 100% and `ParsePageGeometry` at 91%.
`ParseGeometry` alone executes 14,705 cases, so each of its steps took 6–8 minutes of
oracle time.

**One category leaves the file:** Complex Conditional, fifteen sites. Brain Method leaves
`ParseMetaGeometry`. Large Method leaves `ParseGravityGeometry`.

| Steps | Recipe | Score |
| --- | --- | --- |
| `ParseGeometry`: offset prefix and separator (three sites each), rho terminator, sigma separator, sigma parsable, negative height | P | 1.73 → 2.46 |
| `GetGeometry`: width terminator, height separator, height-only; `ParsePageGeometry`; `ParseMetaGeometry` | P | → 2.67 |
| `GetGeometry`'s percent completion; `ParseMetaGeometry`'s '>' and '<' limits, aspect scale factor and '@' area limit | E | → 3.21 |
| `ParseGravityGeometry`'s percent and aspect-ratio regions | E | → 3.32 |
| `ParseGeometry`'s sampling-factor normalization, negative-height shift and percent completion | E | → 3.38 |

**Whole catalogue** at the branch tip against `8a4c272f1`: 15,884 cases, 1 diverged
(`gaps/c5651ffad8`, the known log-map tie).

**What is left, at 3.38.**
- Brain Method: `GetGeometry`. Its flag scan and its width, height, x and y parsers
  each write both `flags` and the cursor `p`.
- Complex and Large Method, Bumpy Road and nesting 4: `ParseGeometry` (cc 84). Its rho,
  sigma, xi, psi and chi parsers each write `flags`, `p`, `q`, `value` and the geometry.
- Excess Arguments: public signatures, and three five-parameter helpers.

**Possible bug, reported, not fixed.** In `ParseMetaGeometry`'s aspect scale (now
`GetAspectScaleFactor`), the width-only branch compares the factor with
`*width/stasis_height`, and the height-only branch with `*height/stasis_width`. Both
cross width with height, where the two-sided branch does not. It may be intended for the
`^` (minimum) flag, but it reads like a swap. It is unchanged here.

**Process note.** `make_task.py` wrote the task file at the branch's start. It reached
the first refactor commit by mistake and was split into its own commit before any push.
