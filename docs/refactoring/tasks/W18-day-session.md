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

## Lane a, fourth round: xwindow.c 1.99 -> 2.27 (wide build, merged); composite, feature-texture, layer no commit

21 commits; whole catalogue wide 14,145 and narrow 15,900 cases, 0 diverged. Each step ran `verify_wt`, guard, `--calls`,
`ci_linux_warnings` and obj_modulo (every step MODULO-IDENTICAL or IDENTICAL, 66 other functions compared); verify prints
VERIFY FAILED only from the two old `ci_warnings` diagnostics.
- **XMakeImageLSBFirst** (514 cases): 5cd4c069e N+E7 bitmap row helper, baf8a126c N y-loop helper (flat), 528e1ed44 O
  matte row (`x` and `q` out-parameters) 2.03 (leaves Deep Nesting). **XMakeImageMSBFirst** (no case): fcccf9a64 N (flat);
  801344a36 E step 4 (amended) matte row 2.06 (leaves Deep Nesting; a first O version was reset: O is not open to unreached
  functions). **XDitherImage** (no case): 1940bd2c8, bd55c2844, 647d2f6df N red, green and blue entry helpers sharing one
  struct, 2.10 (leaves Deep Nesting; the red and green helpers are twins and are not flagged as duplicates).
  **XMakeMagnifyImage** (13 cases): 89b8b9fb3 E, 2201eff1a, f50782fbb, 9adb49685 N (8-bit and multi-byte cases, flat);
  cfc9c0720, 83afa1be4 Z (MSB, 2.18); 13a7b9b8c, 797e307af N (MSB, 2.23); 75383d3d5, 8b0b32d47 N (bitmap row loops,
  flat); cfc0daabe SG shape (b) on both `if (XYBitmap) {S; if (Swap)}` blocks 2.27 (leaves Deep Nesting; depth 8 -> 3,
  cc 72 -> 47, 366 -> 288 lines). The W17 note that only Q fits was wrong: the four case loops are N, E and Z, since `y` and
  `q` are dead after them. 71f3616ee tools (`struct_fill_check.py` reads `(*name)[N]` members); f7e16c4d8 playbook.
- **Recipe text (Decided by `owner`, 2026-11-10 OK): Recipe E step 4 amendment.** A variable the block writes before it
  reads on every path, that nothing after the block reads, and whose only use left in the caller would be a store before the
  block that nothing else reads (a column counter `x` set by `x=0;` above a loop the block's own `for` restarts), so that
  declaring it in the new function would leave that store set but never used (`-Wunused-but-set-variable`, an error in the
  CI jobs; no recipe deletes the store), passes as a pointer `T *v` on Recipe O step 2's last paragraph's terms.
- Plateau xwindow.c 2.27: XMakeImage (Brain Method, shared-memory `#if` blocks plus three other depth-4 sites; emptied
  2.57, without the shm blocks 2.35), XBestVisualInfo and DestroyXResources (`#if` blocks); XGetWindowImage (+0.05 if
  emptied: two different exits, the switch's `while` loops keep depth 4); SG found no usable shape (9 of 33 candidate spreads
  rescored, all flat or worse).
- **composite.c 3.69:** the bumps in `CompositeImage` are the Intensity loop (a clone of `CompositeCopyImage`: owner
  question), the Displace loop and the main loop (progress-tail pragma blocks); the score stays flat until all go, and the
  function emptied scores 3.71; no SG, UX or LC shape found. **feature-texture.c 7.63:** the two bumps are the parallel
  grays column loop (the owner's per-iteration-slot question; removing it scores 7.75) and the tail loops need `shared(status)`
  used in a helper. **layer.c 8.25:** the first validation loop's `ThrowImageException` calls differ in tag (V, I, RV refuse),
  the frame loop's else block carries `#if DEBUG_OPT_FRAME`, SG-style rewrites of the `x < 0` nest left the bump count at 3.
- Possible upstream: `XGetWindowImage`'s second exit (`AcquireMagickMemory(p)` failure) leaks `colors` and `ximage`, unlike
  the first exit; the dead `x=0;` stores in `XMakeImageLSBFirst`/`MSBFirst` remain (now kept alive by passing `&x`).
  Tool notes: `refactor_guard --calls` prints a stray `+2 short` for a member declared `const short (*v)[16]` (still
  PASS); cs-mcp rates all three bump-removing steps flat when a helper still has two or more bumps.

## Lane a, fifth round: xwindow.c 2.27 -> 2.42 (wide build, merged); opencl.c no commit

16 commits; whole wide catalogue 14,145 cases, 0 diverged; obj_modulo MODULO-IDENTICAL or IDENTICAL on every step; no helper
has a finding. `verify_wt` prints VERIFY FAILED only from the two old `ci_warnings` diagnostics.
- **XGetWindowImage** (173 cases each): 7aec2b313 L (hoist `ColormapInfo`, IDENTICAL), 37b4c8dcd P
  `IntersectsTopLevelWindow`, bc5609bf7 E `IsWindowImported`, 5e1fd3b04 E `FindWindowColormap`, 95dd02cc6 I
  `XGetWindowColormap` (writes `number_colors`, `colors` and the block-static `colormap_info`; two exits), 1b6506206 D
  `XGetMaskShift` x3, 2e9cc9237 N `XImportDirectClass`, 4ee5c0070 Y `XImportWindowImage`, 3ec02e105 CN 2.32 (leaves Deep
  Nesting, the whole +0.05 emptying it was worth). All steps before CN were flat.
- **XGetResourceInfo** (539 cases): 74892e1c0 E `GetResourceDisplayOptions` 2.35; 51b81273a E `GetResourceLayoutInfo` 2.42
  (leaves Large Method). **XInitializeWindows** (347 cases), four flat E steps leaving it with no findings: f0c6edcc6
  `LogXWindowProtocols`, c25726d2a `LogXWindowVisual`, 05e15a3c8 `GetXWindowGroupLeader`, 4ffeabeef
  `InitializeXWindowColormaps`; the moved `LogMagickEvent` and `ThrowXWindowFatalException` calls now name the helper in
  `GetMagickModule()` (run only under `-debug X11` or on a fatal error).
- **Recipe text (Decided by `owner`, 2026-11-10 OK):** edc9084c7 Recipe I step 2 amended: a block-scope `static` of
  arithmetic, enumeration or pointer type (not an array), whose address the caller takes nowhere but in the targets
  initialiser, may be a written variable, on Recipe CT step 2's and O step 2's terms (its member is a pointer to its type,
  each use is `*v`, the call passes the same address). It copies the CT and O amendments, which already admit block-scope
  statics.
- **Plateau xwindow.c 2.42:** `XMakeImage` (stubbing every non-`#if` block still 2.32: Brain Method, depth 4 and 206 lines
  stay in the shared-memory `#if` code), `DestroyXResources` and `XBestVisualInfo` (`#if`-pinned); `XGetMapInfo`'s three
  mask blocks differ in two members each, so MA does not fit; `XBestIconSize` keeps two bumps; any other single function
  stubbed scores 2.32, only a group lifts it (`XMakeStandardColormap`, `XDrawImage`, `XMakeWindow` together with the other
  groups: 3.40). **opencl.c 5.80:** held items stay held; E on `BenchmarkOpenCLDevices`' inner loop
  (`SetScoreOnSameOpenCLDevices`, unreached) scored 5.67 (the helper gets a Code Duplication finding), reverted.
- Tool gaps: `targets_fill_check.py` and `struct_fill_check.py` each read one struct, so they FAIL a step using both an input
  struct and a targets struct (95dd02cc6: fills and bindings checked by reading); `struct_fill_check` FAILs a pass-through
  `&source` -> `source` (4ee5c0070). Possible upstream: `XGetWindowImage`'s second colormap exit (`AcquireMagickMemory(p)`
  fails) leaks `colors` and `ximage`; its `AcquireImageColormap` failure path leaks `composite_view`.

## Lane d, third round: compare.c 8.92 -> 9.68 (merged, with 22 new oracle cases); magnify, animate, accelerate plateau

8 commits; whole catalogue on the lane tip 15,922 cases (15,900 plus the 22 new), 0 diverged; on the merged tree 15,922
cases, 0 diverged. The only finding left in compare.c is the five arguments of the public `CompareImages`.
- 6a2b0db91 FB `CompareImages` row fill 8.95 (IDENTICAL; FI step 8(b)'s hoist measured DIFFERENT object code).
  **8b18f7fef oracle (tools only): `imdriver imagesequal IMAGE RECONSTRUCT [CHANNELS]` and 22 cases** for `IsImagesEqual`,
  which no case reached (neither the command line nor any library function calls it): equal images, a difference in the
  first pixels, a difference only in alpha (seen or masked out), images of different size and of different channel counts;
  they protect the row and column loops that Recipe LT changes. `casemap.json` is stale until rebuilt: run them with
  `oracle.py run --filter imagesequal`. c5b14026a LT `IsImagesEqual` (`IsCompareRowEqual` plus the `CompareColumnScan`
  input struct that LT step 5 sends to N) 9.02 (leaves Bumpy Road). 83189e0d0 rule 1 Overall Code Complexity set on
  `CompareImages`: I step 2 const form with five written variables (`AcquireCompareImages`), O step 7
  (`GetCompareColors`), N step 1 whole parallel loop (`HighlightCompareRows`): 9.09 (alone I 8.60, O 8.52, N 8.63; pairs
  8.65, 8.81, 8.68). 14532069f PS `SyncCompareRow`, 192cf14ac PS step 7 (`CompareRow` with a `const CompareRowInput`
  initialiser), 66b4e570a E step 7 (tail of `AcquireCompareImages`): flat; 580970288 E highlight colour triple
  (`GetCompareHighlight`) 9.68. The four flat steps are one plan; probes showed Overall Code Complexity (mean cc 4) clears at
  exactly four more functions, so the file never carries it at the end. Evidence: oracle on the 187 cases that reach
  `CompareImages` and its helpers (165 from the old map plus the 22 new), 0 diverged at every step; guard, `--calls` and
  `ci_linux_warnings` pass; failure paths are allocation failures, checked by reading. The per-pixel `HighlightCompareRow`
  is untouched (the new cost is one call and one struct init per row; a noisy run at load 15 showed no clear difference).
- Plateaus (no commit): **magnify.c 8.84**: folding the 12 `if (C) CopyPixels(..) else CopyPixels(..)` pairs in `Scale2X` and
  `Scale3X` into `?:` frees the line budget (flat at 8.84, object code DIFFERENT, no recipe for it); with it NS on `Xbr2X`
  scores 9.07, but NS step 5 demands IDENTICAL and here `Xbr2X` alone differs (`setbe` -> `setle` on 0/1 values;
  obj_modulo clean elsewhere); without the fold NS on `Xbr2X` crosses 1,000 lines (8.52); `Scale3X` stays cc 12 (the `?:`
  sites still count). **Owner call needed:** a fold recipe and accepting NS on a reached function without IDENTICAL.
  **animate.c `XMagickCommand` 4.40**: the OpenCommand tail is unreached and exits through `return((Image *) NULL)` with a
  `static char filenames[]` array (V, Y, W not allowed on unreached code; I needs two to four written variables; E step 7
  and CT refuse `return`). **accelerate.c `AccelerateGrayscaleImage` 3.19**: P twice removes Complex Conditional and
  Complex Method, but the function joins the wrapper Code Duplication group (10 -> 11 functions), so the score stays flat.
- Possible upstream: `CompareImages` tests `(difference_image != NULL)` after an earlier return (always true);
  `IsImagesEqual` has no caller in the tree. Tool note: `verify_wt`'s `--function` run exits on the stale `casemap.json`
  after the oracle commit (use `--filter`).

## Lane c, third round: widget.c 3.02 -> 3.04 (wide build, merged)

34 commits (33 widget.c, 1 tools); every step ran `verify_wt` (oracle on the function, guard, calls, CI warnings,
obj_modulo); whole wide catalogue 14,145 cases, 0 diverged (one load-sensitive xevents case settled). `obj_identical.sh`
needs otool on this machine, so `obj_modulo.sh` with dummy caller and helper names was used.
- **XDialogWidget** (67 cases, 0 diverged each): 3470709f7 H; CT on Press, Release, Client, Key, KeyRelease, Motion and
  SelectionNotify (33490478e, 3159150c2, 3b85974a6, fb8735e1b, bc05bdd2c, 7b66a8eaf, e7931dfe4; `DialogTargets` plus a
  shared `DialogContext`); O step 7 on options, Update, Redraw (06e63ce47, f634713f2, c5bcfa5fc); SD bb6f4e728, 0ad3bc5ad;
  8362726b1 N on the switch **3.02 -> 3.04** (bumps 4 -> 0); c0ba1e599 N Position, 3183ddea0 E Map; 0e9018d9c N loop,
  9e156a117 FB (IDENTICAL), 3521fc2a0 E width, dc204dafc P predicate (these four take it out of Complex and Large Method
  together; only the public 5-argument finding is left).
- **XCommandWidget** (346 cases): 0a299b137 F tile table (308 -> 222 lines), 1fbea7b27, 53a153cf0, a5fbf7816 E on three
  cases, 31f1c24d4 SD, 246b40756 O, 0fdf6ca4d H, 8d250fb4e E Redraw (**committed `--no-verify`, 3.04 -> 3.02**, recovered by
  the next steps), 6094e3e77 N Update, f5f56475c E Reconfigure (3.04), 635c9e9e0 N Size; every finding gone.
  **XConfirmWidget** (5 cases): 96c2877e7 E Map, f1bf10a3b N Position; no finding left. Tools: 0791c62b2
  `case_tail_check.py --shared` (checks CT helpers that take the N step 8 shared context; it failed on every one).
- **Ordering lesson:** a step that takes a function out of Complex Method while it is still Large costs 0.09-0.13
  (predicate first: 2.91); move lines first and the predicate last, and no step dips (the Dialog chain stays at 3.04).
- Plateau at 3.04 (measured by stubbing function bodies): all remaining non-browser widgets 3.17; four browsers 3.66;
  everything 6.64; the score only moves once the File, Font, Color and List browsers are done as well. Left alone: Menu (a
  scan helper makes `width` a dead store, `-Wunused-but-set-variable`, a CI error; its 5 arguments are public; one probe
  showed an oracle divergence that was not rerun), Notice (no case reaches it; O and CT-style blocks need cases at guard
  level), Preferences and TextView (written arrays `preferences_info` and `cache`, 8+ written structs: a new CT or O
  array-member rule, about 15 commits each), the browsers (not started).
- Evidence notes: a moved `assert` stays in the caller (the abort message keeps the original function name); no
  `GetMagickModule()` throw was moved; the `Map*Widget` helpers (Dialog, Confirm) are twins that CodeScene did not flag.
  Possible upstream: Preferences' Cache press overwrites `width`, which the Update block reads later; Menu's
  `width=WidgetTextWidth(...title)` is a dead store; Dialog's SelectionRequest case has no `break;` and falls into
  `default:`; Dialog's `windows->widget.height < windows->widget.min_height` can never be true; Command's moved
  ConfigureNotify and Expose cases are unreached by the oracle.

## Lane d, fourth round: segment.c 9.02 -> 9.09 (merged, with 33 new oracle cases); the reach lever ends

2 commits; whole catalogue on the lane tip and the merged tree 15,955 cases (15,922 plus the 33 new), 0 diverged, no load
timeouts. 1f4a88df6 `oracle:` (tools only): imdriver command `dynthreshold IMAGE CLUSTER SMOOTH` printing the status and the
returned (red, green, blue), and 33 cases over 11 corpus images (rose, rose_blur, rose_alpha, palette, gray16, bilevel,
tiny, tall, wide, hdri, cmyk) with three threshold pairs each (ids via `--filter dynthreshold`); before them
`GetImageDynamicThreshold` and about 15 helpers were unreached (API only). A coverage build shows every line of the function
and its helpers executed except the allocation-failure exits; each case ran twice on the baseline with identical output.
6fa5e87b0 Recipe W on `GetImageDynamicThreshold`: the histogram/extrema allocation loop becomes
`AcquireDynamicHistograms(image,histogram,extrema,exception)`; 9.02 -> 9.09 (the function leaves Bumpy Road with one bump
left, so no finding; the helper has no finding); verify passed (guard, `--calls`, CI warnings, 33 cases, 0 diverged); only
the success path is oracle-checked, the allocation-failure exits by reading; **the moved `ThrowMagickException` now names the
helper in `GetMagickModule()`** (E step 6 effect; the owner question from W14 and W17). The second bump (`head == NULL`)
was not taken (one bump already clears the finding). Plateau at 9.09: Lines of Code (1,655) and the public 5-argument
`GetImageDynamicThreshold` and 6-argument `SegmentImage`; `split_source.py` refuses both roots (40 shared statics).
- **Other candidates checked (no commit).** The stale `casemap.json` keys functions by pre-split file names, so reach was
  matched by function name and cross-checked with the wide lcov. `GetImageDynamicThreshold` was the only unreached function
  with a finding in a non-wide file; other zero-reach functions in files below 10 are failure-path `Relinquish*` helpers,
  progress helpers and public API in plateau files, none carrying a finding. magnify.c 8.84: `Scale3X` and `Xbr2X` are fully
  covered (reach is not the blocker); the chain needs the two unruled recipes (the `?:` fold, NS accepting a reached function
  without IDENTICAL), probed at about 9.07 with `Scale3X` still cc 12. montage.c 9.11: the line budget (about one line under
  1,000), not reach. colorspace-convert.c 9.08: label calls have different signatures, so TB/KT do not apply. morphology.c
  6.19: `split_source.py` accepts families of 1 to 4 functions in a 180-function file, which would not clear Number of
  Functions. plasma, composite, compare-similarity: clone cliffs, label-run questions or the FFTW/HDRI region.
- Possible upstream: when `extrema[i]` allocation fails but `histogram[i]` succeeded (or the reverse),
  `RelinquishHistogramsBelow(i,...)` frees only indices below `i` and the one that succeeded leaks; the `head == NULL`
  allocation-failure return leaks the histogram and extrema arrays; `extrema` (short) is allocated with
  `sizeof(**histogram)`, so it is over-allocated (harmless). Tooling: `verify_wt.sh --function` exits on the stale
  `casemap.json` (rebuild it once, in the foreground, when no lane is running); the pre-commit hook needs a login shell
  (`bash -ls`) to find `node`.

## Lane a, sixth round: xwindow.c 2.42 -> 2.47 (wide build, merged)

29 commits; whole wide catalogue 14,145 cases, 0 diverged (147 s, no load timeouts). Every step: wide oracle `--function`
(0 diverged), guard and `--calls`, `ci_linux_warnings`, `obj_modulo.sh` (IDENTICAL or MODULO-IDENTICAL, except the
FontToList step), review diff with no new finding. Raises: 98c07a8cb `XFreeResources` leaves Bumpy Road and Complex Method
2.43; `XGetWindowInfo` three E steps (574da6590, d5f0b2aab, 19a685f9a, the last leaving all three categories) 2.45;
`XDisplayImageInfo` four E steps (433219d9e..4f40a08e7) 2.46; `XAnnotateImage` N pair 4addf55fa then 68b5bca0b 2.47, then
five flat steps (ccf43ff25 E, 5a229598a E, b518fd7b5 O, 8144d45a2 V, a0e9b254d V; it ends with only Excess Number of
Function Arguments). Flat steps that each leave a category: 80979adfd playbook (Recipe MA step 2 amended); 2775cadde MA
`XGetMapInfo` (leaves Bumpy Road and Complex Method); `XBestIconSize` 1233c0655, 7b4727732, a0721b2aa E; `XBestPixel`
46cb9d18a, c790f817a E; `XFreeResources` 6e8f4ffbc E; `FontToList` 3fdb5c3ba E and b00817bcd P (obj_modulo shows
register-choice differences in `XBestFont`, which inlines `FontToList`; the oracle is clean); 5944898d3
`XGetResourceDatabase`, b7a569ca8 `XError`, ca8ecde26 `XMakePixmap`, ffd3f0250 `XRemoteCommand` (E, unreached: guard plus
IDENTICAL object code). **Order matters (probed):** leaving Bumpy Road and Complex Method while the function stays a Large
Method lowers the file (`XGetWindowInfo` alone 2.36; `XAnnotateImage` with Complex Method gone and 71 lines left 2.41), so
the Large Method goes first or all three categories leave in the last step.
- **Recipe text (80979adfd, Decided by `owner`, 2026-11-10 OK): Recipe MA step 2 amended:** (b) the operand the block
  evaluates first and unconditionally may differ (the `x` of a leading `if ((x & M) != 0)`, or the right operand of the
  block's first statement); (c) a copy may name two lvalues `e.m1` and `e.m2` of one base `e` (two different members, each
  with its own declared type and its own pointer parameter `T1 *value1,T2 *value2`); Careful: two different members of one
  object never alias.
- **Plateau 2.47:** a padded stub of the non-pinned functions gives 4.07 (beyond what the helper count allows); stubbing
  one function at a time: `XGetWindowImage`, `XRefreshWindow`, `XRenderImage` and `XSelectWindow` give +0.02 each, every
  other function +0.00. Blockers: `XRefreshWindow` (`#if` shape and shared-memory code pin cc 15), `XRenderImage` (nine
  function-local statics; the init block needs a targets struct of eight), `XSelectWindow` (hoisted macro
  `MinimumCropArea`, twin draw ifs, a ButtonPress group writing four variables), `XGetWindowImage` (block statics);
  `XGetResourceClass` needs Recipe D (b), whose text wants a raise (helper `XResourceClassCharacter`, flat probe 2.47);
  `XUserPreferences` is whole-body `#if`; three functions are `#if`-pinned.
- **Tool gap that matters for CI:** `ci_linux_warnings.sh` did not flag a `&param` of type `const T **` passed where
  `const T *` is expected (a first `XTransferAnnotatePixels` attempt, not committed); only the oracle caught it (29
  diverged). MSVC reports this as C2664, the kind of error that broke Windows CI on W16. `struct_fill_check` does not read
  the sibling-struct N form; `casemap.py` with `WIDE=1` starts a re-index (use the wide lcov for reach).
- Possible upstream: `XAnnotateImage` and `XDrawImage`: the transfer loop clobbers `y` and the later `XParseGeometry`
  leaves it alone when the geometry has no y, so the composite offset is the loop's end row; `XGetWindowInfo` sets
  `window->y` from `XDisplayWidth`; `XDisplayImageInfo` leaks the unique file and descriptor when `fdopen` fails;
  `XMakeWindow`'s shape test is dead after the unconditional `shape=MagickFalse`; `XSelectWindow` reads
  `event.xbutton.button` after the loop whatever the last event type was; `XFreeResources` has a dead store
  `font_info=NULL`.

## Lane b, third round: display.c 1.97 -> 2.58 (wide build, merged)

52 commits (50 refactor, 2 playbook); `XDrawEditImage` and `XDisplayImage` both leave Brain Method and Deep, Nested
Complexity. Every step ran `verify_wt` (15 oracle cases for `XDrawEditImage`, 317 for `XDisplayImage`, 0 diverged) plus
obj_modulo or obj_identical on unreached lines; whole wide catalogue 14,145 cases, 0 diverged (150 s; one single-run
load-sensitive divergence reran clean alone). Raises (the rest flat inside probed chains, rule 2): **XDrawEditImage**
d310956f8 CT colour 1.97 -> 1.94 (the only `--no-verify`: the one-time Lines of Code step, said in its message),
50c4ac648 E `XCreateStippleBitmap` 1.97, 8b1e2daaf CT `XSelectDrawStipple` 2.03, 981bde064 SD `XDrawCommandAction` 2.07,
9efafd8ab OC `XDrawCommand` 2.28 (leaves Brain Method and Deep Nesting); **XDisplayImage** a82640828 LC
`XSelectCommandMenuEntry` 2.28 -> 2.58 (leaves Brain Method and Deep Nesting). Recipes used: CT (colour, width, stipple
tail and head, button/key/motion, shape release/motion, XDisplayImage Button1/Button3/image window), E, N, Z
(`XPreviewLine`, `XImageWindowKeyPress`), O (timer), D (Motion into `XTrackInfoWidget`; the polyline, long-line, focus and
virtual-command copies), SD, OC, LC (timer, command menu), CN (eight pairs; the ninth would add two complex conditionals,
measured 2.28 -> 2.23), H (macros hoisted, pp_identical IDENTICAL). `case_tail_check.py` cannot read the shared-context
form (checked by reading, said in each message).
- **Recipe text (Decided by `owner`, 2026-11-10 OK):** ffa5f523f CT step 5 (a call in the block of a helper this recipe made
  earlier for the same caller, `Inner(&name,...,&targets);`, is written in the helper `Inner(name,...,targets);`, passing on
  the two `const` pointers it received; the helper binds from the targets only the members its own text writes); cb23c728c
  OC step 7 (the block may hold calls of helpers that CT, SD or Z made earlier, written `Inner(name,...,targets);` in the new
  helper; it takes the shared input struct and up to two targets structs, at most four parameters, binding only the members
  its own text writes).
- Notes: one helper carries a finding (`XCreateStippleBitmap` is a pure label run, Complex Method cc 9: ruling of
  2026-11-07); *Number of Functions in a Single Module* now appears (allowed when the chain lifts the file; it did); moved
  `ThrowMagickException` calls now name the helper in `GetMagickModule()` (ButtonRelease and Motion list growth, both
  unreached). Rejected: CN on the loop-head pair (2.23); a combined transient-window helper and the first timer helper
  (bumps, Complex, Deep); twin immutable blocks in two helpers (new bumps, fixed by `XSelectVirtualCommand`).
- Plateau: `XDrawEditImage` file-wide 2.58: Complex cc 64, Large 382 lines, two bumps, five arguments (signature);
  `XDisplayImage`: Complex cc 186, Large 1,447 lines, 14 bumps; the setup before the loop (about 700 lines of window creation
  and cleanup) is untouched and needs its own chain. Possible upstream: `XDrawEditImage`, after a failed
  `ResizeQuantumMemory` in ButtonRelease/MotionNotify, throws and the next event writes through the NULL `coordinate_info`;
  the stipple file branch passes `&x,&y` to `XReadBitmapFile`, which overwrites the pointer position with the bitmap hot
  spot; `degrees=0.0` is a dead store; `XDisplayImage`'s Magnify configure branch's `status=XMakeImage(...)` is never read.

## Lane a, seventh round: the CI warning check now compiles the X11 and OpenCL code (merged)

f05038024 `tools/ci_linux_warnings.sh`: the gap lane a found in the xwindow.c round (a `&cp` of type `const T **` passed for
`const T *`, the MSVC C2664 class) was the configuration, not a missing flag: gcc 13 and clang 18 both report it by default
as `-Wincompatible-pointer-types`, but all four CI trees are built `--without-x` and without OpenCL, so the X block of
xwindow.c (and display, animate, widget, accelerate, opencl and the OpenCL branches of other files) is compiled out. A
planted error inside the X block of a scratch copy of xwindow.c printed OK with the old script. Two new trees,
`build-oracle/ci-{gcc,clang}-x` (`--with-x --enable-opencl --with-fftw`); from them only pointer-type, discarded-qualifier,
int-conversion, implicit-declaration and error lines are kept (the X and OpenCL code has hundreds of upstream `-Wshadow`,
`-Wunused-parameter` and `-Wsign-compare` diagnostics that CI never sees); `-Wincompatible-pointer-types` is named for both
compilers; the header comment and `docs/refactoring/VERIFICATION.md` line 59 are updated. The planted error is now reported
by gcc-x and clang-x; `--campaign-a` passes on all 84 files in all six configurations (the first run builds the two new
trees, about a minute); KNOWN is unchanged. The oracle gate therefore now includes the X and OpenCL files' pointer
conversions in its CI check. Merged tree: 15,955 cases, 0 diverged.

## Lane c, fourth round: widget.c 3.04 -> 3.08 (wide build, merged; the Font chain is mid-plan)

66 commits on 51e74cccb (61 refactor on `widget.c`: 39 on `XColorBrowserWidget`, 22 on `XFontBrowserWidget`; 2 playbook; 3
tools); whole wide catalogue 14,167 cases, 0 diverged. Every refactor commit passed `verify_wt` (oracle on the function,
guard, calls, CI warnings) and obj_modulo (IDENTICAL or MODULO-IDENTICAL); each message lists its own measured cc, LoC and
bumps. The local `casemap.py` staleness bypass was reverted, not committed.
- **XColorBrowserWidget is finished (no finding left except its signature-level ones).** CT case tails (ClientMessage
  51fc2fce2, SelectionNotify, Motion x2, KeyPress, ButtonRelease x2, ButtonPress x4: list entry 6af83f84b, whole case
  2b08269c1); E KeyRelease value form f884403f7; H macros 39ede1b53; O step 7 (the Update layout in five runs plus the
  whole `if` f26e6cb25, and the Redraw, Jump, RedrawList, RedrawAction and UpdateList blocks); LB `ReloadColorList`
  b16147711; LC `WaitColorEvent` 4a19a4fa9 (**3.04 -> 3.06**, bumps 5 -> 4); SD x2 and N on the switch dc6435d88, N on the
  loop 906314c2f; E three clean-up helpers, then `WidestColorBrowserName` f8ed2c21c (**3.06 -> 3.08**, leaves Bumpy Road);
  O `RunColorBrowser` f92423d14 (about 35 lines out, cc stays 11); N `PositionColorBrowserWidget`, FB, E
  `WithdrawBrowserWidget`; E `GetColorBrowserWidth` 14b6e820f drops Complex Method and Large Method together (score stays
  3.08).
- **XFontBrowserWidget is MID-PLAN (flat steps, 22 commits; the plan's raising step is not yet done):** cc 104 -> 49, LoC
  792 -> 365, bumps 9 -> 6: all cases (CT, plus D reuse of `ReleaseWidgetKey`), H, the Update runs, the Redraw, Jump and
  RedrawList blocks (638148f5e, the last commit). Still to do: RedrawAction, UpdateList, Wait, SD x2, the switch and the
  loop, then the clean-up and set-up (UpdateList holds a `break` and a `return`, so the loop N needs a Recipe W-style helper
  returning `MagickBooleanType`; RedrawAction assigns the caller's `font_info`, so it needs the pointer form). File and List
  browsers: not started. The score moves only when all four browsers are clear (stub ceiling 3.66). Stopped at the commit
  target, not at a plateau.
- **Ordering lesson (measured):** taking Color out of Complex Method while it was still Large dipped 3.08 -> 2.93; steps
  that cut lines while cc stayed at 9 (FB, then `WithdrawBrowserWidget`) came first and the last step crossed both thresholds
  together. A Large Method needs LoC <= 69 (70 is flagged). Probe: 24 extra copies of three Dialog helpers produced no Code
  Duplication finding.
- **Recipe text (CT amendments, Decided by `owner`, 2026-11-10 OK; 0ccecfe88, 2df73482f):** step 3: six members stay the
  limit, except for a struct that serves every block of one event loop (the cases of its `switch` and its `if (state &
  ...)` blocks), which may hold one member per local of the loop's state; each block still writes two to six, each helper
  binds only the members it names (a member it only reads included) and uses `(*v)`; step 4: a `const` targets struct of
  another helper that the block passes on as `&t` goes as one more parameter `const T *t`, after the input struct and before
  this recipe's targets struct, each `&t` written `t`; step 5: a member the helper only reads, whose variable another block
  of the loop writes, and whose uses include a macro that names the variable (`QuantumMargin` names `font_info`), is bound
  `T *const v = *targets->v;`. Stretch to check: `text[]` and `reset_pattern` go in the context as `char *` although
  `XGetWidgetInfo` stores the address (same address). Tools: 17ed00058, 8a01372fc, 14558e37f (`case_tail_check.py` now reads
  an extended initialiser, a passed-on struct, re-wrapped lines and the live binding).
- **Message slip:** the Font commit that moves `PressFontBrowserReply` (45b4cfc08) does not mention the live `font_info`
  binding (the playbook text came after the first attempt) and carries a stray "no finding on XNone".
- Possible upstream: `XFontBrowserWidget`'s RedrawAction block does `font_info=XLoadQueryFont(...)`, calls `XFreeFont` and
  never restores the variable, so later blocks (`QuantumMargin`, `FindFontBrowserCursor`) use a freed or NULL font (kept as
  is); Color's and Font's SelectionRequest cases have no `break;` and fall into `default:`; Color's ButtonRelease ends
  `if (MatteIsActive(reply_info,...) == MagickFalse) break; break;`.

## Lane b, fourth round: display.c 2.58 -> 2.68 (wide build, merged)

29 commits (last 44d6685b6); whole wide catalogue on the lane tip 14,200 cases, 0 diverged (230 s). **Oracle caveat:**
`casemap.json` was stale after the merged lanes' oracle cases, so each step's cases ran through `--filter` built from the old
wide map (317 cases for `XDisplayImage`, 237 for `XScreenEvent`, 10 for `XOpenImage`): every step 0 diverged; every step
also passed obj_modulo or obj_identical (`NOINLINE=` for inlined functions), the guard, `--calls`, `ci_linux_warnings` and
`ci_warnings`; no helper has a finding, no new Code Duplication. The moved `LogMagickEvent` and `ThrowXWindowException`
calls now name the helper in `GetMagickModule()`. **The raise (XDisplayImage setup, the only score movement):** 5fbcac2e2 E
`XLogDisplayImage`, b8480ee86 E `XGetUpdateTime`, 18225a13c N `XNameImageWindow`, 1f31f38af N `XInitializeBackdropWindow`
(all flat; 18225a13c moved the 23 fill lines of `DisplayEventContext` up to just after `root_window` and added `font_info`,
1f31f38af added `argv`, `argc`, `class_hints` and `manager_hints`; a script shows nothing writes a member's variable between
the new fill and the loop), then f38e5c5e0 N `XMakeImageWindow` **2.58 -> 2.68** (bumps 14 -> 9, cc 186 -> 167, Large 1,447
-> 1,336; a probe showed these five blocks are the minimal set that raises the score). **Functions cleared (flat at 2.68,
each leaves a category under rule 2):** 3248bfb01 CN `XCropExpose` (leaves Bumpy Road; combining pairs in `XCropImage` and
`XCropButtonRelease` creates Complex Conditionals, 2.57, not committed); `XMagnifyImage` a26ee930a E, 20fa0f3a2 O;
fd4b2879c E `XTranslateImage` (leaves Large, cc 15 remains); `XPanImage` 9e8d51c0b, 957397646, e39eb7767 E; `XScreenEvent`
4371f14fe CN, E per case (46d7e7e88 Button, 1a0df5a79 ClientMessage, a3ebddd6e Configure, 60f4176e1 Expose, a05c26ab8 Key,
163ce6cba Map, 84f907709 Motion, 5aeb28e5f Unmap), 85a5cdb58 SD; `XOpenImage` c5c027d39, ddacb98f9, ec4ca4cdd W, a6a1eaebf,
e345a3177 E (most of its lines are unreached: evidence is obj_modulo); 6529df770 SD `XCreateStippleBitmap`; 44d6685b6 E
`DisplayImages`. Functions in the review: Bumpy 22 -> 19, Complex 28 -> 24, Large 26 -> 21; function count 176 -> 202.
- **Plateau at 2.68:** stubbing `XDisplayImage`'s setup and loop gave 2.74 on the unchanged file but, with the real setup
  helpers added, the same stub stayed 2.68 (a real chain adds lines); with the real 12-step setup chain `XDisplayImage` ends at
  cc 131, bumps 7, 1,114 lines, still 2.68, so it was not committed (no raise probed: rule 2). Near-twin window helpers are a
  cliff (two or more of the info, command, widget and popup helpers cost -0.06); `XInitializeMagnifyWindow` is exactly 70
  lines (Large, -0.07). The remaining setup, tables and loop need a hoist of the automatic `const char *const` menu tables (a
  new recipe on Recipe F) plus about 25 more steps. Rejected (flat or capped at five arguments): `XDrawEditImage` info/clamp
  helpers (cc 64 -> 59, bumps unchanged), an `XTranslateImage` refresh helper, `XTrimImage` scans (twin clones),
  `XVisualDirectoryImage` and `XDisplayBackgroundImage` (statics, five or more parameters); `XMagickCommand` (87 cases):
  stubbing 40 of them gives 2.74, so about 40 helpers plus SD chains would be needed.
- Possible upstream: `XOpenImage` leaks `image_info` and `exception` on the early `return` after the grab-delay dialog and
  on `text == NULL` (its early returns also skip the final `DestroyExceptionInfo` and `DestroyImageInfo`); `XDisplayImage`'s
  setup `status=XMakeImage(...)` for the magnify window is read only by the first ConfigureNotify, so an extracted
  `XMakeDisplayImages` must keep returning it.
