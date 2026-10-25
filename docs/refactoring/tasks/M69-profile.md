# M69 - refactor `MagickCore/profile.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **2.12** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2997 lines |
| Functions with findings | 25 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/profile.c
```

The score must read 2.12. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 1 |
| Bumpy Road Ahead | 9 |
| Deep, Nested Complexity | 3 |
| Overall Code Complexity | file |
| Complex Method | 9 |
| Complex Conditional | 9 |
| Code Duplication | 9 |
| Large Method | 6 |
| Excess Number of Function Arguments | 7 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `SyncExifProfile` | 2323 | 78 | 5 | 6 | static | - | 53 | 104 of 114 |
| 2 | `ProfileImage` | 1001 | - | 5 | 8 | public | 2 | 87 | 56 of 68 |
| 3 | `WriteTo8BimProfile` | 1678 | 16 | 4 | 3 | static | - | 161 | 29 of 32 |
| 4 | `GetProfilesFromResourceBlock` | 1788 | 22 | - | 2 | static | - | 57 | 21 of 24 |
| 5 | `Sync8BimProfile` | 2590 | 21 | - | 2 | static | - | 44 | 32 of 34 |
| 6 | `SetImageProfileInternal` | 2026 | 14 | - | 2 | static | - | 191 | 18 of 19 |
| 7 | `PatchCorruptProfile` | 1934 | 10 | - | 2 | static | - | 191 | 16 of 16 |
| 8 | `Update8BIMClipPath` | 2942 | 19 | - | - | public | - | 216 | 20 of 20 |
| 9 | `UpdateClipPath` | 2856 | 15 | - | - | static | - | 5 | 20 of 20 |
| 10 | `GetXMPNumeratorAndDenominator` | 2725 | 11 | - | - | static | - | 9 | 14 of 20 |
| 11 | `TransformDoublePixels` | 596 | - | - | 2 | static | - | 30 | 21 of 22 |
| 12 | `GetXMPOffsets` | 2681 | - | - | - | static | - | 14 | 13 of 15 |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `SyncExifProfile` (line 2323)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2466, `ge_to_gt`: `if ((format < 0) || ((format-1) >= EXIF_NUM_FORMATS))`
- 9 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/profile.c): simplify SyncExifProfile`

#### Step 2: `ProfileImage` (line 1001)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 8 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Contains 2 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- Public function: its name and signature must not change.
- The oracle missed 7 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1069, `eq_to_ne`: `(CompareStringInfo(icc_profile,profile) == 0))`
  - base line 1076, `ne_to_eq`: `if (LocaleCompare(value,"1") != 0)`
  - base line 1077, `remove_void_call`: `SetsRGBImageProfile(image,exception);`
  - base line 1079, `ne_to_eq`: `if (LocaleCompare(value,"R98.") != 0)`
  - base line 1080, `remove_void_call`: `SetsRGBImageProfile(image,exception);`
  - base line 1177, `remove_void_call`: `SetLCMSInfoTranslate(&source_info,0.0);`
  - and 1 more (`tools/oracle/mutate.py --function ProfileImage`)
- 5 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/profile.c): simplify ProfileImage`

#### Step 3: `WriteTo8BimProfile` (line 1678)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/profile.c): simplify WriteTo8BimProfile`

#### Step 4: `GetProfilesFromResourceBlock` (line 1788)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- 3 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/profile.c): simplify GetProfilesFromResourceBlock`

### Wave 2

#### Step 5: `Sync8BimProfile` (line 2590)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/profile.c): simplify Sync8BimProfile`

#### Step 6: `SetImageProfileInternal` (line 2026)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/profile.c): simplify SetImageProfileInternal`

#### Step 7: `PatchCorruptProfile` (line 1934)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/profile.c): simplify PatchCorruptProfile`

#### Step 8: `Update8BIMClipPath` (line 2942)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/profile.c): simplify Update8BIMClipPath`

### Wave 3

#### Step 9: `UpdateClipPath` (line 2856)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- Only 5 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/profile.c): simplify UpdateClipPath`

#### Step 10: `GetXMPNumeratorAndDenominator` (line 2725)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Only 9 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.
- 6 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/profile.c): simplify GetXMPNumeratorAndDenominator`

#### Step 11: `TransformDoublePixels` (line 596)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 632, `eq_to_ne`: `if (target_info->channels == 1)`

Commit message: `refactor(MagickCore/profile.c): simplify TransformDoublePixels`

#### Step 12: `GetXMPOffsets` (line 2681)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 1 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 2696, `sub_to_add`: `if ((pos == (char *) NULL) || ((pos-datum) < 1) || (*(pos-1) != '<') ||`
- 1 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/profile.c): simplify GetXMPOffsets`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/profile.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/profile.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/profile.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-26, Mac, night)

**Result: Code Health 2.12 → 2.54** (`cs-mcp` 1.5.12). Branch `campaign/M69-profile`,
stacked on `campaign/M68-compress`. Ten refactor commits; nine are oracle-verified
(every target function is ready).
- **P (6).** `IsUnwritableDimension`, `IsExifInApp1`, `IsExifProfileName`,
  `IsInvalidResourceCount`, `IsInvalid8BimCount` and `IsMissingXMPTag`. The last was
  taken at guard level, because an unresolved survivor sits on its expression.
- **D (1).** The IPTC, ICC, EXIF and XMP resource cases now share
  `SetProfileFromResource`; they differed only in the profile name.
- **E (3).** `ReadResolutionResource` and `WriteResolutionResource` (both
  value-returning or plain on the resolution resource), and `SpliceProfileInto8Bim`
  (7 arguments, 2.44 → 2.54). WriteTo8BimProfile leaves Bumpy Road Ahead, Deep, Nested
  Complexity and Large Method.

**Process slip, fixed before push.** One build failure (the helper was placed above
WriteResourceLong's definition) was committed locally because verify.sh exits on
`BUILD FAILED` before printing `VERIFY FAILED`. It was reset at once, and verify.sh
now prints `VERIFY FAILED` on that path too.

**What is left, at 2.54.**
- **ProfileImage** (Brain Method, 492 lines) is built from
  `MAGICKCORE_QUANTUM_DEPTH`, `MAGICKCORE_HDRI_SUPPORT` and `MAGICKCORE_LCMS_DELEGATE`
  branches, so only one variant compiles here.
- **SyncExifProfile** (cc 78) `#define`s its tag constants and `MaxDirectoryStack` in
  its body, and two Complex Conditionals use them. **Observation for the hoist
  proposal:** a `#define` inside a function stays defined until the end of the file.
  A helper placed *after* the function, with a prototype before it, could therefore
  use these macros without moving them. Owner decision needed.
- **SetsRGBImageProfile** (284 lines) is mostly an embedded sRGB profile byte table.
- UpdateClipPath and Update8BIMClipPath advance `blob`/`info` and `length` together in
  every block.
