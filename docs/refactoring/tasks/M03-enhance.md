# M03 - refactor `MagickCore/enhance.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.53** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 4597 lines |
| Functions with findings | 34 |
| Measured with | `cs-mcp` 1.5.12, `cs` not installed |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/enhance.c
```

The score must read 1.53. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 3 |
| Bumpy Road Ahead | 20 |
| Deep, Nested Complexity | 10 |
| Overall Code Complexity | file |
| Complex Method | 17 |
| Complex Conditional | 3 |
| Code Duplication | 10 |
| Large Method | 19 |
| Excess Number of Function Arguments | 17 |
| Overall Function Size | file |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `EqualizeImage` | 2053 | 45 | 4 | 12 | public | 2 | 61 | 77 of 87 |
| 2 | `ContrastStretchImage` | 1558 | 49 | 4 | 10 | public | 2 | 171 | 85 of 97 |
| 3 | `NegateImage` | 3953 | 38 | 5 | 8 | public | 2 | 125 | 43 of 49 |
| 4 | `ColorDecisionListImage` | 1018 | 43 | 4 | 7 | public | 2 | 5 | 56 of 63 |
| 5 | `GrayscaleImage` | 2487 | 33 | 4 | 5 | public | 2 | 116 | 32 of 34 |
| 6 | `GammaImage` | 2335 | 25 | 4 | 5 | public | 2 | 6 | 28 of 32 |
| 7 | `ModulateImage` | 3645 | 44 | - | 5 | public | 2 | 86 | 34 of 46 |
| 8 | `ClipCLAHEHistogram` | 303 | 15 | 4 | 4 | static | - | 13 | 27 of 32 |
| 9 | `CLAHEImage` | 620 | 30 | - | 4 | public | - | 13 | 48 of 54 |
| 10 | `SigmoidalContrastImage` | 4280 | - | 4 | 5 | public | 2 | 24 | 28 of 34 |
| 11 | `ClutImage` | 840 | 25 | - | 4 | public | 2 | 5 | 32 of 37 |
| 12 | `HaldClutImage` | 2699 | 30 | - | 3 | public | 2 | 5 | 57 of 63 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `EqualizeImage` (line 2053)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 12 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2226, `ne_to_eq`: `if (black[channel] != white[channel])`
  - base line 2228, `mul_to_div`: `GetPixelChannels(image)*ScaleQuantumToMap(`
  - base line 2229, `add_to_sub`: `ClampToQuantum(image->colormap[j].alpha))+channel];`
- 7 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/enhance.c): simplify EqualizeImage`

#### Step 2: `ContrastStretchImage` (line 1558)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 10 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1751, `ne_to_eq`: `if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)`
  - base line 1755, `mul_to_div`: `GetPixelChannels(image)*ScaleQuantumToMap(ClampToQuantum(`
  - base line 1756, `add_to_sub`: `image->colormap[j].alpha))+(size_t) i];`
- 9 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/enhance.c): simplify ContrastStretchImage`

#### Step 3: `NegateImage` (line 3953)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 8 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4107, `post_inc_to_post_dec`: `progress++;`
- 5 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/enhance.c): simplify NegateImage`

#### Step 4: `ColorDecisionListImage` (line 1018)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 7 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- Only 5 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- 7 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/enhance.c): simplify ColorDecisionListImage`

### Wave 2

#### Step 5: `GrayscaleImage` (line 2487)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/enhance.c): simplify GrayscaleImage`

#### Step 6: `GammaImage` (line 2335)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2391, `ne_to_eq`: `if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)`
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/enhance.c): simplify GammaImage`

#### Step 7: `ModulateImage` (line 3645)

- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 8 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 3741, `remove_void_call`: `ModulateHCL(percent_hue,percent_saturation,percent_brightness,`
  - base line 3747, `remove_void_call`: `ModulateHCLp(percent_hue,percent_saturation,percent_brightness,`
  - base line 3753, `remove_void_call`: `ModulateHSB(percent_hue,percent_saturation,percent_brightness,`
  - base line 3759, `remove_void_call`: `ModulateHSI(percent_hue,percent_saturation,percent_brightness,`
  - base line 3772, `remove_void_call`: `ModulateHSV(percent_hue,percent_saturation,percent_brightness,`
  - base line 3778, `remove_void_call`: `ModulateHWB(percent_hue,percent_saturation,percent_brightness,`
  - and 2 more (`tools/oracle/mutate.py --function ModulateImage`)
- 4 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/enhance.c): simplify ModulateImage`

#### Step 8: `ClipCLAHEHistogram` (line 303)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- 5 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/enhance.c): simplify ClipCLAHEHistogram`

### Wave 3

#### Step 9: `CLAHEImage` (line 620)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- 6 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/enhance.c): simplify CLAHEImage`

#### Step 10: `SigmoidalContrastImage` (line 4280)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 4352, `ne_to_eq`: `if ((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0)`
- 5 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/enhance.c): simplify SigmoidalContrastImage`

#### Step 11: `ClutImage` (line 840)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- Only 5 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 887, `remove_void_call`: `GetPixelInfo(clut_image,clut_map+i);`
  - base line 919, `remove_void_call`: `GetPixelInfo(image,&pixel);`
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/enhance.c): simplify ClutImage`

#### Step 12: `HaldClutImage` (line 2699)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- Only 5 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2759, `remove_void_call`: `GetPixelInfo(hald_image,&zero);`
  - base line 2815, `lt_to_le`: `area=(point.y < 0.5) ? 0.0 : 1.0;`
  - base line 2831, `lt_to_le`: `area=(point.z < 0.5)? 0.0 : 1.0;`
  - base line 2843, `ne_to_eq`: `if (((GetPixelAlphaTraits(image) & UpdatePixelTrait) != 0) &&`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/enhance.c): simplify HaldClutImage`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/enhance.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/enhance.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/enhance.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-24, Mac, overnight)

**Result: Code Health 1.53 → 2.77** (`cs-mcp` 1.5.12). Branch `campaign/M03-enhance`,
stacked on `campaign/M02-compare` (PR #8). 41 refactor commits, one recipe and one function
each, all oracle-verified; two playbook amendments (Recipe D: variables become
parameters; Recipe G: inverting a floating-point comparison).

**Three categories leave the file:** Complex Conditional (three predicates, Recipe P),
Brain Method (`LevelImage`, `LevelizeImage`, `SigmoidalContrastImage`) and Deep,
Nested Complexity (all ten functions).

| Steps | Recipe | Score |
| --- | --- | --- |
| allocation and colorspace tests as predicates (`ContrastStretchImage`, `EqualizeImage`, `LevelImageColors`) | P | 1.53 → 1.67 |
| colormap loops and parallel column loops of the levelers (`LevelImage`, `LevelizeImage`, `SigmoidalContrastImage`); macros the helpers expand move above them | E | → 2.06 |
| `NegateImage`'s two column loops and colormap; its last level a guard clause | E, G | → 2.08 |
| `GammaImage`, `GrayscaleImage` (the intensity switch returns the intensity) | E | → 2.24 |
| `ModulateImage`'s colorspace switch, twice: one `ModulateColor`; then `const ModulateInfo` for it and the nine `Modulate*` kernels | O, D, B | → 2.41 |
| `EqualizeImage`, `ContrastStretchImage`: histogram, map, colormap and pixel passes | E | → 2.59 |
| `ColorDecisionListImage`'s Slope, Offset and Power parsers into one `ParseCDLTriple` | O, D, D | → 2.65 |
| `ClipCLAHEHistogram`'s redistribution pass, the file's last Deep Nesting | E | → 2.77 |
| `ClutImage`, `ContrastImage` towards Large Method | E | flat |

Every step: build, `make.log` free of warnings for the file (checked from the
`ColorDecisionListImage` steps on, after a warning slipped through in M02; see M02's
report), guard OK, `--calls` OK or the extract/deduplicate fingerprint, the oracle on the
function's cases (0 diverged), the CodeScene gate; every moved block checked by script.

**Whole catalogue** at the branch tip against `8a4c272f1`: 15,884 cases, 1 diverged
(`gaps/c5651ffad8`, the known log-map tie).

**Tried and reverted.** `LevelizeImage`'s colormap loop into a helper like
`LevelImage`'s (the two levelers then reported as duplicates, 1.80 → 1.77);
`SigmoidalContrastImage`'s column loop before its colormap (1.95 → 1.92), and later a
value helper for its sharpen choice (flat); `ColorDecisionListImage`'s pixel loop (flat,
no path further while its two struct types are local to it); a guard clause in
`ClipCLAHEHistogram` (CodeScene counts the guard as nesting).

**What is left, at 2.77.** Code Duplication: the eight `Modulate*` kernels (they differ
in their two conversion calls), the row helpers (`LevelImageRow`, `GammaImageRow` and
the others differ in the per-channel expression), `AutoLevelImage`/`NormalizeImage`
(public). Excess Arguments: public signatures (`LevelImage`, `CLAHEImage`, ...) and the
CLAHE helpers. Large and Complex Method: `ColorDecisionListImage` (220 lines; its
`Correction` types are declared inside it, so its logging and map loops cannot be
moved without first hoisting the types, which no recipe does), `HaldClutImage` (its
pixel loop assigns and tests `status`), `CLAHEImage`, `EnhanceImage`,
`WhiteBalanceImage`. This is not yet a plateau: per-function steps of about +0.01 remain;
they were left for a later pass in favour of the files after it.
