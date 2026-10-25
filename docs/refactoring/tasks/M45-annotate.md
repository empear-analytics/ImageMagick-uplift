# M45 - refactor `MagickCore/annotate.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **2.06** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2508 lines |
| Functions with findings | 15 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/annotate.c
```

The score must read 2.06. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Brain Method | 2 |
| Bumpy Road Ahead | 5 |
| Deep, Nested Complexity | 3 |
| Overall Code Complexity | file |
| Complex Method | 7 |
| Complex Conditional | 9 |
| Code Duplication | 6 |
| Large Method | 7 |
| Excess Number of Function Arguments | 8 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `RenderFreetype` | 1518 | - | 6 | 14 | static | 1 | 198 | 136 of 187 |
| 2 | `AnnotateImage` | 230 | 61 | - | 3 | public | - | 177 | 170 of 170 |
| 3 | `RenderType` | 991 | - | 4 | 5 | static | - | 218 | 20 of 26 |
| 4 | `FormatMagickCaption` | 639 | 19 | - | 4 | public | - | 11 | 26 of 26 |
| 5 | `ComplexTextLayout` | 1274 | - | 4 | - | static | - | 180 | 6 of 11 |
| 6 | `GetMultilineTypeMetrics` | 771 | 10 | - | - | public | - | 51 | 14 of 18 |
| 7 | `TraceLineTo` | 1384 | - | - | - | static | - | 15 | 4 of 4 |
| 8 | `TraceMoveTo` | 1399 | - | - | - | static | - | 15 | 4 of 4 |
| 9 | `TraceQuadraticBezier` | 1414 | - | - | - | static | - | 15 | 8 of 8 |
| 10 | `RenderX11` | 2495 | - | - | - | static | - | 20 | - |
| 11 | `RenderPostscript` | 2221 | 32 | - | 4 | static | 1 | 0 | - |
| 12 | `IsValidPostscriptFontname` | 2205 | 9 | - | - | static | - | 0 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `RenderFreetype` (line 1518)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 14 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- Contains 1 OpenMP pragma(s): only blocks inside a loop body that write no variable declared outside it may be extracted. Never move or edit a pragma.
- The oracle missed 36 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1645, `ne_to_eq`: `if ((draw_info->font != (char *) NULL) && (*draw_info->font == '@') &&`
  - base line 1645, `eq_to_ne`: `if ((draw_info->font != (char *) NULL) && (*draw_info->font == '@') &&`
  - base line 1646, `eq_to_ne`: `(IsRightsAuthorized(PathPolicyDomain,ReadPolicyRights,draw_info->font) == Magick`
  - base line 1709, `ne_to_eq`: `if ((draw_info->metrics != (char *) NULL) &&`
  - base line 1710, `ne_to_eq`: `(IsPathAccessible(draw_info->metrics) != MagickFalse))`
  - base line 1714, `ne_to_eq`: `if ((ft_status != 0) && (face->num_charmaps != 0))`
  - and 30 more (`tools/oracle/mutate.py --function RenderFreetype`)
- 15 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/annotate.c): simplify RenderFreetype`

#### Step 2: `AnnotateImage` (line 230)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/annotate.c): simplify AnnotateImage`

#### Step 3: `RenderType` (line 991)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- **Recipe E (extract function)** - 5 nested blocks; each bump is a missing function.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1056, `ne_to_eq`: `if ((type_info != (const TypeInfo *) NULL) &&`
  - base line 1060, `lt_to_ge`: `for (i=0; i < (ssize_t) number_families; i++)`
  - base line 1069, `ne_to_eq`: `if (font != (char *) NULL)`
  - base line 1109, `ne_to_eq`: `if (type_info->metrics != (char *) NULL)`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/annotate.c): simplify RenderType`

#### Step 4: `FormatMagickCaption` (line 639)

- **Recipe E (extract function)** - 4 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments*; the function is public, and its signature is the API.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/annotate.c): simplify FormatMagickCaption`

### Wave 2

#### Step 5: `ComplexTextLayout` (line 1274)

- **Recipe G (guard clauses)** - nesting is 4, target is under 4.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).
- The oracle missed 5 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1301, `ne_to_eq`: `if (((*grapheme)[i].index != 0) && (last_glyph != 0))`
  - base line 1301, `ne_to_eq`: `if (((*grapheme)[i].index != 0) && (last_glyph != 0))`
  - base line 1313, `eq_to_ne`: `if (ft_status == 0)`
  - base line 1314, `sub_to_add`: `(*grapheme)[i-1].x_advance+=(FT_Pos) ((draw_info->direction ==`
  - base line 1314, `eq_to_ne`: `(*grapheme)[i-1].x_advance+=(FT_Pos) ((draw_info->direction ==`

Commit message: `refactor(MagickCore/annotate.c): simplify ComplexTextLayout`

#### Step 6: `GetMultilineTypeMetrics` (line 771)

- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- **Recipe E (extract function)** - a large method; extract its self-contained blocks.
- Public function: its name and signature must not change.
- The oracle missed 2 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 849, `gt_to_ge`: `if (extent.width > metrics->width)`
  - base line 867, `lt_to_ge`: `for (i=0; i < (ssize_t) count; i++)`
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/annotate.c): simplify GetMultilineTypeMetrics`

#### Step 7: `TraceLineTo` (line 1384)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/annotate.c): simplify TraceLineTo`

#### Step 8: `TraceMoveTo` (line 1399)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/annotate.c): simplify TraceMoveTo`

### Wave 3

#### Step 9: `TraceQuadraticBezier` (line 1414)

- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/annotate.c): simplify TraceQuadraticBezier`

#### Step 10: `RenderX11` (line 2495)

- No recipe in the catalogue reaches *Excess Number of Function Arguments* (5 arguments).
- **Recipe D (deduplicate)** - duplication: only if the blocks differ in one value.

Commit message: `refactor(MagickCore/annotate.c): simplify RenderX11`

#### Step 11: `RenderPostscript` (line 2221)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

#### Step 12: `IsValidPostscriptFontname` (line 2205)

- **No oracle case executes this function. Do not change it.** Report it so cases can be added.

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/annotate.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/annotate.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/annotate.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.

### Report (2026-10-25, Mac, evening)

**Result: Code Health 2.06 → 2.33** (`cs-mcp` 1.5.12). Branch `campaign/M45-annotate`,
stacked on `campaign/M44-blob`. 4 refactor commits, all Recipe P, adding 6 predicates.
- Oracle-verified: RenderFreetype (`IsUnauthorizedFontFile`, `IsTextWithoutGlyphs`) and
  FormatMagickCaption (`IsBreakableSpace`).
- Guard-verified: RenderFreetype's two stroke tests (`IsRenderedStroke`,
  `IsVisibleStroke`), which have survivors on their lines, and IsValidPostscriptFontname
  (`IsPostscriptNameChar`, no case).
- The first try at `IsPostscriptNameChar` declared its parameter `const char *`. The
  build warned (the caller's `p` is `const unsigned char *`), so it was reverted and
  redone with the right type.

**Not done:**
- `ContainsRaqmParagraphSeparator`: it sits inside `#if defined(MAGICKCORE_RAQM_DELEGATE)`,
  which this build does not compile, so it is untouchable.
- RenderFreetype's interword-spacing test: it would need five arguments, including a
  loop-local cursor.

**What is left, at 2.33.**
- `RenderFreetype` (the glyph loop: bumpy, nested, with shared FreeType state).
- `AnnotateImage` (multi-line layout with gravity).
- `RenderPostscript` (no oracle case: Ghostscript is not in the corpus).
