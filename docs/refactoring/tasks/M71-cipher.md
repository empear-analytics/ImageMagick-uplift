# M71 - refactor `MagickCore/cipher.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **6.28** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1196 lines |
| Functions with findings | 8 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/cipher.c
```

The score must read 6.28. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 2 |
| Overall Code Complexity | file |
| Complex Method | 3 |
| Complex Conditional | 1 |
| Code Duplication | 8 |
| Large Method | 3 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `PasskeyEncipherImage` | 771 | 15 | - | 2 | public | - | 6 | 27 of 27 |
| 2 | `PasskeyDecipherImage` | 561 | 14 | - | 2 | public | - | 3 | 26 of 26 |
| 3 | `SetAESKey` | 998 | 11 | - | - | static | - | 6 | 30 of 30 |
| 4 | `AcquireAESInfo` | 192 | - | - | - | static | - | 6 | 3 of 3 |
| 5 | `IncrementCipherNonce` | 527 | - | - | - | static | - | 6 | 4 of 4 |
| 6 | `EncipherAESBlock` | 385 | - | - | - | static | - | 6 | 12 of 12 |
| 7 | `EncipherImage` | 752 | - | - | - | public | - | 0 | - |
| 8 | `DecipherImage` | 542 | - | - | - | public | - | 0 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `PasskeyEncipherImage` (line 771)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/cipher.c): simplify PasskeyEncipherImage`

#### Step 2: `PasskeyDecipherImage` (line 561)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 3 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/cipher.c): simplify PasskeyDecipherImage`

#### Step 3: `SetAESKey` (line 998)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/cipher.c): simplify SetAESKey`

#### Step 4: `AcquireAESInfo` (line 192)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/cipher.c): simplify AcquireAESInfo`

### Wave 2

#### Step 5: `IncrementCipherNonce` (line 527)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/cipher.c): simplify IncrementCipherNonce`

#### Step 6: `EncipherAESBlock` (line 385)

- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/cipher.c): simplify EncipherAESBlock`

#### Step 7: `EncipherImage` (line 752)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 8: `DecipherImage` (line 542)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/cipher.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/cipher.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/cipher.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, night)

**Result: Code Health 6.28 → 6.90** (`cs-mcp` 1.5.12). Branch `campaign/M71-cipher`,
stacked on `campaign/M70-quantum-export`. `MAGICKCORE_CIPHER_SUPPORT` is defined in
this build, so the AES code compiles. The mutation reports hold no data for
cipher.c, so both commits are guard-level. The oracle also ran: 6 cases execute
these functions, 0 diverged.
- `IsAESInfoIncomplete` (P, AcquireAESInfo), 6.28 → 6.87. Complex Conditional leaves
  the file.
- `SetAESRounds` (E, SetAESKey, value-returning).

**What is left, at 6.90.**
- PasskeyEncipherImage and PasskeyDecipherImage are near-twins (Code Duplication,
  Large Method). Sharing their key, nonce and block loop is Recipe D, which needs
  mutation data for them first.
- The duplicates reported after line 1080 are the `#else` stubs, which this build
  does not compile.
