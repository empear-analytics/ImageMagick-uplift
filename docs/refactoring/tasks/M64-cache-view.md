# M64 - refactor `MagickCore/cache-view.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **7.78** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1115 lines |
| Functions with findings | 18 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/cache-view.c
```

The score must read 7.78. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Code Duplication | 18 |
| Excess Number of Function Arguments | 7 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `GetCacheViewAuthenticPixels` | 298 | - | - | - | public | - | 7966 | - |
| 2 | `GetCacheViewAuthenticPixelQueue` | 378 | - | - | - | public | - | 6 | - |
| 3 | `GetCacheViewColorspace` | 413 | - | - | - | public | - | 7 | - |
| 4 | `GetCacheViewExtent` | 447 | - | - | - | public | - | 6 | - |
| 5 | `GetCacheViewImage` | 489 | - | - | - | public | - | 7 | - |
| 6 | `GetCacheViewStorageClass` | 522 | - | - | - | public | - | 7 | - |
| 7 | `GetCacheViewVirtualPixelQueue` | 600 | - | - | - | public | - | 6 | - |
| 8 | `GetCacheViewVirtualPixels` | 650 | - | - | - | public | - | 9735 | - |
| 9 | `GetOneCacheViewAuthenticPixel` | 700 | - | - | - | public | - | 7 | - |
| 10 | `GetOneCacheViewVirtualPixel` | 772 | - | - | - | public | - | 7 | - |
| 11 | `GetOneCacheViewVirtualPixelInfo` | 845 | - | - | - | public | - | 27 | 2 of 3 |
| 12 | `GetOneCacheViewVirtualMethodPixel` | 904 | - | - | - | public | - | 7 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `GetCacheViewAuthenticPixels` (line 298)

- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/cache-view.c): simplify GetCacheViewAuthenticPixels`

#### Step 2: `GetCacheViewAuthenticPixelQueue` (line 378)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/cache-view.c): simplify GetCacheViewAuthenticPixelQueue`

#### Step 3: `GetCacheViewColorspace` (line 413)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 7 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/cache-view.c): simplify GetCacheViewColorspace`

#### Step 4: `GetCacheViewExtent` (line 447)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/cache-view.c): simplify GetCacheViewExtent`

### Wave 2

#### Step 5: `GetCacheViewImage` (line 489)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 7 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/cache-view.c): simplify GetCacheViewImage`

#### Step 6: `GetCacheViewStorageClass` (line 522)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 7 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/cache-view.c): simplify GetCacheViewStorageClass`

#### Step 7: `GetCacheViewVirtualPixelQueue` (line 600)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/cache-view.c): simplify GetCacheViewVirtualPixelQueue`

#### Step 8: `GetCacheViewVirtualPixels` (line 650)

- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/cache-view.c): simplify GetCacheViewVirtualPixels`

### Wave 3

#### Step 9: `GetOneCacheViewAuthenticPixel` (line 700)

- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 7 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/cache-view.c): simplify GetOneCacheViewAuthenticPixel`

#### Step 10: `GetOneCacheViewVirtualPixel` (line 772)

- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 7 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/cache-view.c): simplify GetOneCacheViewVirtualPixel`

#### Step 11: `GetOneCacheViewVirtualPixelInfo` (line 845)

- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/cache-view.c): simplify GetOneCacheViewVirtualPixelInfo`

#### Step 12: `GetOneCacheViewVirtualMethodPixel` (line 904)

- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 7 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/cache-view.c): simplify GetOneCacheViewVirtualMethodPixel`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/cache-view.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/cache-view.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/cache-view.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, night)

**Result: Code Health 7.78, unchanged.** The only findings are Code Duplication over 18
thin wrappers and Excess Number of Function Arguments on the public pixel accessors
(5–6 arguments, public API).

**Tried and reverted.** GetOneCacheViewAuthenticPixel, GetOneCacheViewVirtualPixel and
GetOneCacheViewVirtualMethodPixel share an identical background-colour fill. Extracting
it from the first (`SetPixelToBackgroundColor`) left the score flat and the
duplication down only from 18 functions to 17. The step that would pay, using the
helper in the other two, is Recipe D. No case executes those functions (4 mutants
each, all uncovered), so D is not allowed and the flat step had no legal follow-up.

**What would unlock it:** an oracle case that calls GetOneCacheViewVirtualPixel and
GetOneCacheViewAuthenticPixel. The C API driver can do that.

### Addendum (2026-10-26, Mac, night): fresh mutation data

Cases now reach the three GetOneCacheView* getters (7 each). Mutated on the Mac tonight
(`mutation-cview-*.json`), all three are ready (4 of 4 killed each, thin). The D step
they waited for (`SetPixelToBackgroundColor`, one helper for the three identical
background fills) then verified cleanly (21 case runs, 0 diverged). But it left the
score flat at 7.78, and duplication dropped only from 18 functions to 17: the three
getters stay duplicates of each other in their remaining lines. With no next step to
name, rule 2 reverts it. cache-view.c's remaining duplication is the thin-wrapper
pattern that Code Health weighs but no recipe here reduces.
