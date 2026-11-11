# W18 night session (2026-11-11 to 2026-11-12)

Batch cut from origin/main 9cfb6c9e3 after #93 (W17) merged. Four Sonnet 5.5 lanes in worktrees `~/im-w14a..d` on
`campaign/W18-a..d`; each finished lane is merged into `campaign/W18` and gated on the whole catalogue (and
`tools/ci_linux_warnings.sh --campaign-a`) before every push. Scores from cs-mcp 1.5.16. Start (sweep of the W17
tip): campaign A 84 files, mean 8.80, 35 at 10.0.

## Lane a, first round: widget.c 2.97 -> 2.99, accelerate.c 3.04 no commit (wide build)

- **widget.c:** `b73584c28` playbook (Recipe MA step 2 (c): the copies' lvalues may differ in a member of the chain
  before the last, each `v->a.m` through one shared pointer with no call or subscript; the copy only assigns it) and
  `4dbcb879b` MA, `XScreenEvent` (`XScreenTrackWidgetMapping` becomes `XScreenTrackMapping(event,&mapped)`, serving the
  popup and widget blocks; leaves Bumpy Road, cc 15 -> 13; 237 cases, 0 diverged): **2.97 -> 2.99**.
- **Held for the owner: Recipe FT (Fold an Identical Twin Function into Its Twin)**, on local branch
  `w18-ft-twin-fold` in `~/im-w14a` (commits f92852168 playbook, 0838c908c, f6a11e44a tool `twin_fold_check.py`). A
  `static` function whose parameter types and body equal another's token for token, once its parameter names are
  renamed, is removed and each call names the other. In widget.c `ReleaseTextViewDismiss` is `ReleaseDialogAction`
  under a rename; removing it takes the file from 111 to 110 functions and `XTextViewWidget` from 2.97 to 3.04
  (+0.07; with the MA step 3.06). It deletes a function definition, and AGENTS.md and the W11 brief say no recipe
  deletes code (Recipe D replaces duplicate blocks by a call, which is the nearest accepted form); the lane wrote
  it under the OK to evolve the recipes, but it was not merged. May an identical twin function be folded away?
  Other twins exist (`MoveTextViewSliderByPage` into `MoveListBrowserSliderByPage`).
- Probed, not taken: Recipe S split (`split_source.py` refuses every widget root: shared statics; `DestroyXWidget`
  because `XWidgetInfo` is defined outside the region). Stub ceilings: one widget alone is flat; the six smaller
  widgets together 3.17; the five big widgets alone 2.90; all eleven 6.84.
- **accelerate.c 3.04:** split refused for every `Accelerate*` root (shared predicates and helpers); all 16 Large
  `Compute*` functions stubbed 7.39, the five Bumpy Road functions 3.19, but their bumps hold `goto cleanup` and
  `OpenCLThrowMagickException(GetMagickModule())` blocks earlier rounds found flat to extract; peeling
  `checkAccelerateCondition` (E step 7, three helpers) leaves Complex Method but the score stays 3.04. Wrapper clones
  through the assert/trace preamble: owner question.
- Lane wide catalogue: 14,145 cases, 0 diverged (12 load-sensitive cases). Merged gate: 15,900 cases, 0 diverged.

## Lane d, first round: channel.c 9.28 -> 10.0 (merged)

02325165f FB `ChannelFxImage` (`parse` and `apply` fills as const initialisers, IDENTICAL, 9.31); a3131778e and
b231ad8b3 playbook (Recipe RV step 6 (d): five or six written locals through (c)'s one `const` struct of
addresses, the helper binding each `T *const v = name->v;` and writing `(*v)`; (e): the result may be the caller's
written local `w`, passed as `T *result`, so no `returned` variable); c69c89747 RV (d)(e) `TryFxSeparator` (the
`,`/`|`/`;` switch writes six locals and has two exits; cc 17 -> 12, 9.47; 60 cases); 7f010b4a7 N step 1 with E step
7 `ParseChannelFxTokens` (token loop plus epilogue; `channel_mask` moved into the input struct to bring the helper
under 70 lines; 10.0). The `;` success path is reached by the oracle; the separator's exit paths were checked by
reading. Recipe text Decided by `owner` (2026-11-10 OK). Narrow catalogue 15,900 cases, 0 diverged.
Plateaus (no commit): xwindow.c 1.99 (nothing new beyond the W17 pins; lone cc-9 functions worth about +0.001 each);
paint-floodfill.c 8.35 (every bump holds `PushSegmentStack`, which returns through `ThrowBinaryException`);
visual-effects-plasma.c 8.50 (a shared edge helper plus RV helpers probed at 8.15: the helpers clone each other);
resample.c 8.53 (findings sit in `FILTER_LUT`/`DEBUG_*` `#if` code); magnify.c 8.84 (about 998 of 1,000 lines);
segment.c 9.02 (the unreached `GetImageDynamicThreshold`: neither bump fits E/G/P); quantize.c 8.74 (a `KmeansImage`
seed-block helper plus a hoisted identical exit test probes at 8.81, +0.07, but needs a new "hoist an identical
tail" recipe: not taken yet). Possible upstream: `ChannelFxImage` leaks the destination list when
`AppendChannelCanvas` fails. Tool note: `refactor_guard --calls` fails a helper renamed after its first commit.

## Lane a, second round: distort.c 7.60 -> 7.70 (merged); fourier, feature-texture, layer no commit

- **distort.c:** 1741aa935 playbook (Recipe PL step 1, integer form: `e` may be an integer parameter `ssize_t n`
  whose peeled labels are decimal literals and whose `S` is `return(c);` of a numeric literal; the predicate's
  parameter is `const` of `e`'s type; a kept `switch` without `default:` stays without one; a comment on a peeled
  label moves with it), 6f05f56b1 PL `poly_basis_dx` (labels 0, 2, 5, 9, 14, 20, each only `return( 0.0 );`, into
  `IsPolyBasisDxZero(const ssize_t n)`; cc 22 -> 16, still Complex Method; script check that the predicate holds exactly
  the six labels and comments and the kept `switch` is the original minus them; 17 cases, 0 diverged). Timing (per
  pixel, twice per polynomial term): polynomial distort of 1500x1500, order 5, 11 alternating runs: median 1.305 s
  baseline, 1.314 s candidate, +0.6%; an order-2 run +0.7%. Plateau 7.70: `poly_basis_dx` keeps 15 labels; a chain
  clones `poly_basis_fn`'s chain (dx(15..19) equals `PolyBasisFn` cases 10..14) and is slower; `SetDistortNoBestFit`
  is the owner question; `DistortImage` and `SparseColorImage` are public with 6 and 5 arguments; every split is
  refused (`GenerateCoefficients` shared).
- **fourier.c 7.59:** every finding sits in the publics' `#if !defined(ENABLE_FFTW_DELEGATE)`/`#else` bodies or
  InverseFourier's `HAVE_COMPLEX_H` pairs (owner question); split_source.py refuses both families (seven shared statics).
- **feature-texture.c 7.63:** stub ceilings for `GetImageFeatures`: tail loops removed 7.98, plus the grays block 8.22,
  plus the allocation and exit blocks 8.46, emptied 9.38. Blocks: the tail loops' `shared(status)` clause names
  `status`, which a helper would never use (unused-variable under CI's `-Wextra` without OpenMP); the grays column
  loop (owner question); the exit blocks throw through `GetMagickModule()`; the six-memset run as Recipe O step 7 is
  only +0.01 and strains O step 2.
- **layer.c 8.25:** every movable block of `OptimizeLayerFrames` carries `#if DEBUG_OPT_FRAME` lines, the rest are
  `ThrowImageException` exits.
- Probes passed to lane d: colorspace-convert.c ConvertRGBToGeneric as a three-helper SD chain scores 7.78 or 8.28
  against 9.08 (Code Duplication); threshold.c B on `RangeThresholdValue`/`RangeThresholdChannels` is flat at 9.09.
- Possible upstream: `poly_basis_dy` calls `poly_basis_dx(n-1)` ("weird but true"). Gate: 15,900 cases, 0 diverged.
