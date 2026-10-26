# M91 - Recipe L, second round, and what it left

Not a generated task file. M90 added Recipe L. This task went through the other
function-local `typedef`s in MagickCore and took the steps they allow, plus the
plain extractions found on the way. Branch `campaign/M91`, stacked on `campaign/M90`
(PR #43).

Measured with `cs-mcp` 1.5.12.

### Report (2026-10-26, Mac, late morning)

| File | Function | Recipe | Code Health |
|---|---|---|---|
| type.c | GetTypeInfoByFamily | E `GetTypeScore`, E `GetExactTypeInfo` (oracle, careful) | 4.13 → 4.21 |
| compress.c | LZWEncodeImage | L (`TableType`), E `ResetLZWTable` (oracle, careful) | 5.28 → 5.31 |
| draw.c | DrawPolygonPrimitive | E `GetPolygonEdgesExtent`, E `ClampPolygonBounds` (guard: not ready, 68 of 114; oracle ran too, 362 cases) | 1.77 → 1.79 |

DrawPolygonPrimitive goes from cc 46 to 33. Its local `ExtentInfo` was not hoisted: the
only block a hoist frees is the four-line `poly_extent` computation, and the pixel loops
that read `poly_extent` need six or more other locals.

**TraceStrokePolygon: Recipe O refused on fresh data.**
- The `.p` and `.q` slope selections are twins that each set `slope` and
  `inverse_slope`. That is Recipe O once, then D for the second copy.
- The draw.c reports predate tonight's edits, and their survivors mapped to unrelated
  text. So TraceStrokePolygon was mutated again on the Mac: Mull build `mull-draw91`,
  report `mutation-draw91.json`, 262 mutants, careful (82% adjusted).
- The fresh report shows real survivors on the slope lines: `cxx_lt_to_le` and
  `cxx_lt_to_ge` on `dy.p < 0.0`, `dx.p < 0.0` and their `.q` copies.
- Recipe O requires no unmatched survivor on the block's lines, so the step was not
  taken.
- **Open question: gap or equivalent?** A `<` to `<=` on `dy.p < 0.0` differs only when
  `dy.p == 0`. That test runs only when `|dx.p| < MagickEpsilon`, and the zero-length
  check before it mostly rules out that pair. The exception is the round-cap fallback,
  where `n` is set to the last vertex.
  - If no input reaches it, the survivors are equivalent and belong in `verdicts.json`.
    Then the gate would let Recipe O through.
  - If an input does reach it, a case with a degenerate round-capped path would kill
    them.
  - Either way it needs a reading, or a probe case, before the step.

**TracePath** (ready, cc 167) stays as M77 found it. Every token read is followed by
`ThrowPointExpectedException`, a macro that throws, so none of its argument blocks can
move.

**Tried and reverted.**
- type.c `GetBestTypeInfo`, the same-family scoring pass. It assigns `type_info` only
  when a font beats `max_score`, so a miss must leave the caller's value. That takes a
  fifth parameter, and the score fell 4.21 → 4.20.

**Surveyed, nothing taken.**

| Function | Local type | Why not |
|---|---|---|
| compare.c SIMMaximaImage, SIMMinimaImage | `MaximaInfo`, `MinimaInfo` | Inside `#if defined(MAGICKCORE_HDRI_SUPPORT) && defined(MAGICKCORE_FFTW_DELEGATE)` (compare.c 2946–4626). FFTW is off in this build, so the code is not compiled here. |
| compare.c SimilarityImage | `SimilarityInfo` | Its "best so far" switch would need five parameters. The function has no mutation data at all (the September merged report predates it), so it would be guard-only. **Gap:** mutate SimilarityImage (10 cases). |
| annotate.c RenderFreetype | `GlyphInfo` | The glyph blocks are interleaved with `ThrowFreetypeErrorException`, a local macro that throws (`GetMagickModule` records `__func__`). |
| shear.c XShearImage, YShearImage | `ShearDirection` (enum) | The enum only drives the switch. Each case body reads about eight of the parallel loop's locals. |
| statistic.c GetImageRange | `RangeInfo` | Already refactored. Moving the row loop leaves cc at 9, still flagged; the critical section stays. |
| splay-tree.c IterateOverSplayTree | enum | No finding on the function. |
| profile.c SyncExifProfile, type.c GetTypeInfoByFamily (`Fontmap`) | | See [M90](M90-hoist-types.md). |
