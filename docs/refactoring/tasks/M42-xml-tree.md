# M42 - refactor `MagickCore/xml-tree.c`

| Field | Value |
| --- | --- |
| Baseline Code Health | **1.61** / 10 |
| Exit target | **plateau** - no legal recipe raises the score further; record it |
| Hand-off point | **4.00** leaves the Red band. A small model stops there and hands on |
| File size | 2348 lines |
| Functions with findings | 25 |
| Measured with | `cs-mcp` 1.5.12, `cs` 1.0.46 |

## 1. Read these first

- [`../../../AGENTS.md`](../../../AGENTS.md) - the contract, including the verification sequence.
- [`../PLAYBOOK.md`](../PLAYBOOK.md) - the only transformations you may apply, and the rules for OpenMP regions and public functions.
- [`../README.md`](../README.md) - campaign rules and the definition of done.

## 2. Record the baseline before touching anything

```bash
python3 tools/ch.py --review MagickCore/xml-tree.c
```

The score must read 1.61. If it does not, this file changed after the task was written: stop and report that instead of proceeding.

## 3. What CodeScene flags here

| Smell | Functions |
| --- | ---: |
| Lines of Code in a Single File | file |
| Bumpy Road Ahead | 12 |
| Deep, Nested Complexity | 4 |
| Overall Code Complexity | file |
| Complex Method | 14 |
| Complex Conditional | 20 |
| Code Duplication | 8 |
| Large Method | 7 |
| Excess Number of Function Arguments | 3 |

## 4. Target functions, highest leverage first

| # | Function | Line | Cyclomatic | Nesting | Bumps | Linkage | OpenMP | Cases | Mutants killed |
| --- | --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | --- |
| 1 | `NewXMLTree` | 1607 | 107 | 7 | 15 | public | - | 150 | 7 of 11 |
| 2 | `ParseInternalDoctype` | 1344 | 58 | 5 | 8 | static | - | 91 | 2 of 5 |
| 3 | `ParseEntities` | 1028 | 49 | 6 | 8 | static | - | 135 | 1 of 6 |
| 4 | `XMLTreeInfoToXML` | 2247 | 23 | 5 | 6 | public | - | 34 | - |
| 5 | `XMLTreeTagToXML` | 2142 | 26 | - | 6 | static | - | 31 | - |
| 6 | `FileToXML` | 574 | 23 | - | 3 | public | - | 1504 | 0 of 2 |
| 7 | `InsertTagIntoXMLTree` | 168 | 16 | - | 3 | static | - | 124 | - |
| 8 | `DestroyXMLTreeRoot` | 464 | 15 | - | 3 | static | - | 142 | 1 of 1 |
| 9 | `ConvertUTF16ToUTF8` | 949 | 18 | - | 2 | static | - | 150 | 0 of 2 |
| 10 | `ParseProcessingInstructions` | 1271 | 15 | - | 2 | static | - | 109 | 0 of 5 |
| 11 | `AddPathToXMLTree` | 326 | 11 | - | 2 | public | - | 3 | - |
| 12 | `CanonicalXMLContent` | 284 | 9 | - | 2 | static | - | 31 | - |

Thresholds for C: cyclomatic complexity under 9, nesting depth under 4. **Cases** is how many oracle cases execute the function; **Mutants killed** counts the sampled mutants on lines the oracle executes. A function with no cases cannot be checked and is listed last: skip it and report it.

## 5. Steps

Work **one function at a time, in the order above**, and run the verification in section 6 after each commit. Re-run the review at the start of each wave: line numbers shift as earlier waves land.

### Wave 1 - start here

#### Step 1: `NewXMLTree` (line 1607)

- **Recipe G (guard clauses)** - nesting is 7, target is under 4.
- **Recipe E (extract function)** - 15 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- The oracle missed 4 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1788, `ne_to_eq`: `if (((*p != '\0') && (*p != '>')) ||`
  - base line 1809, `eq_to_ne`: `if ((*p == '>') || ((*p == '\0') && (terminal == '>')))`
  - base line 1888, `remove_void_call`: `ParseCharacterContent(root,tag+8,(size_t) (p-tag-10),'c');`
  - base line 1951, `remove_void_call`: `ParseProcessingInstructions(root,tag+1,(size_t) (p-tag-2));`

Commit message: `refactor(MagickCore/xml-tree.c): simplify NewXMLTree`

#### Step 2: `ParseInternalDoctype` (line 1344)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 8 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 3 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1398, `eq_to_ne`: `entities=(*c == '%') ? predefined_entities : root->entities;`
  - base line 1554, `sub_to_add`: `ParseProcessingInstructions(root,c,(size_t) (xml-c));`
  - base line 1554, `remove_void_call`: `ParseProcessingInstructions(root,c,(size_t) (xml-c));`

Commit message: `refactor(MagickCore/xml-tree.c): simplify ParseInternalDoctype`

#### Step 3: `ParseEntities` (line 1028)

- **Recipe G (guard clauses)** - nesting is 6, target is under 4.
- **Recipe E (extract function)** - 8 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 5 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1091, `lt_to_le`: `if (c < 0x80)`
  - base line 1102, `sub_to_add`: `*xml=(char) ((0xFF << (7-i)) | (c >> (6*i)));`
  - base line 1102, `mul_to_div`: `*xml=(char) ((0xFF << (7-i)) | (c >> (6*i)));`
  - base line 1114, `eq_to_ne`: `if (((*xml == '&') && ((state == '&') || (state == ' ') ||`
  - base line 1191, `ne_to_eq`: `while ((*xml != '\0') && (*xml != ' '))`

Commit message: `refactor(MagickCore/xml-tree.c): simplify ParseEntities`

#### Step 4: `XMLTreeInfoToXML` (line 2247)

- **Recipe G (guard clauses)** - nesting is 5, target is under 4.
- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.

Commit message: `refactor(MagickCore/xml-tree.c): simplify XMLTreeInfoToXML`

### Wave 2

#### Step 5: `XMLTreeTagToXML` (line 2142)

- **Recipe E (extract function)** - 6 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- No recipe in the catalogue reaches *Excess Number of Function Arguments* (6 arguments).

Commit message: `refactor(MagickCore/xml-tree.c): simplify XMLTreeTagToXML`

#### Step 6: `FileToXML` (line 574)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/xml-tree.c): simplify FileToXML`

#### Step 7: `InsertTagIntoXMLTree` (line 168)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/xml-tree.c): simplify InsertTagIntoXMLTree`

#### Step 8: `DestroyXMLTreeRoot` (line 464)

- **Recipe E (extract function)** - 3 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/xml-tree.c): simplify DestroyXMLTreeRoot`

### Wave 3

#### Step 9: `ConvertUTF16ToUTF8` (line 949)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- 2 further survivor(s) are of kinds the oracle cannot see by design (logging, loop bounds over padding, allocation sizes, or read by hand and found equivalent; see `tools/oracle/classify.py`).

Commit message: `refactor(MagickCore/xml-tree.c): simplify ConvertUTF16ToUTF8`

#### Step 10: `ParseProcessingInstructions` (line 1271)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- The oracle missed 5 sampled mutant(s) here that are not of a known harmless kind. Take extra care on these lines, and consider closing the gap first:
  - base line 1318, `add_to_sub`: `root->processing_instructions[i+1]=(char **) NULL;`
  - base line 1329, `add_to_sub`: `root->processing_instructions[i],(size_t) (j+3),`
  - base line 1331, `eq_to_ne`: `if (root->processing_instructions[i] == (char **) NULL)`
  - base line 1338, `add_to_sub`: `(void) CopyMagickString(root->processing_instructions[i][j+2]+j-1,`
  - base line 1341, `add_to_sub`: `root->processing_instructions[i][j+1]=(char *) NULL;`

Commit message: `refactor(MagickCore/xml-tree.c): simplify ParseProcessingInstructions`

#### Step 11: `AddPathToXMLTree` (line 326)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.
- Public function: its name and signature must not change.
- Only 3 oracle case(s) execute this function, so the oracle sees little of it. Consider adding cases before a structural change.

Commit message: `refactor(MagickCore/xml-tree.c): simplify AddPathToXMLTree`

#### Step 12: `CanonicalXMLContent` (line 284)

- **Recipe E (extract function)** - 2 nested blocks; each bump is a missing function.
- **Recipe P (named predicate)** - move compound conditions into named `MagickBooleanType` helpers.

Commit message: `refactor(MagickCore/xml-tree.c): simplify CanonicalXMLContent`

## 6. Verification after every commit

```bash
tools/oracle/build.sh cand
python3 tools/refactor_guard.py MagickCore/xml-tree.c   # --calls after E or S
python3 tools/oracle/oracle.py run --function <Function>
python3 tools/ch.py --review MagickCore/xml-tree.c
```

Name the function you refactored in the oracle run, not a helper you extracted from it. If the guard fails or the oracle diverges, `git checkout -- MagickCore/xml-tree.c` and stop.

Before the branch is pushed: `python3 tools/oracle/oracle.py run`.

## 7. Report

For each function: the recipes applied, the score before and after, and the review lines that changed. For the file: where it plateaued, and which smells are left.
