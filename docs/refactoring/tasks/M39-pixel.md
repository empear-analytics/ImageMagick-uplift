# M39 - refactor `MagickCore/pixel.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.08** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 6533 lines |
| Functions with findings | 27 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/pixel.c
```

The score must read 1.08. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 16 |
| Bumpy Road Ahead | 21 |
| Deep, Nested Complexity | 17 |
| Overall Code Complexity | file |
| Complex Method | 11 |
| Large Method | 23 |
| Excess Number of Function Arguments | 19 |
| Overall Function Size | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `InterpolatePixelChannel` | 4433 | 55 | - | 16 | public | - | 96 | 9 of 14 |
| 2 | `InterpolatePixelChannels` | 4870 | - | 5 | 19 | public | - | 136 | 24 of 27 |
| 3 | `ImportCharPixel` | 2509 | - | 4 | 11 | static | - | 30 | 11 of 38 |
| 4 | `ExportLongLongPixel` | 1336 | - | 5 | 9 | static | - | 15 | - |
| 5 | `ExportFloatPixel` | 922 | - | 5 | 9 | static | - | 21 | 1 of 2 |
| 6 | `ExportLongPixel` | 1129 | - | 5 | 9 | static | - | 15 | - |
| 7 | `ExportDoublePixel` | 715 | - | 5 | 9 | static | - | 15 | - |
| 8 | `ExportShortPixel` | 1755 | - | 5 | 9 | static | - | 23 | 1 of 3 |
| 9 | `ExportCharPixel` | 508 | - | 5 | 9 | static | - | 29 | 0 of 2 |
| 10 | `ExportQuantumPixel` | 1545 | - | 5 | 9 | static | - | 15 | - |
| 11 | `ImportShortPixel` | 3943 | - | 4 | 9 | static | - | 22 | 11 of 36 |
| 12 | `ImportQuantumPixel` | 3722 | - | 4 | 9 | static | - | 14 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `InterpolatePixelChannel` (line 4433)

- **Recipe E (extract function)** - 16 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- The oracle missed 5 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4585, `add_to_sub`: `pixels[i]=alpha[i]*(double) p[i*(ssize_t) GetPixelChannels(image)+`
  - base line 4647, `add_to_sub`: `pixels[i]=alpha[i]*(double) p[i*(ssize_t) GetPixelChannels(image)+`
  - base line 4711, `mul_to_div`: `alpha[i]=QuantumScale*(double) GetPixelAlpha(image,p+i*`
  - base line 4713, `mul_to_div`: `pixels[i]=alpha[i]*(double) p[i*(ssize_t) GetPixelChannels(image)+`
  - base line 4805, `mul_to_div`: `pixels[i]=alpha[i]*(double) p[i*(ssize_t) GetPixelChannels(image)+`

Commit message: `refactor(MagickCore/pixel.c): simplify InterpolatePixelChannel`

#### Step 2: `InterpolatePixelChannels` (line 4870)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 19 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4971, `remove_void_call`: `SetPixelChannel(destination,channel,ClampToQuantum(sum),pixel);`
  - base line 5085, `ge_to_gt`: `if ((x-x_offset) >= 0.75)`
  - base line 5350, `mul_to_div`: `pixels[j]=alpha[j]*(double) p[j*(ssize_t) GetPixelChannels(source)+`

Commit message: `refactor(MagickCore/pixel.c): simplify InterpolatePixelChannels`

#### Step 3: `ImportCharPixel` (line 2509)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 11 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- The oracle missed 26 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2538, `remove_void_call`: `SetPixelBlue(image,ScaleCharToQuantum(*p++),q);`
  - base line 2539, `remove_void_call`: `SetPixelGreen(image,ScaleCharToQuantum(*p++),q);`
  - base line 2540, `remove_void_call`: `SetPixelRed(image,ScaleCharToQuantum(*p++),q);`
  - base line 2557, `remove_void_call`: `SetPixelBlue(image,ScaleCharToQuantum(*p++),q);`
  - base line 2558, `remove_void_call`: `SetPixelGreen(image,ScaleCharToQuantum(*p++),q);`
  - base line 2559, `remove_void_call`: `SetPixelRed(image,ScaleCharToQuantum(*p++),q);`
  - and 20 more (`tools/oracle/mutate.py --function ImportCharPixel`)
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/pixel.c): simplify ImportCharPixel`

#### Step 4: `ExportLongLongPixel` (line 1336)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).

Commit message: `refactor(MagickCore/pixel.c): simplify ExportLongLongPixel`

### Wave 2

#### Step 5: `ExportFloatPixel` (line 922)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/pixel.c): simplify ExportFloatPixel`

#### Step 6: `ExportLongPixel` (line 1129)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).

Commit message: `refactor(MagickCore/pixel.c): simplify ExportLongPixel`

#### Step 7: `ExportDoublePixel` (line 715)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).

Commit message: `refactor(MagickCore/pixel.c): simplify ExportDoublePixel`

#### Step 8: `ExportShortPixel` (line 1755)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1777, `lt_to_ge`: `for (y=0; y < (ssize_t) roi->height; y++)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/pixel.c): simplify ExportShortPixel`

### Wave 3

#### Step 9: `ExportCharPixel` (line 508)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 585, `add_to_sub`: `p=GetVirtualPixels(image,roi->x,roi->y+y,roi->width,1,exception);`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/pixel.c): simplify ExportCharPixel`

#### Step 10: `ExportQuantumPixel` (line 1545)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).

Commit message: `refactor(MagickCore/pixel.c): simplify ExportQuantumPixel`

#### Step 11: `ImportShortPixel` (line 3943)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- The oracle missed 22 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3965, `post_inc_to_post_dec`: `for (y=0; y < (ssize_t) roi->height; y++)`
  - base line 3972, `remove_void_call`: `SetPixelBlue(image,ScaleShortToQuantum(*p++),q);`
  - base line 3973, `remove_void_call`: `SetPixelGreen(image,ScaleShortToQuantum(*p++),q);`
  - base line 3974, `remove_void_call`: `SetPixelRed(image,ScaleShortToQuantum(*p++),q);`
  - base line 3991, `remove_void_call`: `SetPixelBlue(image,ScaleShortToQuantum(*p++),q);`
  - base line 3992, `remove_void_call`: `SetPixelGreen(image,ScaleShortToQuantum(*p++),q);`
  - and 16 more (`tools/oracle/mutate.py --function ImportShortPixel`)
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/pixel.c): simplify ImportShortPixel`

#### Step 12: `ImportQuantumPixel` (line 3722)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).

Commit message: `refactor(MagickCore/pixel.c): simplify ImportQuantumPixel`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/pixel.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/pixel.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/pixel.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
