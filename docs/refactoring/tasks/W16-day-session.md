# W16 - campaign A, parallel lanes (2026-11-10)

Campaign A (Windows desktop, WSL2), branch `campaign/W16` cut from `main` 72f10d28d after #91. Four lanes in git
worktrees, as in W14. The owner's rulings of 2026-11-07 apply (a file may pass 110 functions when the chain lifts it;
label-run helpers may be Complex Method; Recipe S splits the tool accepts when the new file scores at least the
original; no new OpenMP exceptions). Start: campaign A mean 8.19, 27 of 74 files at 10.0.

## Lane d, first batch
- `enhance-level-colors.c` 9.36 -> 9.68 (e226d3e2d, cherry-picked).
- **Parked for the owner, not in the PR:** two `shear.c` commits that take it 9.17 -> 10.0 (local branch
  `w16-shear-enum`). The first (c74c70fd5) folds Recipes PC, N and a D reuse into one commit and replaces
  `YShearImage`'s local `UP`/`DOWN` enum and switch with the file's existing `ShearDirection` and an `if`
  (LEFT = UP, RIGHT = DOWN); behaviour is the same (43 cases, 0 diverged) but removing the enum is a rewrite no
  recipe covers. Merge it if you accept that.
- No step on `vision.c`, `paint.c`, `statistic.c`, `effect.c`, `feature.c`, `resize.c`, `visual-effects.c`: their
  remaining findings are public argument counts and file length (or need B on an unreached function).

## Lane b: channel.c 8.58 -> 9.28, effect-blur.c 8.22 -> 8.68, resize-image.c 8.32 -> 8.78 (Opus, 8 commits)

One temporary drop: fcf5f64f4 (`resize-image.c` 8.32 -> 8.25, `--no-verify`, step 1 of a measured plan; the next step
reaches 8.78). Upstream, not changed: `ChannelFxImage` leaks the image list when `AppendChannelCanvas` fails;
`SharpenAdaptiveRows` releases its kernels with an inline copy of `RelinquishAdaptiveKernelSet`'s body.

## Lane d, second batch: threshold.c, resample.c, colorspace-convert.c at plateau (Sonnet, no commits)

## Lane a: enhance-tone.c 5.99 -> 6.99, fourier.c 6.33 -> 6.45 (Opus, 11 commits)

**For the owner:** 6f54268cc uses Recipe N for a tail that reads only three caller variables (N's *Use when* asks
for five or more), because whole-parallel-loop tails go to N or Z, not E; should E admit whole loops under N step 1's
conditions? dfad29877 and e7f09ffbc use I step 2's const-addresses form for three variables. **Upstream, likely a
bug, not changed:** `EqualizeImage`'s `memset(black,0,sizeof(*black))` and `memset(white,...)` clear one double
each, not the whole arrays.

## Second looks under the 2026-11-07 rulings: composite.c, morphology.c, compare.c unchanged (Opus, no commits)

All three stay where they were (3.46, 3.04, 2.97); see their plateau rows. **Owner options that would move them:**
merging `CompositeCopyImage`/`CompositeIntensityImage` through a row-operation parameter; helpers writing OpenMP
reduction variables through pointers (`BlendRMSEResidual`); merging mirrored forward/reverse twins
(`morphology.c`). **Upstream, likely a real bug, not changed:** `BlendRMSEResidual` resets `channel_residual=0.0`
on every row inside its reduction loop, so only the last row's sum survives.

## Lane c: colorspace.c 7.44 -> 8.62, quantize.c 7.80 -> 8.74 (Opus, 24 commits + 2 playbook)

New recipes under the mandate, not yet ruled on: **Recipe LP** (eb285eae6, Name a Label Set as a Predicate: a
two-group `switch` whose single group returns on every path becomes `if (Pred(e) != MagickFalse)` with a predicate
holding the labels; equal label sets may share it) and **Recipe SC** (bd93823d5, Extract Sibling Blocks Differing in
Several Constants: SV generalised to two or more constants, carried in a const struct; copies identical by script
once the constants are named by role). **Note:** `Riemersma`'s recursion now passes through one extra function per
level. Upstream, not changed: `TransformsRGBByTables` takes `status` from `TransformsRGBPixels` and returns
`MagickTrue` anyway.

## Lanes with Recipes SC and LP (Opus and Sonnet)

- **Lane b:** `transform.c` 7.85 -> 8.95 (6 commits); `effect-blur.c` unchanged. Upstream, not changed: `MotionBlurImage`
  releases `kernel` and `offset` before destroying `blur_image`, the reverse of `CloneDirectImage`'s order.
- **Lane a:** `morphology.c` unchanged; a script check shows every mirrored twin differs in operators, expressions or
  called function, so SC does not apply.
- **Lane c:** `visual-effects.c`, `vision.c`, `enhance-tone.c`, `visual-effects-plasma.c` unchanged; the twins span two
  functions and differ in non-constants. Upstream, not changed: `GetGammaLumaIntensity`'s Rec709 case repeats
  `EncodeGrayscaleGamma`'s body inline where the Rec601 case calls it.
- **Lane d:** no further LP site in the campaign A files (every candidate switch ends its groups in `break` or has a
  `default:` that does work).

## Where W16 leaves campaign A

The remaining low files are held by things only an owner ruling can open: twins that differ in operators or called
functions (morphology, threshold, resize-image pixel functions, enhance-tone, effect-blur), `#if defined`/`#else` code
(fourier, resample, attribute-depth), per-iteration array slots (feature-texture), OpenMP reduction variables written
by a helper (composite), unreached functions needing B (paint, segment), and the parked chains (`w16-shear-enum`,
`w14-fourier-ifdef`, `w14-fx-annealed`, `w14-tone-splits`).

## Evolved recipes (owner OK, 2026-11-10): lane d

**Recipe B and N amendment** (2f5e79de4): B and N may run on unreached functions when the guard is clean, a new
`tools/struct_fill_check.py` checks the fill and the bindings, and a new `tools/obj_modulo.sh` shows every other
function's object code unchanged (IDENTICAL or MODULO-IDENTICAL, addresses aside, the helper the only new function).
Planted errors (a wrong binding, a wrong fill, a flipped comparison, a changed literal) were each caught. **For review:**
the same commit also updates AGENTS.md's list of evidence levels and adds the two tools (tools would normally be a
commit of their own). **Recipe LE** (368e86789): replace a function-local enum with a file-scope enum whose constants
map one to one, values only compared or switched on, object code IDENTICAL. The parked branch `w16-shear-enum` is
superseded and can be deleted. Note: N's *Careful* recommends the member form to avoid clones while the brief forbids
changing forms to hide a clone; the lane did not use the member form.

## Evolved recipes (owner OK, 2026-11-10): lanes b and c

**Lane b, twin merging.** Recipe **CF** (merge twins differing only in the function they call: a `typedef`, a
`static inline` body with a `const` method parameter, callees passed by name, a `const void *` input where the callees
differ only in that parameter); Recipe **RL** (share a parallel row loop through a row function: one non-inline
function holds the loop with its pragma text unchanged, each caller passes a `const` struct `{tag, method,
arguments}`), amended so that a loop may end at its view's destruction; Recipe **MT** (merge mirrored twins through a
constant selector: each differing run becomes `((side == A) ? runA : runB)` with the runs verbatim, checked by folding).
threshold.c 8.03 to 9.09 and enhance-tone.c 6.99 to 8.47. **For review:** RL makes the row work an indirect call once per
row (`objdump`: the only `call *` is in `ThresholdImageRows`); a per-pixel indirect call was probed and rejected (13%
slower). Timings (baseline vs candidate, CPU medians, alternating): threshold RL 0.860 s vs 0.860 s, CF rows 0.870 s vs
0.870 s; enhance-tone MT 3.09 s vs 3.01 s, RL 1.910 s vs 1.900 s. RL step 2 lets an inline column loop become a row
function by Recipe E in the same commit. Three threshold.c steps were flat (rule 2) before the 9.09 step.

**Lane c, OpenMP blockers.** Recipe **IS** (extract a block in one iteration that writes only that iteration's slots,
`x[i]` or an index computed only from `i`, passed as pointers; the whole region is read to check every access uses its
own `i`); Recipe **RA** (extract a block that only applies its operator to at most two `reduction` variables, passed as
`&v`, the thread's private copy). Neither touches a pragma. feature-texture.c 5.25 to 5.98, composite.c 3.46 to 3.66,
vision.c 9.31 to 9.38. **For review:** feature-texture.c now carries *Overall Code Complexity* (W11 said to report such
a step rather than commit it; it was committed because the score rose and a probe shows the finding is unavoidable).
Timings: features of a 128x128 noise image 0.96 s vs 0.92 s; seamless-blend 1.77 s both.

**Upstream oddities noted, not changed:** `BilevelImage` passes `progress++` to the progress call right after the atomic
`progress++` (two increments a row); in `GetImageFeatures` the Q green term tests and divides by
`density_y[x].direction[i].red`; `density_xy` is not cleared between the sum-average and difference passes; `p+=...;;`
in the co-occurrence loop.

## Wide oracle made trustworthy: lane a (tooling only)

The 153 base-vs-cand divergences between two wide builds of one commit were not behaviour: 129 came from a baseline
cache key collision (`case_key` ignored `env`, `x11` and `stdin`, so an OpenCL twin and its plain case, or one X11 case
on eight screens, shared one cached baseline), and 24 from configuration paths `BUILD_TREE_RE` did not normalise. Both
are fixed in `tools/oracle/oracle.py`; `HARNESS_VERSION` is 15, so every clone rebuilds its baseline once. Two wide builds
of the same commit now agree on all 14,145 wide cases (three runs), and planted changes in `accelerate.c` and `display.c`
are caught. Twelve interactive `xevents` drag cases remain load-sensitive (display maps its Info window under the first
press); they are listed in `cases.py` `LOAD_SENSITIVE` and settled by a rerun alone. **For the owner:** the proper fix
moves those presses below y=30 (new case ids, `casemap-wide.json` rebuilt); mutation kills in `display.c` that rest on
these cases may be false. The six X11/OpenCL files of campaign A can now be verified with `WIDE=1`.

## Evolved recipes (owner OK, 2026-11-10): lane d, second round

**Recipe I at the unreached level** (fc96f247c): I may run on a function no case reaches, `Verified: guard`, when
`tools/targets_fill_check.py` (2aea9fd61) shows the targets struct filled and bound member by member, the removed
lines and the helper body the same lines, and every lost exit back in the caller unchanged; `obj_modulo.sh` must print
IDENTICAL or MODULO-IDENTICAL, now also accepting a function the shortened caller inlines when its text is unchanged
(53792f5fb). **Recipe D when only the caller's object code differs** (4cbd6c55e): `tools/reuse_check.py` (d488fa330)
checks the call is the only added line and the removed lines are the helper body under the call's bindings. Planted
errors were caught by each tool. segment.c 7.14 to 9.02, transform.c 8.95 to 9.09. **For review:** both levels rest on
object code and text checks, not on behaviour cases; no other unreached campaign A function was found that they open.
Upstream: `GetImageDynamicThreshold` allocates `extrema[i]` with `sizeof(**histogram)` (harmless over-allocation).

## Lane b, second round: Recipe CF amended

**CF amendment** (f30299bcb): a copy may call functions in two roles (a conversion and its inverse), callees may be
declared in an included header, and a merged function with five or more parameters takes a `const` struct holding the
methods and enough read variables. `objdump` shows the calls stay direct once the function is inlined. enhance.c 7.72
to 9.09 (one rule 2 flat step, `2a44f3ea7`, inside the measured plan). compare.c stays 2.97 (see its plateau row).
Upstream: `GetMEPPSimilarity` reads the shared `maximum_error` outside its critical section while other threads write
it inside (a data race); `SIMFilterImageNaNs` carries `SIMSquareImage`'s comment; `GetSSIMSimularity` is misspelt.

## Owner approval (2026-11-10)

The owner approved the W16 playbook text: Recipes LP, SC, LE, CF (with its amendment), RL (with its amendment), MT, IS
and RA, and the unreached-function levels for Recipes B, N, I and D (with `tools/obj_modulo.sh`,
`tools/struct_fill_check.py`, `tools/targets_fill_check.py` and `tools/reuse_check.py`). Their Catalogue changes rows
already read Decided by `owner`. Still open: the AGENTS.md edit and tools bundled into the B/N amendment commit,
the *Overall Code Complexity* step in feature-texture.c, the xevents drag cases, and the older questions above.

## Lane c, second round: Recipes TI and MA, MT amended

**Recipe TI** (7f3eb3ee4): two static twin functions with one caller each merge into one `static inline` function
taking an enum side and a `const` struct of their read-only inputs; `nm` must show it inlined, otherwise it is declared
`static magick_attribute((__always_inline__)) inline`. **Recipe MA** (141a93848): sibling blocks that differ in
constants, a leading test operand and the member of one lvalue they update become calls passing `&e.m`. **MT step 4**
(8714830f8) may take a `const` struct pointer of the copy's inputs. resize-image.c 8.78 to 9.38, effect-blur.c 8.68
to 9.09, enhance-tone.c 8.47 to 9.09, layer.c 7.55 to 7.84. **For review:** TI on the resize pixel path depends on the
inlining: without it GCC kept the function out of line, 21-26% slower, and 12 cases diverged in NaN payloads only. A
compiler that ignores `magick_attribute` (it is empty outside GCC-compatible compilers) is not measured. These recipes
came after the owner's approval of the W16 text and are not yet confirmed.
Upstream: `mogrify -define quantum:format=floating-point -depth 32` on an 8-bit MIFF writes garbage pixels (NaN,
13119%), deterministic and in the baseline too.

## Lane d, third round: Recipe TB

**Recipe TB** (a596d4ee4): a Complex Method that is one `switch` whose labels only return constants becomes a
`static const` table, in membership form (`for` over the labels, `return(MagickTrue)`) or index form (labels 0..k,
`if ((size_t) n < count) return(T[n]);`, a negative `n` still reaching the default). No literal is added; a script
checks the table against the labels and values. Added after the owner's approval of the W16 text; not yet confirmed.
colorspace.c 8.62 to 9.38, distort.c 7.15 to 7.60. **Owner questions:** (1) may rule 1 admit a mixed-recipe set
whose steps each cross *Lines of Code* alone (magnify.c 8.62 to 8.81)? (2) may Recipe S and `split_source.py` accept
family-only `static const` tables and a numeric top-of-file macro (colorspace.c split: 10.0 and 9.38)?
**Upstream:** montage.c `MontageImageList` dereferences a NULL `image` in `ThrowImageException` after a failed
`CloneImage`, and reads `image` after `DestroyImage` on the `AcquireMontageStrings` failure; magnify.c
`SelectMagnifyMethod` switches on the case-sensitive first character before a case-insensitive `LocaleCompare`.
