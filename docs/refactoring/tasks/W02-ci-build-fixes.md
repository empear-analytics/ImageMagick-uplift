# W02 - ImageMagick's own CI build in campaign A's files

Not a generated task file. The first task of campaign A (Windows desktop, WSL2 Ubuntu
24.04), as [CAMPAIGNS.md](../CAMPAIGNS.md) sets out. Branch `campaign/W02`, from `main`
at `c74bb179e` (#59), also the oracle's baseline. Measured with `cs-mcp` 1.5.16 (the
owner moved both machines to it on 2026-10-28; CAMPAIGNS.md still names 1.5.13, under
which `fx.c`, `shear.c`, `transform.c` and `vision.c` scored the same).

### Setup on this machine

- `oracle.py selfcheck`: 15,900 cases, 2 runs each, 0 nondeterministic; with `WIDE=1`,
  14,145 cases, 0 nondeterministic.
- `casemap.py`: 5,349 functions reached (wide: 5,058), 0 cases without a profile.
  `linecov.py` rebuilt `oracle.lcov` and `oracle-wide.lcov`.
- The CodeScene MCP server on Windows reads the WSL clone through
  `\\wsl.localhost\...`, and its scores match `tools/ch.py` inside WSL.
  `pre_commit_code_health_safeguard` cannot run there (Windows git refuses the WSL path,
  "dubious ownership"); `tools/codescene_precommit.py`, which the commit hook runs
  inside WSL, stands in for it, as AGENTS.md allows.

### Report

`ci_warnings.sh` is clean on `vision.c` and `transform.c`, and on `shear.c` except
`ShearRotateImage`, which is left (below). Across every MagickCore file the normal build
compiles, it now reports only that site and campaign B's `quantum-import.c`.

| File | Function | Recipe | Cases | Code Health |
| --- | --- | --- | ---: | --- |
| vision.c | ConnectedComponentsImage: one `CCNeighborDirection`, eight `CCThresholdRange` | B (step 5) | 59 | 2.66 → 2.66 |
| shear.c | ShearImage: `ShearIntegralImage` (sizes, borders, shears x then y) | E | 29 | 6.46 → 6.52 |
| shear.c | ShearImage: two `ShearRegion`, one `CropToFitRegion` | B (step 5) | 29 | 6.52 → 6.52 |
| transform.c | RollImage: `WrapRollOffset` (the x and y wrap loops) | D | 26 | 2.97 → 2.97 |
| transform.c | RollImage: four `CopyRegion` | B (step 5) | 26 | 2.97 → 2.97 |

Every struct here takes its values from member accesses or casts, or after statements
that write its operands, so each is declared plain and filled member by member, directly
before its call, as step 5 says. Filling costs lines. In a function that is already a
*Large Method*, or that the fill pushes past 70 lines, it lowers the score, and the
pre-commit gate refuses it:

- **ShearImage** (80 lines): the fill alone, 6.46 → 6.42. Recipe E first moved the
  sizing, border and both passes into `ShearIntegralImage`. Every exit of that block
  returns `(Image *) NULL`, one of them through `ThrowImageException`, so the helper
  returns an image and the caller returns `(Image *) NULL` on it (step 6). A narrower cut
  with five parameters added *Excess Number of Function Arguments* (6.29).
- **RollImage** (about 55 lines): the fill alone, 2.97 → 2.88. Recipe D first folded its
  two wrap loops into one helper; that is flat, and is kept under rule 2 as the step that
  makes room. All four loop bodies are covered (line coverage 20, 27, 14, 11).

The whole catalogue: 15,900 cases, 0 diverged.

### Left: ShearRotateImage, for the owner

`ShearRotateImage` holds the last site in `shear.c` (three `ShearRegion` and a
`CropToFitRegion`, 949:21). The fill alone costs 0.05 (6.52 → 6.47); the function is a
*Large Method* of 100 lines. Making room the way ShearImage got it does not work here:

- `shear.c` has seven functions. An eighth, any eighth, crosses a file-level cliff. The
  file gains *Overall Code Complexity*, *Overall Function Size* (median 65 lines) and
  *Code Duplication* between `XShearImage` and `YShearImage`, and loses about a point:
  5.62 with the function's tail moved into a helper (Recipe E step 7), 5.48 with only
  its bounds computation moved.
- No recipe shortens the function without a new function.

So the step needs the owner's decision. Either accept a 0.05 drop for the CI fix (the
gate would need `--no-verify`, which AGENTS.md forbids without that), or wait until
campaign work on `XShearImage` and `YShearImage` carries `shear.c` past the cliff. Rule 1's
exception of 2026-10-28 (several functions, one recipe, one commit) is the likely way to
do that.

### For campaign B

The `Main` run on `c74bb179e` fails for two reasons, both in campaign B's files:

- **macOS arm64:** `quantum-import.c:4535:26`, the declaration after a statement that
  M102 left for B.
- **Windows x64, x86 and arm64 (MSVC):** `annotate.c` lines 1103, 1114 and 1156, C4456
  "declaration of 'freetype' hides previous local declaration", an error under `/WX`.
  `ci_warnings.sh` does not catch it; clang's counterpart is `-Wshadow`, which
  ImageMagick's own sources may not be clean under.

No MSVC diagnostic in that run is in a campaign A file.
