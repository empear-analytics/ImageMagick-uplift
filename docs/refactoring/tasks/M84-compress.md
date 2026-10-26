# M84 - refactor `MagickCore/compress.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **5.20** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1325 lines |
| Functions with findings | 4 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/compress.c
```

The score must read 5.20. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Bumpy Road Ahead | 3 |
| Deep, Nested Complexity | 3 |
| Overall Code Complexity | file |
| Complex Method | 4 |
| Large Method | 4 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `HuffmanDecodeImage` | 362 | 57 | 7 | 10 | public | - | 5 | 64 of 64 |
| 2 | `HuffmanEncodeImage` | 668 | 34 | 5 | 9 | public | - | 8 | 50 of 50 |
| 3 | `LZWEncodeImage` | 911 | 14 | 5 | 2 | public | - | 19 | 18 of 18 |
| 4 | `PackbitsEncodeImage` | 1076 | 13 | - | - | public | - | 48 | 24 of 24 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `HuffmanDecodeImage` (line 362)

- **Recipe G (guard clauses)** - nesting is 7, target is under 4.
- **Recipe E (extract function)** - 10 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 5 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/compress.c): simplify HuffmanDecodeImage`

#### Step 2: `HuffmanEncodeImage` (line 668)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 8 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/compress.c): simplify HuffmanEncodeImage`

#### Step 3: `LZWEncodeImage` (line 911)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/compress.c): simplify LZWEncodeImage`

#### Step 4: `PackbitsEncodeImage` (line 1076)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/compress.c): simplify PackbitsEncodeImage`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/compress.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/compress.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/compress.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-26, Mac, night): Recipe H, first uses

This task added **Recipe H - Hoist Function-Local Macros** (`playbook:` commit, with
`tools/pp_identical.sh`) and applied it to the four files whose blocks it had pinned. The
branch `campaign/M84-hoist` therefore carries more than compress.c. Each hoist is its own
commit, proven by an `IDENTICAL` preprocessed translation unit, and is followed in the
same branch by the steps it unblocks:

| File | Hoisted from | Code Health | Steps it unblocked |
|---|---|---|---|
| compress.c | HuffmanDecodeImage (31 lines) | 5.20 → 5.28 | `InitializeHuffmanHashes` (E, oracle) |
| signature.c | TransformSignature (8 lines) | 8.49 → 8.54 | `ExpandMessageSchedule` (E, guard) |
| gem.c | GenerateDifferentialNoise (8 lines) | 9.12 → 9.21 | `GenerateGaussianNoise` (E, oracle) |
| profile.c | SyncExifProfile (9 lines) | 2.54 → 2.59 | `IsExifDirectoryOffset`, `IsNextDirectoryOffset` (P, oracle) |

The compress.c hoist raised the score by itself (5.20 → 5.27), because the macro lines
no longer count toward HuffmanDecodeImage. The other hoists were flat until their
extraction followed.

**Tried and reverted.**
- signature.c `LoadMessageWords`. It would leave TransformSignature's `T` used only by
  its closing `T=0;` wipe, and removing that wipe is not this recipe's call.
- gem.c `GeneratePoissonNoise`. A file-level Overall Code Complexity finding appeared
  (9.21 → 8.70).

**Still pinned after the hoist.** HuffmanDecodeImage's decode loop uses `InputBit`,
which advances `byte`, `mask`, `runlength` and `bit` together. That calls for Recipe O or
K (out-parameters), not E. LZWEncodeImage and HuffmanEncodeImage write their bit
accumulators through `OutputCode` and `HuffmanOutputCode` in the same way.
