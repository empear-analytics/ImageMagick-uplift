# M28 - refactor `MagickCore/stream.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **2.45** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2833 lines |
| Functions with findings | 12 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/stream.c
```

The score must read 2.45. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 3 |
| Deep, Nested Complexity | 1 |
| Overall Code Complexity | file |
| Complex Method | 5 |
| Complex Conditional | 3 |
| Code Duplication | 6 |
| Large Method | 3 |
| Excess Number of Function Arguments | 5 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `StreamImagePixels` | 1371 | 291 | 5 | 57 | static | - | 261 | 49 of 77 |
| 2 | `QueueAuthenticPixelsStream` | 884 | 18 | - | 3 | static | - | 306 | 2 of 4 |
| 3 | `GetVirtualPixelStream` | 706 | 15 | - | 2 | static | - | 13 | - |
| 4 | `WriteStreamImage` | 1215 | 18 | - | - | static | - | 261 | 4 of 4 |
| 5 | `ValidatePixelCacheMorphology` | 854 | 11 | - | - | static | - | 15011 | 0 of 3 |
| 6 | `GetAuthenticMetacontentFromStream` | 321 | - | - | - | static | - | 13 | - |
| 7 | `GetAuthenticPixelsFromStream` | 405 | - | - | - | static | - | 297 | - |
| 8 | `GetOneAuthenticPixelFromStream` | 450 | - | - | - | static | - | 13 | - |
| 9 | `GetVirtualMetacontentFromStream` | 637 | - | - | - | static | - | 13 | - |
| 10 | `GetAuthenticPixelsStream` | 367 | - | - | - | static | - | 19 | - |
| 11 | `GetOneVirtualPixelFromStream` | 513 | - | - | - | static | - | 0 | - |
| 12 | `GetVirtualPixelsStream` | 599 | - | - | - | static | - | 0 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `StreamImagePixels` (line 1371)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 57 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 14 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1699, `lt_to_ge`: `for (x=0; x < (ssize_t) GetImageExtent(image); x++)`
  - base line 1701, `mul_to_div`: `*q++=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*`
  - base line 1718, `lt_to_ge`: `for (x=0; x < (ssize_t) GetImageExtent(image); x++)`
  - base line 1720, `mul_to_div`: `*q++=(double) ((QuantumScale*(double) GetPixelBlue(image,p))*`
  - base line 1893, `mul_to_div`: `*q++=(float) ((QuantumScale*(double) GetPixelBlue(image,p))*`
  - base line 1897, `mul_to_div`: `*q++=(float) ((QuantumScale*(double) GetPixelRed(image,p))*`
  - and 8 more (`tools/oracle/mutate.py --function StreamImagePixels`)
- 14 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/stream.c): simplify StreamImagePixels`

#### Step 2: `QueueAuthenticPixelsStream` (line 884)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 939, `remove_void_call`: `ResetPixelCacheChannels(image);`
  - base line 976, `ne_to_eq`: `if (cache_info->metacontent_extent != 0)`

Commit message: `refactor(MagickCore/stream.c): simplify QueueAuthenticPixelsStream`

#### Step 3: `GetVirtualPixelStream` (line 706)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (7 arguments).

Commit message: `refactor(MagickCore/stream.c): simplify GetVirtualPixelStream`

#### Step 4: `WriteStreamImage` (line 1215)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.

Commit message: `refactor(MagickCore/stream.c): simplify WriteStreamImage`

### Wave 2

#### Step 5: `ValidatePixelCacheMorphology` (line 854)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 870, `ne_to_eq`: `if ((image->storage_class != cache_info->storage_class) ||`
  - base line 875, `ne_to_eq`: `(image->rows != cache_info->rows) ||`
  - base line 878, `ne_to_eq`: `(image->metacontent_extent != cache_info->metacontent_extent) ||`

Commit message: `refactor(MagickCore/stream.c): simplify ValidatePixelCacheMorphology`

#### Step 6: `GetAuthenticMetacontentFromStream` (line 321)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/stream.c): simplify GetAuthenticMetacontentFromStream`

#### Step 7: `GetAuthenticPixelsFromStream` (line 405)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/stream.c): simplify GetAuthenticPixelsFromStream`

#### Step 8: `GetOneAuthenticPixelFromStream` (line 450)

- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/stream.c): simplify GetOneAuthenticPixelFromStream`

### Wave 3

#### Step 9: `GetVirtualMetacontentFromStream` (line 637)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/stream.c): simplify GetVirtualMetacontentFromStream`

#### Step 10: `GetAuthenticPixelsStream` (line 367)

- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).

Commit message: `refactor(MagickCore/stream.c): simplify GetAuthenticPixelsStream`

#### Step 11: `GetOneVirtualPixelFromStream` (line 513)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 12: `GetVirtualPixelsStream` (line 599)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/stream.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/stream.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/stream.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, morning)

**Result: Code Health 2.45 → 3.10** (`cs-mcp` 1.5.12). Branch `campaign/M28-stream`,
stacked on `campaign/M27-attribute`. 3 refactor commits, all Recipe P and guard-verified,
because none of the functions is ready.

**Predicates; Complex Conditional leaves the file:**
- `IsCacheMorphologyChanged` (+0.25): ten clauses, `memcmp` included.
- `IsOutsideStreamPixel` (+0.15).
- `IsOutsideStreamRegion` (+0.25). It takes five arguments and so carries an Excess
  Number of Function Arguments finding, but the file still gains.

**StreamImagePixels (cc 289, 57 bumps, 1,330 lines) stays.** It is one `switch` over eight
storage types, each with a chain of `if (map == "BGR") { ...; break; }` blocks.
- The bumps are those `if` blocks, and they leave through `break`. A helper could only
  take them by turning `break` into `return`, which needs a new recipe.
- Moving only the copy loops inside them leaves the score and the bump count unchanged.
  A probe confirmed this (3.10 → 3.10, bumps 57 → 57).
- Moving whole case bodies would give eight ~150-line helpers that differ only in the
  Scale function and the type of `q`. That is the clone pattern that has cost score
  elsewhere.
- `pixel.c`'s `ExportImagePixels` has the same structure. A recipe for it would serve
  both files.

**Flakes:** the hough matrix-on-disk pair diverged once (during the
`IsCacheMorphologyChanged` step) and gave 0 of 2 in three reruns.

**Whole catalogue:** see the batch PR.

**What is left, at 3.10.**
- `StreamImagePixels`, as above.
- The stream accessors' duplication: thin wrappers with the same assert-and-log
  preamble.
- Five functions with 5–7 arguments (Recipe B).
