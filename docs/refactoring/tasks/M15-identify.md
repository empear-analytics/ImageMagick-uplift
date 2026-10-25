# M15 - refactor `MagickCore/identify.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **2.16** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 1706 lines |
| Functions with findings | 6 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/identify.c
```

The score must read 2.16. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 4 |
| Deep, Nested Complexity | 4 |
| Overall Code Complexity | file |
| Complex Method | 4 |
| Complex Conditional | 5 |
| Large Method | 2 |
| Excess Number of Function Arguments | 3 |
| Overall Function Size | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `IdentifyImage` | 478 | 285 | 7 | 38 | public | - | 283 | 263 of 296 |
| 2 | `GetLocationStatistics` | 135 | 20 | 5 | 5 | static | - | 6 | 16 of 16 |
| 3 | `PrintChannelLocations` | 284 | 14 | 4 | 3 | static | - | 6 | 18 of 18 |
| 4 | `PrintChannelPerceptualHash` | 390 | 11 | 4 | 3 | static | - | 18 | 21 of 21 |
| 5 | `PrintChannelMoments` | 355 | - | - | - | static | - | 18 | 6 of 6 |
| 6 | `PrintChannelStatistics` | 446 | - | - | - | static | - | 197 | 10 of 10 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `IdentifyImage` (line 478)

- **Recipe G (guard clauses)** - nesting is 7, target is under 4.
- **Recipe E (extract function)** - 38 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 27 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 631, `ne_to_eq`: `return(ferror(file) != 0 ? MagickFalse : MagickTrue);`
  - base line 635, `remove_void_call`: `GetTimerInfo(&image->timer);`
  - base line 652, `ne_to_eq`: `if ((image->magick_columns != 0) || (image->magick_rows != 0))`
  - base line 652, `ne_to_eq`: `if ((image->magick_columns != 0) || (image->magick_rows != 0))`
  - base line 681, `le_to_lt`: `if (image->total_colors <= image->colors)`
  - base line 771, `ne_to_eq`: `if ((image->magick_columns != 0) || (image->magick_rows != 0))`
  - and 21 more (`tools/oracle/mutate.py --function IdentifyImage`)
- 6 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/identify.c): simplify IdentifyImage`

#### Step 2: `GetLocationStatistics` (line 135)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/identify.c): simplify GetLocationStatistics`

#### Step 3: `PrintChannelLocations` (line 284)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (7 arguments).
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/identify.c): simplify PrintChannelLocations`

#### Step 4: `PrintChannelPerceptualHash` (line 390)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/identify.c): simplify PrintChannelPerceptualHash`

### Wave 2

#### Step 5: `PrintChannelMoments` (line 355)

- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).

Commit message: `refactor(MagickCore/identify.c): simplify PrintChannelMoments`

#### Step 6: `PrintChannelStatistics` (line 446)

- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).

Commit message: `refactor(MagickCore/identify.c): simplify PrintChannelStatistics`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/identify.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/identify.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/identify.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, overnight)

**Result: Code Health 2.16 → 3.21** (`cs-mcp` 1.5.12). Branch `campaign/M15-identify`,
stacked on `campaign/M14-geometry` (PR #20). 17 refactor commits, one recipe and one
function each:
- 4 Recipe P: 3 oracle-verified, 1 guard-verified;
- 13 Recipe E: 8 oracle-verified, 5 guard-verified.

Every function touched is ready (`IdentifyImage` 93%). The guard-verified steps are those
whose moved lines hold unmatched survivors; each commit names the lines.

**One category leaves the file:** Complex Conditional. The predicates are
`HasPageGeometry` (two sites), `IsSingleUnnumberedImage`, `HasChromaticity` and
`IsDirectoryNameCharacter`. Deep Nesting and Large Method leave `GetLocationStatistics`.

| Steps | Recipe | Score |
| --- | --- | --- |
| the five compound tests as predicates | P | 2.16 → 2.64 |
| `IdentifyImage`'s verbose sections: channel depths, properties, Hu moments | E | → 2.73 |
| the IPTC tag table, value printing and record listing, then the profile listing | E | → 2.94 |
| the colormap, chromaticity, montage directory, convex hull and transparent-colour lines | E | → 3.10 |
| `GetLocationStatistics`' channel loop | E | → 3.21 |

`IdentifyImage` goes from cc 285, 1,143 lines and nesting 7 to cc 150, 696 lines and
nesting 5.

**Whole catalogue** at the branch tip against `8a4c272f1`: 15,884 cases, 1 diverged
(`gaps/c5651ffad8`, the known log-map tie).

**Tried and reverted.**
- The channel statistics table (a 100-line, 5-argument helper: 2.68 → 2.67).
- The artifact listing after the property listing, and the features table after the
  moments table. CodeScene reports each pair as duplicates (2.63, 2.59), and the pairs
  differ in more than one call, so Recipe D cannot merge them.
- The whole profile listing before its IPTC parts were split out (a 140-line helper with
  nesting 7: 2.65).
- The transparent-pixel search with its `x=0` (the helper is reported at nesting 4:
  3.06).

**What is left, at 3.21.** In `IdentifyImage`:
- the `locate` and one-line (non-verbose) paths, which `return` from inside;
- the statistics and features tables, as above;
- the artifact and registry listings.
Elsewhere, Excess Arguments on the `PrintChannel*` helpers, which predate this task.

**Possible bugs, reported, not fixed.**
- `IdentifyImage` prints the "Composite mask" statistics from `WriteMaskPixelChannel`;
  `CompositeMaskPixelChannel` looks intended.
- The profile listing (now `PrintImageProfiles`) does `continue` when
  `GetImageProfile()` returns NULL, before it advances `name`. A NULL profile would loop
  forever.
