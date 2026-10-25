# M35 - refactor `MagickCore/feature.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.60** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2341 lines |
| Functions with findings | 6 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/feature.c
```

The score must read 1.60. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 4 |
| Bumpy Road Ahead | 5 |
| Deep, Nested Complexity | 5 |
| Overall Code Complexity | file |
| Complex Method | 2 |
| Complex Conditional | 1 |
| Large Method | 5 |
| Excess Number of Function Arguments | 4 |
| Overall Function Size | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `GetImageFeatures` | 616 | 180 | 6 | 24 | public | 6 | 26 | 16 of 25 |
| 2 | `CannyEdgeImage` | 240 | - | 5 | 9 | public | 4 | 12 | 2 of 2 |
| 3 | `HoughLineImage` | 1828 | - | 7 | 5 | public | 1 | 21 | 6 of 8 |
| 4 | `MeanShiftImage` | 2155 | - | 7 | 3 | public | 2 | 12 | 8 of 11 |
| 5 | `TraceEdges` | 160 | - | 5 | 3 | static | - | 12 | - |
| 6 | `RenderHoughLines` | 1763 | 9 | - | - | static | - | 21 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `GetImageFeatures` (line 616)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 24 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 6 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 5 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 669, `add_to_sub`: `if ((image->columns < (distance+1)) || (image->rows < (distance+1)))`
  - base line 769, `gt_to_ge`: `if (gray.green > number_grays)`
  - base line 1556, `gt_to_ge`: `(entropy_x.direction[i].green > entropy_y.direction[i].green ?`
  - base line 1556, `gt_to_le`: `(entropy_x.direction[i].green > entropy_y.direction[i].green ?`
  - base line 1587, `mul_to_div`: `(sqrt(fabs(1.0-exp(-2.0*(double) (entropy_xy2.direction[i].alpha-`
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/feature.c): simplify GetImageFeatures`

#### Step 2: `CannyEdgeImage` (line 240)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 4 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/feature.c): simplify CannyEdgeImage`

#### Step 3: `HoughLineImage` (line 1828)

- **Recipe G (guard clauses)** - nesting is 7, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2089, `ne_to_eq`: `if (artifact != (const char *) NULL)`
  - base line 2104, `remove_void_call`: `AppendImageToList(&lines_image,accumulator_image);`

Commit message: `refactor(MagickCore/feature.c): simplify HoughLineImage`

#### Step 4: `MeanShiftImage` (line 2155)

- **Recipe G (guard clauses)** - nesting is 7, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2235, `remove_void_call`: `GetPixelInfo(image,&mean_pixel);`
  - base line 2261, `div_to_mul`: `for (v=(-((ssize_t) height/2)); v <= (((ssize_t) height/2)); v++)`
  - base line 2266, `div_to_mul`: `for (u=(-((ssize_t) width/2)); u <= (((ssize_t) width/2)); u++)`

Commit message: `refactor(MagickCore/feature.c): simplify MeanShiftImage`

### Wave 2

#### Step 5: `TraceEdges` (line 160)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (7 arguments).

Commit message: `refactor(MagickCore/feature.c): simplify TraceEdges`

#### Step 6: `RenderHoughLines` (line 1763)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/feature.c): simplify RenderHoughLines`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/feature.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/feature.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/feature.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, late morning)

**Result: Code Health 1.60 → 1.61** (`cs-mcp` 1.5.12). Branch `campaign/M35-feature`,
stacked on `campaign/M34-configure`. 1 refactor commit: `SetCannyOrientation` (E,
oracle), the orientation classification inside CannyEdgeImage's parallel loop.
- The `CannyInfo` struct passes by value and comes back, so `pixel.orientation=` keeps
  its text.
- CannyEdgeImage: bumps 9 → 7, nesting 5 → 4.
- Its readiness rests on thin evidence (2 mutants).

**Blocked: GetImageFeatures (cc 180, 24 bumps).** Its working type `ChannelStatistics`
is a typedef declared *inside* the function. A helper outside it cannot name that type,
so none of its blocks can move: not even the six-pointer allocation test (probe: build
error). Moving the typedef to file scope would unblock the whole function, but no
catalogue recipe covers that. It would be a small, purely declarative recipe:

- **Hoist a local type:** move a typedef that sits at the top of a function body, before
  any statement, to file scope directly above the function. No other identifier may have
  the same name.

That is a candidate for the catalogue, for the owner to decide.

**What is left, at 1.61.**
- `GetImageFeatures`, as above.
- `HoughLineImage` and `MeanShiftImage` (nesting 7, careful and not ready). Their bodies
  are parallel loops that write several of the iteration's locals.
- `CannyEdgeImage`'s neighbour-fetch `switch`, which writes two structs through `&`.
- `TraceEdges` takes 7 arguments (Recipe B).
