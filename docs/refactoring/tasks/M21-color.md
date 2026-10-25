# M21 - refactor `MagickCore/color.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **2.25** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2793 lines |
| Functions with findings | 14 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/color.c
```

The score must read 2.25. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 2 |
| Bumpy Road Ahead | 7 |
| Deep, Nested Complexity | 3 |
| Overall Code Complexity | file |
| Complex Method | 8 |
| Complex Conditional | 7 |
| Code Duplication | 4 |
| Large Method | 5 |
| Excess Number of Function Arguments | 3 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `QueryColorCompliance` | 2324 | 90 | 5 | 13 | public | - | 13233 | 29 of 40 |
| 2 | `LoadColorCache` | 1990 | - | 6 | 11 | static | - | 1391 | 1 of 13 |
| 3 | `IsEquivalentImage` | 1738 | - | 5 | 2 | public | - | 6 | - |
| 4 | `ConcatenateColorComponent` | 1161 | 35 | - | - | public | - | 335 | 7 of 9 |
| 5 | `GetColorCompliance` | 1010 | 13 | - | 3 | public | - | 1362 | 5 of 8 |
| 6 | `ParseCSSColor` | 2240 | 18 | - | 2 | static | - | 143 | 4 of 5 |
| 7 | `QueryColorname` | 2740 | 13 | - | 2 | public | - | 435 | 7 of 8 |
| 8 | `ListColorInfo` | 1899 | 12 | - | 2 | public | - | 3 | - |
| 9 | `GetColorTuple` | 1556 | 12 | - | - | public | - | 564 | 14 of 15 |
| 10 | `ConcatenateHexColorComponent` | 1488 | 10 | - | - | static | - | 403 | - |
| 11 | `GetColorInfoList` | 1316 | - | - | - | public | - | 3 | - |
| 12 | `IsEquivalentAlpha` | 1685 | - | - | - | public | - | 13 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `QueryColorCompliance` (line 2324)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 13 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 11 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2411, `ne_to_eq`: `if ((n != 4) && (n != 8) && (n != 16) && (n != 32) && (n != 64))`
  - base line 2427, `le_to_lt`: `if ((c >= '0') && (c <= '9'))`
  - base line 2496, `remove_void_call`: `LocaleLower(colorspace);`
  - base line 2617, `ne_to_eq`: `(color->alpha_trait != UndefinedPixelTrait))`
  - base line 2642, `div_to_mul`: `ConvertHCLToRGB(fmod(fmod(geometry_info.rho,360.0)+360.0,360.0)/`
  - base line 2642, `remove_void_call`: `ConvertHCLToRGB(fmod(fmod(geometry_info.rho,360.0)+360.0,360.0)/`
  - and 5 more (`tools/oracle/mutate.py --function QueryColorCompliance`)

Commit message: `refactor(MagickCore/color.c): simplify QueryColorCompliance`

#### Step 2: `LoadColorCache` (line 1990)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 11 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- The oracle missed 9 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2020, `ne_to_eq`: `for (q=(char *) xml; *q != '\0'; )`
  - base line 2026, `eq_to_ne`: `if (*token == '\0')`
  - base line 2047, `eq_to_ne`: `if ((*q == '"') || (*q == '\''))`
  - base line 2053, `eq_to_ne`: `if (*q == ']')`
  - base line 2100, `remove_void_call`: `GetPathComponent(filename,HeadPath,path);`
  - base line 2135, `eq_to_ne`: `(LocaleCompare(keyword,"</policy>") == 0))`
  - and 3 more (`tools/oracle/mutate.py --function LoadColorCache`)
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/color.c): simplify LoadColorCache`

#### Step 3: `IsEquivalentImage` (line 1738)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- Only 6 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/color.c): simplify IsEquivalentImage`

#### Step 4: `ConcatenateColorComponent` (line 1161)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1203, `ne_to_eq`: `if ((compliance != NoCompliance) &&`
  - base line 1251, `mul_to_div`: `GetMagickPrecision(),RadiansToDegrees(scale*QuantumScale*color));`

Commit message: `refactor(MagickCore/color.c): simplify ConcatenateColorComponent`

### Wave 2

#### Step 5: `GetColorCompliance` (line 1010)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1034, `ne_to_eq`: `for (q=colorname; *q != '\0'; q++)`
  - base line 1050, `ne_to_eq`: `if (p != (ElementInfo *) NULL)`
  - base line 1069, `remove_void_call`: `SetHeadElementInLinkedList(color_cache,p);`

Commit message: `refactor(MagickCore/color.c): simplify GetColorCompliance`

#### Step 6: `ParseCSSColor` (line 2240)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2252, `remove_void_call`: `SetGeometryInfo(geometry_info);`

Commit message: `refactor(MagickCore/color.c): simplify ParseCSSColor`

#### Step 7: `QueryColorname` (line 2740)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2761, `gt_to_ge`: `if (pixel.depth > 16)`

Commit message: `refactor(MagickCore/color.c): simplify QueryColorname`

#### Step 8: `ListColorInfo` (line 1899)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 3 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/color.c): simplify ListColorInfo`

### Wave 3

#### Step 9: `GetColorTuple` (line 1556)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1586, `ne_to_eq`: `if ((color.depth > 8) && (IsSVGCompliant(pixel) != MagickFalse))`

Commit message: `refactor(MagickCore/color.c): simplify GetColorTuple`

#### Step 10: `ConcatenateHexColorComponent` (line 1488)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/color.c): simplify ConcatenateHexColorComponent`

#### Step 11: `GetColorInfoList` (line 1316)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.
- Only 3 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/color.c): simplify GetColorInfoList`

#### Step 12: `IsEquivalentAlpha` (line 1685)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/color.c): simplify IsEquivalentAlpha`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/color.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/color.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/color.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, overnight)

**Result: Code Health 2.25 → 3.16** (`cs-mcp` 1.5.12). Branch `campaign/M21-color`,
stacked on `campaign/M20-composite` (PR #26). 16 refactor commits and one playbook
commit, one function per commit:

| Function | Commits | Recipes | Evidence |
| --- | ---: | --- | --- |
| `QueryColorCompliance` | 9 | 1 P (three predicates), 8 E | guard |
| `ConcatenateColorComponent` | 1 | P (two predicates) | guard |
| `QueryColorname` | 1 | P | oracle |
| `LoadColorCache` | 5 | 1 P, 4 E | guard |

**Gone from the file:** Complex Conditional (all seven sites) and Code Duplication. The
four duplication findings (`GetColorInfoList`, `GetColorList`, `IsEquivalentAlpha`,
`IsEquivalentIntensity`) stopped being reported during the campaign. None of those
functions was touched, so CodeScene's clone detection must have re-weighed the file.

**QueryColorCompliance: cc 90 → 32, 13 bumps → 3, 342 → 136 lines.**
- Predicates: `IsHexColorLength`, `IsInvalidHexAlphaLength`, `IsHueColorspaceName`.
- Branches of the colour-component chain: `SetOklchColorFromGeometry`,
  `SetLabColorFromGeometry`, `SetScaledColorFromGeometry`.
- `SetPsiColorFromGeometry`, `SetGrayColorFromGeometry`, `SetHueColorFromGeometry`.
  The hue block's writes to `scale` and `geometry_info` are dead, because a `return`
  follows directly.
- Two value-returning blocks: the colour-name fallback (`QueryColorFromCSSName`) and the
  whole `rgb(...)` form (`QueryColorFunction`).

**Playbook amendment** (its own commit, before its first use): Recipe E step 7 now also
covers a block of a value-returning function whose every path ends in `return(<expr>);`.
The block becomes `return(<call>);`.

**LoadColorCache: bumps 11 → 9, 189 → 129 lines.**
- `IsIncludeElementOpen`.
- `SetColorInfoCompliance`, `GetIncludePath` (+0.15 on its own) and
  `SetColorInfoAttribute` (the whole attribute `switch`).
- `AcquireColorInfo` left the score flat (kept under rule 2: Large Method 133 → 129).
- The include condition appears verbatim in six other config loaders (configure,
  delegate, locale, log, policy, type). Their tasks can reuse the same steps.

**Whole catalogue** at the branch tip against `8a4c272f1`: 15,884 cases, 1 diverged
(`gaps/c5651ffad8`, the known log-map tie).

**Flakes seen during the task** (neither caused by a change):
- `resize/6ee45e9cfa` ("rose half write-mask -scale 50%") diverged in 6 of 12 runs after
  one build, 0 of 20 after a rebuild of the same source, and 0 of 12 at the previous
  commit. The baseline is deterministic on it. It looks like an environment flake on the
  Mac and is worth watching.
- The two hough "matrix on disk" cases (`matrix/f2913328b4`, `matrix/e1095950b7`, temp
  names) diverged once. They gave 0 of 2 in three reruns of the same build.

**Process note: readiness from one report.** During the task I first ran the readiness
gate on a single color.c report (`mutation-erdc-sdl-color.json`, 47 mutants), not on
every report merged (147 mutants). The task file's own table already used the merged
figures. Several commit messages therefore stated wrong mutant counts:
- `QueryColorCompliance` was given as "not ready, 7 mutants, 1 killed" (merged:
  *careful*, 37 of 43).
- `ConcatenateColorComponent` was described as having no mutant (merged: *careful*, 7 of
  9, with an unmatched survivor on the moved percentage test).
- `QueryColorname` was given as "ready, 3 of 3" (merged: *careful*, 7 of 8; the survivor
  is outside the moved expression, so oracle level holds).
- `LoadColorCache`'s counts were also wrong.

Every message was corrected before the push (message-only rewrite, tree unchanged). No
evidence level changed: every guard-level claim stands on the guard, and the one
oracle-level claim satisfies the careful verdict. The gate now always runs over every
report.

**What is left, at 3.16.**
- `QueryColorCompliance`'s hex branch (cc 21 if extracted). Moving it costs 0.01: the
  helper keeps four findings, and its two digit loops each write the pixel and the
  cursor, so E cannot split them.
- The `device-` and `icc-color` blocks in `QueryColorFunction` write three locals. That
  needs Recipe O, which needs the oracle level the function does not have.
- `LoadColorCache`'s DOCTYPE skip loop. Moving it costs 0.04: the if-else ladder takes
  its nesting into the helper.
- Its include loop writes `q`, `keyword`, `token` and `status`.
- `IsEquivalentImage`, `ListColorInfo`, `GetColorInfoList` and `GetColorList`: no
  oracle case executes them.
- The 5-argument functions are public, or (`LoadColorCache`) recursive with every
  argument live.
