# M63 - refactor `MagickCore/random.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **7.68** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 997 lines |
| Functions with findings | 8 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/random.c
```

The score must read 7.68. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 2 |
| Complex Method | 1 |
| Code Duplication | 5 |
| Large Method | 2 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `GenerateEntropicChaos` | 352 | 15 | - | 2 | static | - | 1630 | 39 of 39 |
| 2 | `SetRandomKey` | 882 | - | - | 2 | public | - | 489 | 17 of 20 |
| 3 | `GetRandomKey` | 714 | - | - | - | public | - | 489 | 1 of 1 |
| 4 | `IncrementRandomNonce` | 864 | - | - | - | static | - | 489 | 4 of 4 |
| 5 | `AcquireRandomInfo` | 167 | - | - | - | public | - | 1630 | 16 of 20 |
| 6 | `GetRandomInfoNormalize` | 656 | - | - | - | public | - | 0 | - |
| 7 | `GetRandomInfoSeed` | 684 | - | - | - | public | - | 0 | - |
| 8 | `GetRandomSecretKey` | 747 | - | - | - | public | - | 0 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `GenerateEntropicChaos` (line 352)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/random.c): simplify GenerateEntropicChaos`

#### Step 2: `SetRandomKey` (line 882)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- Public function: its name and signature must not change.
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/random.c): simplify SetRandomKey`

#### Step 3: `GetRandomKey` (line 714)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/random.c): simplify GetRandomKey`

#### Step 4: `IncrementRandomNonce` (line 864)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/random.c): simplify IncrementRandomNonce`

### Wave 2

#### Step 5: `AcquireRandomInfo` (line 167)

- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Public function: its name and signature must not change.
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/random.c): simplify AcquireRandomInfo`

#### Step 6: `GetRandomInfoNormalize` (line 656)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 7: `GetRandomInfoSeed` (line 684)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 8: `GetRandomSecretKey` (line 747)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/random.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/random.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/random.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, night)

**Result: Code Health 7.68 → 8.06** (`cs-mcp` 1.5.12). Branch `campaign/M63-random`,
stacked on `campaign/M62-quantum`. Two oracle-verified E commits (both functions are
ready, 20 of 20 killed):
- `SeedFromSecretKey` (AcquireRandomInfo), 7.68 → 8.00. AcquireRandomInfo leaves
  Large Method.
- `RefillRandomReservoir` (SetRandomKey). It sits right above SetRandomKey, because
  IncrementRandomNonce is defined between the banner and the function. A first
  placement above the banner did not compile; that commit was undone locally before
  anything was pushed.

**Process note.** That failed build was committed briefly: the verify step was piped
through `tail`, so its exit status was lost. The commit was reset at once, and verify
results are now checked explicitly for `VERIFY FAILED`.

**What is left, at 8.06.**
- GenerateEntropicChaos (201 lines) consists of `#if` branches for getentropy,
  sysconf, getrusage, gettimeofday, clock_gettime, times, Windows and the fuzzing
  build. Most of them are not compiled here.
- Code Duplication over five small getters that share the assert prologue.
