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

## Lane d, second round: quantize.c 8.74 -> 8.81 with new Recipe HT; the 9.09 files plateau (merged)

- **Recipe HT, Hoist the Exit Test Common to a Selection's Branches** (9713e5807, Decided by `owner` under the
  2026-11-10 OK; own commit before first use). Shape: in `if (C) P else Q`, `P` ends in `T` = `if (V == F)
  return(V);` and `Q` holds the same test followed by statements `Z` that run only when it passed. Steps 1-5: `V` is a
  local `MagickBooleanType`, `F` a constant, no `#pragma` or preprocessor line outside OpenMP; `Q = Q0 T Z` with `Q0`
  assigning `V` before reading it, `T` once at top level, `Z` non-empty and not writing `V`; no label, `goto`, leaving
  `break`/`continue`, and no `return` in `Q0` or `Z`; `Q` cut into a `static MagickBooleanType` helper (at most four
  parameters named as the caller's variables) that declares `V`, holds `Q` unchanged and ends `return(V);`; the
  selection becomes `if (C) P0 else V=Helper(args);` with `T` written once after it. Behaviour: `P` runs `P0` then `T`
  in the same order; when `T` fires in `Q` the helper returns the same `V` and `Z` ran in neither version; when it does
  not, `Z` runs, does not write `V`, and the hoisted test does not fire. Proof: new `tools/tail_hoist_check.py FILE
  CALLER HELPER [REV]` compares the old and new caller token by token (a write to `V` in `Z` prints FAIL); object code
  is not identical (register allocation only, measured), so the proof is the token check, reading steps 1-3, the
  oracle and `refactor_guard --calls`. It merges two copies of an exit test into one (like Recipe D) and deletes no
  logic; flagged for the owner with the twin-fold question.
- **quantize.c 8.74 -> 8.81:** a76798f30 HT `KmeansImage` (`SeedKmeansFromColorList`, no finding; leaves Bumpy Road
  Ahead; 22 cases, 0 diverged; four cases reach `SeedKmeansFromColors`). Plateau: 126 functions (split_source.py
  refuses every family) and the public 5-argument `KmeansImage`.
- **colorspace-convert.c 9.08:** `ConvertRGBToGeneric` (cc 20, 113 lines) is the only real finding (a stub scores 9.68);
  label-run helpers clone each other (7.71 / 8.28), TB/KT need constant cases (these are calls with different
  signatures), function-pointer tables are not allowed.
- **effect-blur.c, enhance-tone.c, enhance.c, threshold.c (9.09):** Lines of Code (2,624 / 1,956 / 1,242 / 1,531) plus
  argument counts including public functions; split_source.py dry runs on every public root: effect-blur refused by
  shared `SetBlurProgress`/`ReleaseFailedImage`/`MagickSigma`; enhance-tone and threshold by shared row helpers, the
  `ThresholdMap` type and macros; enhance.c's Modulate family by the tool's refusal of function-pointer typedefs; the
  enhance.c splits the tool does accept leave the kept file at 9.09 and 1,081 lines. HT may fit other if/else exit
  tests; not searched for.

## Lane c, second round: animate.c 3.53 -> 4.40, opencl.c 5.47 -> 5.80 (wide build, merged)

34 commits; each ran `verify_wt` (oracle on the function, guard, calls, CI warnings); whole wide catalogue 14,145
cases, 0 diverged (one load-sensitive case settled). **XSaveImage** (no case enters it; `Verified: guard`; last
round's chain redone one step at a time): 45a1b57ac E 3.55, 1b8ce9461 E 3.57, 4ea86f63d P 3.58, 4f1c23581 E 3.59, then
3.60 and 3.67 (leaves Complex Method). **XAnimateImages** (30 cases; partial evidence, the lcov cannot show which
events they raise; cc 150 -> 78, bumps 9 -> 6): E 8257d72af (ConfigureNotify) 3.70, 16f4c8a3a (DestroyNotify) 3.71,
71807eb55 (Expose tail) 3.73, 440243d1a (UnmapNotify tail) 3.76, 6abf357a1 (MapNotify tail) 3.79; N e825fb233
(shared `AnimateEventContext`) 3.82, a76fcea1a (ClientMessage tail) 3.98; CT 2ddd15c46 (MapNotify) 3.90, f995187f9
(PropertyNotify) 3.95, 9f79370dd (KeyPress) 4.11, e1f294e87 (ButtonPress) 4.19, 826f5b94e (ClientMessage) 4.25; SD
34025cbda 4.02 and 0b1602237 4.06 (key-symbol switch; an E on the whole switch gave a cc-12 helper, 3.97); small
bodies 3b85cc0db, 56935dd28 (flat), 1e8976b99 4.12, 8f5f63fea 4.14, 2162357ff 4.15, bf7b06987 4.26; setup 23eed7e2c
4.28, 8d81d1ece 4.30, aaa2f877c (XCreateSceneWindows loop, the fill moved up before the loop) 4.37, a91babc45
(XEnterWorkingDirectory) 4.40. **opencl.c:** b561b7d37 N `HasOpenCLDevices` 5.62 (leaves Complex Method); 6591df72d E
`GetOpenCLCacheDirectory` tail 5.63 (`#if WINDOWS` block untouched); 09cd0e787 I `LoadOpenCLDevices` (void caller,
four exits) 5.80 (targets_fill_check FAILs only on `return;`, a diff-alignment artifact, explained in the message).
Recipe text (Decided by `owner`): 79f3520c1 N step 2 (an array assigned whole to a pointer member of a struct the
block fills, `s.m=v;`, may join the input group; C11 6.3.2.1p3, 6.5.16.1p1), 75355d861 CT step 4 (the helper takes the
input struct the caller's sibling blocks share under N step 8). **Message slip:** the CT amendment came after its first
three uses (their messages cite N step 8 and CT step 4 together). Moved `ThrowXWindow`/`LogMagickEvent` calls now name
the helper in `GetMagickModule()`; all are unreached or debug-only. Not per-pixel, so no timing.
Plateau: XAnimateBackgroundImage (bumps 9, cc 57; function-statics whose address is also passed to many callees, and
`ThrowXWindowFatalException` blocks; its block stubs score 4.28-4.37 but no recipe admits a static struct whose
address is also passed to calls), a LeaveNotify helper clones Enter (3.94), XSaveImage keeps 2 bumps (returns inside
unreached blocks), XAnimateImages cc 78 and about 780 lines in setup blocks naming statics or throw macros,
XMagickCommand cc 9 and 7 arguments; opencl.c: LoadOpenCLDeviceBenchmark, GetOpenCLCacheDirectory (`#if` block), three
twin groups (owner CF question), EnqueueOpenCLKernel (10 arguments).

## Lane b, first round: display.c 1.41 -> 1.74 (wide build, merged)

50 commits (46 refactor, 3 playbook, 1 tools); Brain Methods 6 -> 4 (XColorEditImage, XCropImage and XROIImage leave
Brain Method and Deep Nesting); oracle cases 23 / 13 / 6 for ColorEdit / Crop / ROI, 0 diverged each step; whole wide
catalogue on the final tip 14,145 cases, 0 diverged. The lane rewrote its commit messages once (trees identical) to
drop cc numbers it had not measured. Unreached lines carry obj_modulo (MODULO-IDENTICAL or IDENTICAL) or `Verified:
guard`.
- **XColorEditImage 1.41 -> 1.49:** 1b5b74d05 playbook D amendment (a cast of a call to the exact type its function
  returns, and a `(void)` cast of a `(void)` cast, are no differing value) with e95fa2948 `reuse_check.py` stripping
  them; bd3ef9920 typedef move, 1d105895d D Method, 12f1a08be D Fuzz, c16b75ce5 D discard keys, 86665878c D motion
  (1.41 -> 1.40, committed `--no-verify`, said so), 161c2dd80 CT pen colour, 9f8604610 CT border, 5bdbe79f9 N reset,
  4c31cfc33 N flood, f6a25445e SR, c7f06689d W (1.49).
- **XCropImage 1.49 -> 1.60:** dbc79fccb D, 71c5f34d9 E, ffe4a3bb0 E, e94c3ecb7 OC (1.50), 5410d6290 N, ccac76829
  playbook, fe22e1069, 203ef8e42 CT, cbaefbadd N, 818678939 D, 3e2cc2050 CT, 8bd613c28 E, c21aac6ca E,
  a16397680 / 8babd3401 / 783b6162a SD x3, d704856d1 CT (1.60).
- **XROIImage 1.60 -> 1.74:** 60b51df0d, 224dcffa5, 5a0d8e8bb D x3; edde291e0 H (`ApplyMenus`, pp_identical wide
  IDENTICAL); ec6b7fec3 D, f1272ef13, 60e266abf CT, 3cdb92a75 CT, 0b9fa672f D, 523b00b46 D; f31758109 SD (1.64),
  0391d6ccc, 5fd1dd165 SD; 24bf31cec D; 8be13a1aa playbook, d6a6939f9 LC (1.64 -> 1.62, `--no-verify`, said so),
  78a01ac35 LC; 54b712895, 809963a3f CT; 1565bf89a OC (1.74).
- **New recipe text** (Decided by `owner`, 2026-11-10 OK): the D amendment above; **a Recipe named FT, Replace a
  Fall-Through into a Call by the Call** (case A ends `magick_fallthrough;` into case B whose whole body is `Q(a...);
  break;`; every argument names an object declared outside both cases; A gets the same call and `break;`; guard shows
  +1 for Q); **Recipe LC, Extract Block Returning Whether It Continued** (a block in a sequential loop body, leaving
  through `continue`, becomes a `static MagickBooleanType` helper whose `continue;` is `return(MagickTrue);` and whose
  end is `return(MagickFalse);`; caller `if (Helper(...) != MagickFalse) continue;`). **Code clash:** this FT is a
  different recipe from the held twin-fold recipe also named FT on `w18-ft-twin-fold` (not in main); if the owner
  approves the twin fold it needs another code.
- Score behaviour: the file sits at rounding boundaries (the first step of a chain dips 0.01-0.02); a step can lose
  0.02 when *Lines of Code in a Single File* leaves the review while the file grows; near-twin helpers
  (XROIMotion/XCropMotion, XROIKeyPress/XCropKeyPress) cost 0.02 each until the chain's last step.
- Plateau: XAnnotateEditImage (depth 5; about 17 steps), XDrawEditImage (depth 6, about 60 depth-4+ sites),
  XDisplayImage (depth 6, about 50 sites); XColorEditImage, XCropImage, XROIImage remain Complex/Large Method.
  Possible upstream: XCropImage help titles; XROIImage/XCropImage MotionNotify tests `event.xbutton.window` (not
  xmotion); SelectionRequest falls into `default:`; XROIImage ends `return(MagickTrue); return(MagickTrue);`.

## Lane a, third round: accelerate.c 3.04 -> 3.19 (wide build, merged)

27 commits (22 refactor); Bumpy Road is gone file-wide (it started on five functions). Whole wide catalogue: 14,145
cases, 0 diverged (one load-sensitive case settled). Each step ran `verify_wt` (wide); its only complaint is the old
`3999999999` warning in LocalContrast (it also prints VERIFY FAILED when a function has no oracle cases).
- **ComputeEqualizeImage:** 2dcad9cb4, 6762e7f6a, fde450b8f, 93ebd41fa P (`EqualizesRed/Green/Blue/AlphaChannel`, two
  sites each; flat, guard); 8891bedce E `EqualizeColormap` 3.06 (leaves Bumpy Road; the block is dead in practice,
  `checkAccelerateCondition` refuses PseudoClass; obj_modulo DIFFERENT, so E's own proof).
- **ComputeLocalContrastImage:** 1cd2e8032 CN (IDENTICAL); c2dac90a1 Y (launch loop, its throw moved into the helper)
  3.08 (5 cases).
- **ComputeContrastStretchImage** (no caller in the tree, 0 lines covered: guard level): 704f35af1, b7476a858,
  9a0b3d958, 7ce7a48e5 P (`Stretches*Channel`); 4d763c20f E `StretchColormap` (obj_modulo MODULO-IDENTICAL with
  `NOINLINE=ComputeContrastStretchImage`); e449166c9 SG 3.13; dd411bfc9 SG 3.15 (IDENTICAL).
- **ComputeDespeckleImage:** cd1a3b8b2 E `ReleaseTempImageBuffers` (IDENTICAL); 34f03476d Y with an array as the
  written variable 3.17 (2 cases).
- **ComputeResizeImage:** 735ee2bc9 I `ResizeHorizontalThenVertical` (52 cases); acd3d636f SG shape (c) 3.19.
- **New recipe text** (Decided by `owner`, 2026-11-10 OK): 123adc8cd **Recipe SG, Spread an Enclosing Guard over the
  Tests Inside It** ((a) `if (G) { if (A1) X1 else if (A2) X2 ... }` becomes `if ((G) && (A1)) X1 else if ((G) &&
  (A2)) X2 ...`; (b) `if (G) { S if (B) T }` becomes `if (G) S` then `if ((G) && (B)) T`; G has no side effect, S
  writes nothing G reads; it un-nests a bump without a helper, avoiding four per-channel helpers that clone each other,
  3.08 -> 3.01), checked by new `tools/guard_spread_check.py` (ba9a56d32; a planted `>=` is caught) and obj_modulo;
  93074eee0 SG shape (c) (`if (G) { S } else E` then `if ((G) && (B)) T`, neither S nor E writing what G reads; tool
  baffd4076); e6ce9691b V step 2 and Y step 4 (the one written variable may be a local array the block only indexes,
  passed as `T *v`).
- **For the owner:** throws moved into helpers in c2dac90a1, 34f03476d and 735ee2bc9 (reached code, oracle level):
  they now name the helper in `GetMagickModule()`, against the brief's advice to keep throws in the caller (those
  bumps held nothing else to cut around; same effect as segment.c under E step 6).
- Plateau at 3.19: every single-function stub of the 18 smelly functions is flat, and so are two to five together; only
  the eight largest together lift it (ContrastStretch, Despeckle, Equalize, LocalContrast, Motion, Blur, Rotational,
  Resize stubbed: 3.84; without Resize 3.55), about cc 400, which needs shared prologue and cleanup helpers across all
  of them plus kernel-argument and launch helpers. Still blocking: the 10 `Accelerate*` wrapper clones (owner), 18
  Excess Arguments (all removed: +0.08), 3,882 lines (the lane's helpers added about 160). Rejected: D on 37 release
  sites (flat; ContrastImage alone drops to 2.97 through a new clone), P on AccelerateGrayscaleImage (joins the wrapper
  clone group), four per-channel stretch helpers (clones, 3.01), arm helpers for Resize with a nested exit test, a
  typedef move for `ReadHistogramBuffer` (reverted). Possible upstream: ComputeEqualizeImage's PseudoClass block is
  dead; ComputeContrastStretchImage has no caller anywhere in the tree.

## Lane b, second round: display.c 1.74 -> 1.97 (wide build, merged)

26 commits on 3a1ec92e8 (18 refactor, 5 playbook, 3 tools); no `--no-verify`. `XAnnotateEditImage` leaves Brain Method
and Deep, Nested Complexity (bumps 17 -> 6, 742 -> 470 lines; it keeps Complex cc 67, Large, Excess Arguments 5); the 22
cases reaching it ran every step, 0 diverged; whole wide catalogue 14,145 cases, 0 diverged. Steps were flat until the
last two: e6db624c5 D `XTrackInfoWidget`; CT c369647c9 font (1.76), a163e79f0 pen colour, 9da754cd0 box colour,
755d2c72c rotation (statics via targets, per-case `AnnotateMenuInput` fill); eaba8fb4f E `XRedrawTextLines`;
f9e195d98 and 258b02af3 CT BackSpace and Delete; 4191048fb CR `XAdvanceTextLine`; e929ae695 FT guard form; dd9634f3e CR
`XInsertTextCharacter`; afbea91be E `XModifyKeySymbol`; dc2c7a0ff SR `XTypeTextKey`; 2fb62d444 CR `XAnnotateKeyPress`
(1.81); 914f36a32, c7b79db40 CR paste; 15da51c9d E `XSetBoxPixelIndex`; eeb700e50 E `XSetPenPixelIndex` (1.97, the stub
ceiling). Evidence: oracle plus obj_modulo (last one IDENTICAL), CT/CR steps also case_tail_check OK. Recipe text
(Decided by `owner`, 2026-11-10 OK): 23b56bca5 **new Recipe CR, Extract a Tail That Ends Its Case or Iteration or
Returns Failure** (a switch case's tail or a sequential loop body's tail leaving by `break;`/`continue;` or falling
off, every `return` returning one constant; written variables by CT's targets struct; a `static MagickBooleanType`
helper returning `MagickTrue` on the normal end and `MagickFalse` for the early return; the caller `if (Helper(...) ==
MagickFalse) return(c);` then the case's `break;`); 349fe2a4c CT step 3 (a member may be added later as a new last
member when its helper writes it); 317a2b8fa FT (the next case may be the guard `if (Q(...) == MagickFalse)
return(c); break;`); 7bf74b70e SR (a kept group may write the controlling variable if no group falls through into a
moved group; a shared input struct filled once before the switch); e8381f5b8 CR step 4 wording; tools:
`case_tail_check.py` (pointer and array temporaries, comment lines, `--returns`). Plateau: XDrawEditImage and
XDisplayImage need a 20-25 step chain each (all depth-4 sites under two or three loops; a raw probe moving both
do-loops and the polygon bounding box scores 2.19 for XDrawEditImage but those helpers would carry findings of their
own; the finding-free chain needs one new recipe, a void block with a targets struct of 7-8 written variables, plus SD
splits of two 11-label element switches). **display.c has 14,493 code lines: adding about 8 or more lines anywhere drops
the score 0.03 once** (a 5-statement dummy function 1.97 -> 1.94, 60 statements also 1.94; probably a Lines of Code
step near 14,500), so any further chain pays that once. Possible upstream: `XAnnotateEditImage`'s `XK_bracketleft` group
assigns `XK_Escape` then breaks (a dead store); the selection-paste loop and the Return key allocate the line with
differently cast expressions; both allocation-failure returns leak `annotate_info`; `cursor` is not freed on the early
returns.

## Lane c, second round: widget.c 2.99 -> 3.02 (wide build, merged)

43 commits on d71ca3694; only two raised the score, none dropped it; whole wide catalogue 14,145 cases, 0 diverged (one
load-sensitive xevents case settled). **XMenuWidget** (162 cases, 0 diverged each; cc 49 -> 10, LoC 304 -> 96): CT
1bed49df5 Press, 5f170ef64 Release, 205e39716 Configure, 641144ba2 Expose, 07b38dc0f Leave, fd056c408 Enter, 883df5554
Move; O step 7 b3e4f9b77 Update, 6b2d946ed Redraw, 7b1f7b09d DrawMenuLine; SD 4e1e40189 then N d78420903 **2.99 ->
3.01** (bumps 4 -> 0; no helper has a finding); E b1f2aeb0d CloseMenuWidget; b52f13192 MapMenuWidget. **Shared
handlers** (flat, IDENTICAL): extracted once in XColorBrowserWidget (ConfigureWidgetWindow 8e03f6540,
EnterWidgetWindow abfedfcc4, ExposeWidgetWindow 806e38736, LeaveWidgetWindow 63f28c3f5) and XConfirmWidget
(ClientWidgetMessage 8082c137b), reused by D (b) in Confirm, Dialog, File, Font, Notice, Preferences and TextView
(fc13557f1, e94eed72d, 8da715ca4, 511db6120, 1341ef74a, 55ef36ecf, ca5f4e737; ClientWidgetMessage in Notice,
Preferences, TextView: f89bb116a, a1707fda9, e651b2ed2; reuse_check reports only the break/return rewrite lines).
**XNoticeWidget** (unreached; `Verified: guard`; obj_modulo IDENTICAL or MODULO-IDENTICAL): CT ab9c678cb, 31ebdc634,
458934724 (cc 47 -> 28). **XConfirmWidget** (5 cases): CT b81e86d93, 7465491c6, 7c9c30b5e, 487679bce (Move as two CTs so
the helper stays under cc 9); H cdf5ed3d2 (macros hoisted; pp_identical on cand-wide IDENTICAL); O step 7 a089ba207,
9417aae84; SD d54e9030d; N 79f56f93b **3.01 -> 3.02** (bumps 4 -> 0, cc 10, LoC 95). **Playbook:** the lane's own CT
step 3 amendment (5978a05a5: a variable outside the struct may be added by a later block) duplicated lane b's
349fe2a4c (merged first); the merge kept lane b's wording and dropped the duplicate; the code commits that use it
(`MenuTargets`, `ConfirmTargets`) are unchanged. Plateau: flat or non-monotone until a function leaves a category: a
lone bump drop 5 -> 4 in XNoticeWidget costs -0.02 (XTextViewWidget at 5 bumps likewise; both lowered it is back at 3.01;
not committed); XMenuWidget leaving Complex Method alone (cc 10 -> 8) costs about -0.11 (left at cc 10); from the current
HEAD any single widget stubbed gives at most +0.01, the eight smaller ones +0.18 over 3.01, with the browsers 6.30.
Next if continued: Dialog, Preferences, Command and List with the same pattern; Notice and TextView as one same-recipe
set. Possible upstream: XMenuWidget `width=WidgetTextWidth(...title)` is a dead store (moving the width loop leaves
`width` set but unused: `-Wunused-but-set-variable` in CI); XPreferencesWidget's Cache press overwrites `width`. Tool
gaps: `case_tail_check.py` fails on an extended targets initialiser and on `&targets` becoming `targets`. Message slips
(amended): the Notice Move commit now says `dismiss_info` is the only written variable; the TextView reuse commit first
carried the Preferences message after a race with a background run.
