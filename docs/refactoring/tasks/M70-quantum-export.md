# M70 - refactor `MagickCore/quantum-export.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.24** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 3902 lines |
| Functions with findings | 20 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/quantum-export.c
```

The score must read 1.24. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 10 |
| Bumpy Road Ahead | 18 |
| Deep, Nested Complexity | 10 |
| Overall Code Complexity | file |
| Complex Method | 8 |
| Code Duplication | 13 |
| Large Method | 18 |
| Excess Number of Function Arguments | 20 |
| Overall Function Size | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `ExportBGRQuantum` | 357 | - | 4 | 9 | static | - | 93 | 5 of 11 |
| 2 | `ExportRGBQuantum` | 2863 | - | 4 | 9 | static | - | 6798 | 2 of 7 |
| 3 | `ExportGrayAlphaQuantum` | 2045 | 38 | - | 5 | static | - | 146 | 1 of 8 |
| 4 | `ExportGrayQuantum` | 1787 | - | 4 | 8 | static | - | 1080 | 7 of 13 |
| 5 | `ExportRGBAQuantum` | 3167 | - | 5 | 6 | static | - | 2821 | 0 of 2 |
| 6 | `ExportMultispectralQuantum` | 2620 | - | 4 | 7 | static | - | 71 | 1 of 9 |
| 7 | `ExportRGBOQuantum` | 3414 | - | 5 | 6 | static | - | 69 | 0 of 1 |
| 8 | `ExportBGROQuantum` | 905 | - | 5 | 6 | static | - | 28 | - |
| 9 | `ExportBGRAQuantum` | 658 | - | 5 | 6 | static | - | 69 | 1 of 2 |
| 10 | `ExportIndexAlphaQuantum` | 2421 | 28 | - | 5 | static | - | 240 | 0 of 7 |
| 11 | `ExportIndexQuantum` | 2253 | 24 | - | 5 | static | - | 744 | 1 of 5 |
| 12 | `ExportQuantumPixels` | 3667 | - | 4 | 4 | public | - | 12768 | 20 of 28 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `ExportBGRQuantum` (line 357)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 406, `lt_to_ge`: `for (x=0; x < (ssize_t) number_pixels; x++)`
  - base line 468, `rem_to_div`: `switch ((x+1) % 3)`
  - base line 495, `lt_to_ge`: `for (bit=0; bit < (ssize_t) (3*number_pixels % 2); bit++)`
  - base line 530, `post_inc_to_post_dec`: `for (x=0; x < (ssize_t) number_pixels; x++)`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantum-export.c): simplify ExportBGRQuantum`

#### Step 2: `ExportRGBQuantum` (line 2863)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 9 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2898, `lt_to_ge`: `for (x=0; x < (ssize_t) number_pixels; x++)`
  - base line 2974, `add_to_sub`: `switch ((x+1) % 3)`
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantum-export.c): simplify ExportRGBQuantum`

#### Step 3: `ExportGrayAlphaQuantum` (line 2045)

- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- The oracle missed 6 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2091, `gt_to_ge`: `*q|=(GetPixelLuma(image,p) > threshold ? black : white) << 3;`
  - base line 2097, `eq_to_ne`: `pixel=(unsigned char) (GetPixelAlpha(image,p) == OpaqueAlpha ?`
  - base line 2103, `ne_to_eq`: `if ((number_pixels % 4) != 0)`
  - base line 2109, `sub_to_add`: `(7-bit);`
  - base line 2165, `mul_to_div`: `pixel=SinglePrecisionToHalf(QuantumScale*(double)`
  - base line 2220, `lt_to_ge`: `for (x=0; x < (ssize_t) number_pixels; x++)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantum-export.c): simplify ExportGrayAlphaQuantum`

#### Step 4: `ExportGrayQuantum` (line 1787)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 8 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- The oracle missed 6 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1826, `lt_to_le`: `*q|=(GetPixelLuma(image,p) < threshold ? black : white) << 5;`
  - base line 1906, `mul_to_div`: `p+=(ptrdiff_t) 3*GetPixelChannels(image);`
  - base line 1909, `lt_to_le`: `if (x < (ssize_t) number_pixels)`
  - base line 1912, `lt_to_ge`: `if (x++ < ((ssize_t) number_pixels-1))`
  - base line 1915, `lt_to_le`: `if (x++ < (ssize_t) number_pixels)`
  - base line 2033, `lt_to_ge`: `for (x=0; x < (ssize_t) number_pixels; x++)`

Commit message: `refactor(MagickCore/quantum-export.c): simplify ExportGrayQuantum`

### Wave 2

#### Step 5: `ExportRGBAQuantum` (line 3167)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantum-export.c): simplify ExportRGBAQuantum`

#### Step 6: `ExportMultispectralQuantum` (line 2620)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- The oracle missed 6 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2637, `remove_void_call`: `ExportPixelChannel(image,quantum_info,number_pixels,p,q,`
  - base line 2638, `sub_to_add`: `(PixelChannel) (MetaPixelChannels+quantum_info->meta_channel-1));`
  - base line 2667, `post_inc_to_post_dec`: `for (x=0; x < (ssize_t) number_pixels; x++)`
  - base line 2681, `post_inc_to_post_dec`: `for (i=0; i < (ssize_t) GetImageChannels(image); i++)`
  - base line 2709, `post_inc_to_post_dec`: `for (i=0; i < (ssize_t) GetImageChannels(image); i++)`
  - base line 2723, `post_inc_to_post_dec`: `for (x=0; x < (ssize_t) number_pixels; x++)`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantum-export.c): simplify ExportMultispectralQuantum`

#### Step 7: `ExportRGBOQuantum` (line 3414)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3466, `post_inc_to_post_dec`: `for (x=0; x < (ssize_t) number_pixels; x++)`

Commit message: `refactor(MagickCore/quantum-export.c): simplify ExportRGBOQuantum`

#### Step 8: `ExportBGROQuantum` (line 905)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/quantum-export.c): simplify ExportBGROQuantum`

### Wave 3

#### Step 9: `ExportBGRAQuantum` (line 658)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 721, `rem_to_div`: `switch (n % 3)`

Commit message: `refactor(MagickCore/quantum-export.c): simplify ExportBGRAQuantum`

#### Step 10: `ExportIndexAlphaQuantum` (line 2421)

- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- The oracle missed 5 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2455, `eq_to_ne`: `pixel=(unsigned char) (GetPixelAlpha(image,p) == (Quantum)`
  - base line 2476, `rem_to_div`: `for (bit=3; bit >= (ssize_t) (4-(number_pixels % 4)); bit-=2)`
  - base line 2540, `lt_to_ge`: `for (x=0; x < (ssize_t) number_pixels; x++)`
  - base line 2540, `post_inc_to_post_dec`: `for (x=0; x < (ssize_t) number_pixels; x++)`
  - base line 2571, `post_inc_to_post_dec`: `for (x=0; x < (ssize_t) number_pixels; x++)`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantum-export.c): simplify ExportIndexAlphaQuantum`

#### Step 11: `ExportIndexQuantum` (line 2253)

- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2307, `rem_to_div`: `for (bit=7; bit >= (ssize_t) (8-(number_pixels % 8)); bit--)`
  - base line 2332, `rem_to_div`: `if ((number_pixels % 2) != 0)`
  - base line 2410, `lt_to_ge`: `for (x=0; x < (ssize_t) number_pixels; x++)`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/quantum-export.c): simplify ExportIndexQuantum`

#### Step 12: `ExportQuantumPixels` (line 3667)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- The oracle missed 8 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3756, `remove_void_call`: `SetPixelRed(image,GetPixelGreen(image,r),r);`
  - base line 3757, `remove_void_call`: `SetPixelGreen(image,quantum,r);`
  - base line 3783, `remove_void_call`: `ExportBGROQuantum(image,quantum_info,number_pixels,p,q);`
  - base line 3814, `remove_void_call`: `ExportCMYKOQuantum(image,quantum_info,number_pixels,p,q,exception);`
  - base line 3819, `remove_void_call`: `ExportCbYCrYQuantum(image,quantum_info,number_pixels,p,q);`
  - base line 3892, `post_inc_to_post_dec`: `for (x=0; x < (ssize_t) number_pixels; x++)`
  - and 2 more (`tools/oracle/mutate.py --function ExportQuantumPixels`)

Commit message: `refactor(MagickCore/quantum-export.c): simplify ExportQuantumPixels`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/quantum-export.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/quantum-export.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/quantum-export.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
