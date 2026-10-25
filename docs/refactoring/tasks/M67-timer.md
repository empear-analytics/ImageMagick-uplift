# M67 - refactor `MagickCore/timer.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **8.81** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 665 lines |
| Functions with findings | 7 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/timer.c
```

The score must read 8.81. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Code Duplication | 7 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `ContinueTimer` | 140 | - | - | - | public | - | 22 | - |
| 2 | `DestroyTimerInfo` | 178 | - | - | - | public | - | 3 | - |
| 3 | `GetElapsedTime` | 329 | - | - | - | public | - | 313 | 3 of 3 |
| 4 | `GetTimerInfo` | 414 | - | - | - | public | - | 15883 | 1 of 1 |
| 5 | `GetUserTime` | 450 | - | - | - | public | - | 313 | 0 of 3 |
| 6 | `ResetTimer` | 510 | - | - | - | public | - | 18 | 0 of 1 |
| 7 | `StopTimer` | 614 | - | - | - | static | - | 320 | 2 of 5 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `ContinueTimer` (line 140)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/timer.c): simplify ContinueTimer`

#### Step 2: `DestroyTimerInfo` (line 178)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 3 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/timer.c): simplify DestroyTimerInfo`

#### Step 3: `GetElapsedTime` (line 329)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/timer.c): simplify GetElapsedTime`

#### Step 4: `GetTimerInfo` (line 414)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/timer.c): simplify GetTimerInfo`

### Wave 2

#### Step 5: `GetUserTime` (line 450)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/timer.c): simplify GetUserTime`

#### Step 6: `ResetTimer` (line 510)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/timer.c): simplify ResetTimer`

#### Step 7: `StopTimer` (line 614)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/timer.c): simplify StopTimer`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/timer.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/timer.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/timer.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, night)

**Result: Code Health 8.81, unchanged. No legal step was found.** The only finding is
Code Duplication over seven functions with three to six statements each, such as
GetElapsedTime and GetUserTime (which differ only in `elapsed` versus `user`).
- The shared part is mostly the `assert(time_info…)` prologue, which reports the
  enclosing function on failure, so it cannot move.
- The rest are one-line state tests that are too small to pay for a helper.
