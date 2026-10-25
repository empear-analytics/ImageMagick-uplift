# M72 - refactor `MagickCore/distribute-cache.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **5.01** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2073 lines |
| Functions with findings | 19 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/distribute-cache.c
```

The score must read 5.01. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 4 |
| Overall Code Complexity | file |
| Complex Method | 5 |
| Complex Conditional | 1 |
| Code Duplication | 13 |
| Large Method | 3 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `DistributePixelCacheServer` | 1364 | 25 | - | 2 | public | - | 0 | - |
| 2 | `dpc_receive_deadline` | 239 | 15 | - | 2 | static | - | 0 | - |
| 3 | `dpc_receive` | 205 | 13 | - | 2 | static | - | 0 | - |
| 4 | `DistributePixelCacheConnectionHandler` | 1152 | 18 | - | - | static | - | 0 | - |
| 5 | `SIPHash24` | 338 | 13 | - | - | static | - | 0 | - |
| 6 | `GetDPCPolicyLimit` | 1294 | - | - | - | static | - | 0 | - |
| 7 | `DestroyDistributeCacheInfo` | 670 | - | - | - | public | - | 0 | - |
| 8 | `ReadDistributeCacheMetacontent` | 863 | - | - | - | static | - | 0 | - |
| 9 | `ReadDistributeCachePixels` | 930 | - | - | - | static | - | 0 | - |
| 10 | `WriteDistributeCacheMetacontent` | 998 | - | - | - | static | - | 0 | - |
| 11 | `WriteDistributeCachePixels` | 1064 | - | - | - | static | - | 0 | - |
| 12 | `GetDistributeCacheHostname` | 1606 | - | - | - | public | - | 0 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `DistributePixelCacheServer` (line 1364)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 2: `dpc_receive_deadline` (line 239)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 3: `dpc_receive` (line 205)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 4: `DistributePixelCacheConnectionHandler` (line 1152)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

### Wave 2

#### Step 5: `SIPHash24` (line 338)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 6: `GetDPCPolicyLimit` (line 1294)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 7: `DestroyDistributeCacheInfo` (line 670)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 8: `ReadDistributeCacheMetacontent` (line 863)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

### Wave 3

#### Step 9: `ReadDistributeCachePixels` (line 930)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 10: `WriteDistributeCacheMetacontent` (line 998)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 11: `WriteDistributeCachePixels` (line 1064)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 12: `GetDistributeCacheHostname` (line 1606)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/distribute-cache.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/distribute-cache.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/distribute-cache.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
