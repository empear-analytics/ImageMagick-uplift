# M90 - Recipe L: hoist function-local types

Not a generated task file: this task added **Recipe L - Hoist Function-Local Types**
(`playbook:` commit, proven with `tools/obj_identical.sh`) and applied it to the
functions whose blocks a local `typedef` had pinned. M35 had recorded `GetImageFeatures`
as blocked by one. The branch `campaign/M90` therefore covers more than one file. Each hoist is a commit of its own,
with an `IDENTICAL` object file, and the same branch follows it with the steps it
unblocks.

Measured with `cs-mcp` 1.5.12 (the version of the committed baseline).

### Report (2026-10-26, Mac, morning)

| File | Hoisted from | Code Health | Steps it unblocked |
|---|---|---|---|
| feature.c | GetImageFeatures (`ChannelStatistics`, renamed `FeatureStatistics`) | 1.61 → 1.81 | `IsFeatureMemoryMissing` (P), `GetFeatureGrayLevels`, `CompactFeatureGrays`, `GetFeatureAdjacencyOffset`, `GetFeatureNormalization` (E, all guard) |
| enhance.c | ColorDecisionListImage (`Correction`, `ColorCorrection`) | 2.77 → 2.84 | `ParseCDLSOPNode`, `ParseCDLSATNode`, `BuildCDLMap`, `ApplyCDLToColormap`, `ApplyCDLToPixels` (E, oracle) |
| enhance.c | HaldClutImage (`HaldInfo`) | flat at 2.84 | `GetHaldPoint`, `SetHaldPixel` (E, oracle) |
| property.c | GetEXIFProperty (`DirectoryInfo`, `TagInfo`) | flat at 2.02 | `GetEXIFTagDescription`, `GetEXIFTagByName` (E, oracle) |

Function by function:

- **GetImageFeatures**: cc 180 → 151, 24 → 18 bumps, 942 → 871 lines.
  - Only the local type's name needed step 3: it shadows the public
    `ChannelStatistics` of `statistic.h`, and 50 uses were renamed. The object file is
    still identical.
  - The function is not ready (16 of 27 killed), so every step rests on the guard. The
    oracle ran too (26 cases, 0 diverged).
- **ColorDecisionListImage**: cc 25 → 18, 5 → 2 bumps, 204 → 132 lines.
  - The two node parsers take `color_correction` by value and return it, so each
    `&color_correction.<channel>.<field>` keeps its text.
  - `ApplyCDLToPixels` is the pixel loop of a parallel row loop. Its writes are `q` and
    the loop body's own `luma` and `x`.
- **HaldClutImage**: cc 30 → 23, 151 → 131 lines.
- **GetEXIFProperty**: cc 116 → 110, 14 → 13 bumps.
  - The `static const` `EXIFTag` table stays in the function, as Recipe L requires.
  - The helpers take it as `const TagInfo *`, which is what the hoist made possible.

**Tried and reverted.**
- feature.c `CountFeatureCooccurrences`: the per-pixel body of the serial co-occurrence
  loop. It needs five arguments, and the score fell 1.81 → 1.78. The commit gate would
  refuse it anyway.

**Not hoisted.**
- profile.c `SyncExifProfile` (`DirectoryInfo`). The type is used only by the directory
  stack's pop and push.
  - The push writes five locals and `break`s out of the entry loop.
  - The pop is a three-variable Recipe O step worth one `if`.
  - Recipe L keeps a hoist only when extractions follow it, so it was left.
- type.c `GetTypeInfoByFamily` (`Fontmap`). Its one use is the substitution loop, whose
  `sizeof(fontmap)/sizeof(fontmap[0])` would change meaning if the table were passed as
  a pointer.

**What is left.**
- GetImageFeatures: its five texture-feature regions are `omp parallel for` loops.
  - Their blocks write the shared `correlation`, `mean` and `sum_squares` (refused by the
    OpenMP rule), or need six arguments (`image`, `channel_features`, `cooccurrence`,
    `x`, `y`, `i`).
  - The normalisation loop nest needs five arguments.
  - That is Recipe B territory.
- HaldClutImage: the four `status=InterpolatePixelInfo(...)` calls assign the shared
  `status` and `break`. That is neither E nor T.

**Whole catalogue on the Mac** (`oracle.py run`, baseline `9afae6d183`): 15,884 cases,
**1 diverged**: `gaps/c5651ffad8` (`driver logcfg: eight log maps in one file`), the
catalogue defect M01 recorded. Its mechanism is now confirmed.
- `LogInfoCompare` sorts by path first. Each build reads its configuration from its own
  source tree:
  - the candidate from the repository's `config/`;
  - the base from `build-oracle/src/<sha>/config/`.
- So the case's own `log.xml` sorts on opposite sides of the other file's entries. BSD
  qsort then breaks the eight-way tie differently.
- Swapping only the configuration path, with `oracle.run_case` and `source_dir_of`
  patched, swaps the outputs: the base binary with the repository's path prints the
  candidate's output, and the reverse.
- The divergence therefore depends on where the source trees live, not on the code.
  Running the base build from a source tree at the same depth would remove it.

**A process note.** `tools/oracle/build.sh` sends compiler output to
`build-oracle/cand/make.log`. A check that greps the script's own output sees no
warnings. One step (`ApplyCDLToColormap`) left `i` unused, which only
`obj_identical.sh`'s own compile showed. The fix went into that commit before the branch
was pushed. Check `make.log` for the file after each build.
