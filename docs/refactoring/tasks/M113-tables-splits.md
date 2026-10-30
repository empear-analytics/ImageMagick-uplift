# M113 - cipher table, type.c attributes, color.c and string.c splits

Not a generated task file. Branch `campaign/M113`, stacked on `campaign/M112` (#76). Campaign B
on the Mac, night of 2026-10-29/30. Measured with `cs-mcp` 1.5.16.

### Report (2026-10-30, Mac, night)

**color.c 3.55 → 4.24 and string.c 3.76 → 4.57 (both out of the red), cipher.c 6.90 → 7.28,
type.c 4.70 → 4.76.**

One catalogue change, in its own commit: **Recipe F, step 2 amended** again. A static that isn't
declared `const` also qualifies when its elements are only read by subscript (none assigned,
incremented or address-taken), as with `EncipherAESBlock`'s `D[...]`.

| Step | Recipe | Evidence | Code Health |
|---|---|---|---|
| `EncipherAESBlock`: the 52-line AES table `D[]` to file scope; `obj_same_code.sh`: SAME CODE (1,725 instructions) | F | oracle, 6 cases | cipher 6.90 → 7.28 |
| `LoadTypeCache`: the attribute switch to `SetTypeInfoAttribute(type_info,keyword,token,filename,font_path)`, as `SetPolicyInfoAttribute` did in `policy.c`. It returns `type_info`, which the glyphs and metrics cases replace when a path is rejected | E | oracle, 89 cases | type 4.70 → 4.76 (the helper has five parameters) |
| `IsEquivalentImage` to the new `color-equivalent.c` (7.60) | S | oracle, 6 cases | color 3.55 → 4.24 |
| `InterpretSiPrefixValue` and `GetSiPrefixExponent` to the new `string-si.c` (8.50) | S | oracle, 15,903 cases (logcfg only) | string 3.76 → 4.20 |
| `StringToArgv` and `CountArgvArguments` to the new `string-argv.c` (9.06) | S | oracle, 56 cases | string 4.20 → 4.57 |

The splits are registered with `tools/register_source.py` (`Makefile.am`, `Makefile.in`,
`Make.com`, and `format_c_api_docs` where it lists the original). The Windows build
configuration lives outside this repository and still needs the three files added. Each split
was committed with `--no-verify`, because the gate's rule that new code must score 10.0 cannot
hold for code moved verbatim (owner, 2026-10-28). The trailers were checked by
`check_trailers.py`.

Whole catalogue at the tip: 15,906 cases, 1 diverged (`gaps/c5651ffad8`, the known logcfg
artifact).
Line coverage at the tip (the `magick` binary): `EncipherAESBlock` 6,062 calls, `SetTypeInfoAttribute` 29,857, `InterpretSiPrefixValue` 55,007 and `GetSiPrefixExponent` 18,656, `StringToArgv` and `CountArgvArguments` 321. `IsEquivalentImage` has no caller in `magick`, so the linker leaves it out and `linecov.py`, which exports only that binary, cannot see it; the 6 C-driver cases (`windrv/*`) that the oracle ran call it through `imdriver`.

**How the split candidates were found.** A scratchpad script dry-ran `split_source.py --out`
for every exported function in 14 low-scoring B files and scored both sides. Only the three
splits above raised the original by 0.2 or more and moved a real family. `ConcatenateString`
and `ConcatenateStringInfo` would also raise `string.c`, but only by removing a duplication
finding with a single small function, so they were left.

**Tried and reverted.**
- `SetUserSpecifiedFormat`'s `format_type_formats[]` to file scope: flat at 2.77, and a
  parameter of the same name in `FindExplicitFormatType` would shadow it.
- `DecodeGamma` and `EncodeGamma`'s coefficient tables to file scope with prefixed names:
  flat at 2.38, with lines over 80 columns.
- `LoadLogCache`'s attribute switch, as in `type.c`: 4.20 → 4.05. The helper picks up
  bumps 5, cc 23 and 84 lines.
- `GetPathComponent`: its format-prefix loop (3.65 → 3.53) and its subimage block (Recipe O,
  3.65 → 3.55). Both helpers inherit the nesting.
- `property.c`: `GetImageProperty` and `SetImageProperty` together, refused by
  `split_source.py` (the EXIF `#define`s lie between the functions).

**A blocker that turned out not to be one.** The oracle normalises the location in exception
messages (`@ warning/file/Function/line`) because "refactoring legitimately changes" it
(`oracle.py`, `NORMALISE`). Moving a `ThrowMagickException(...,GetMagickModule(),...)` into a
helper therefore does not change what the oracle compares. Earlier reports counted it as a
blocker; only the trace log (`LogMagickEvent`) and macros that `return` or `break` remain
real blockers.
