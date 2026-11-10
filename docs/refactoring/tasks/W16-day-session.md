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

## Lane a, second round: the wide-build files

accelerate.c 2.06 to 2.79 and animate.c 1.77 to 2.49 (12 refactor commits, recipes P, CN, N, Y and E; no new
recipe), each step built and checked with `WIDE=1` (build, ci_warnings, guard, `--function` on the wide catalogue);
whole wide catalogue 14,145 cases and narrow 15,900 cases, 0 diverged, on the lane branch. **Tool fix** (c4335c3bb):
`obj_identical.sh` and `obj_modulo.sh` printed `IDENTICAL` for any edit to accelerate.c because the narrow build
compiles nothing of it; they now take `WIDE=1` and print `NOTHING COMPILED` (exit 2) for an empty object.
**For the owner:** every edit is inside branches the wide build compiles (`MAGICKCORE_OPENCL_SUPPORT`,
`MAGICKCORE_X11_DELEGATE`), which AGENTS.md permits ("a branch this build does not compile" is what is forbidden);
no `#if`, `#else` or `#ifdef RECREATEBUFFER` block was moved or edited. Please confirm that the playbook's
*Preprocessor branches* text is read that way for the six wide-build files, and say whether a split of accelerate.c
may carry its file-level `#if` into the new file. **Upstream (accelerate.c):** ContrastStretch and Equalize throw after
acquiring the OpenCL device and queue (leak) and may call `DestroyCacheView` on a NULL view; ContrastStretch searches
all four channels with `histogram[i].s[2]`; Despeckle does not check its HullPass kernels for NULL; ComputeResizeImage
passes `(cl_int) image->rows` where other calls pass `(cl_uint)`; LocalContrast sets the `cl_int` `iRadius` with
`sizeof(unsigned int)`.

## Lane b, third pass: compare.c measured, nothing committed

The long compare.c chain was scored end to end before any commit; no legal plan whose helpers carry no finding rises
above 2.97 (see its plateau row). Upstream: FUZZ, MSE and MEPP do not `memset` `similarity` while the other Get
functions do; comment typos ("criss-correlation", "structual").

## Lane b, sweep of the 9.0-9.99 files

25 campaign A files between 9.0 and 9.99 were reviewed against today's recipes. Only transform.c had a step left
(9.09 to 9.38, `ExcerptImage` at the unreached level). The others are held by public argument counts, Lines of Code,
or blockers already measured: channel.c `ChannelFxImage` (two exit kinds no recipe takes together),
colorspace-convert.c `ConvertRGBToGeneric` (per-pixel dispatch to callees of different signatures), resize-image.c
`ResizeImage` (OpenCL `#if`), segment.c (see its row). Files held only by Lines of Code: colorspace.c, statistic.c,
vision.c, transform.c (9.38 each).

## Lane d, morphology.c: Recipe ST amended

**ST step 7** (9af7c5f1a): when a chain step would take more than four parameters it takes a Recipe N input struct;
the caller fills it in the step 5 group directly before the call, and a later step moving that group leaves the fill
in the caller's new `default:` group. morphology.c 3.04 to 3.10 (11 flat steps inside a measured plan, then one rise;
see its plateau row). Added after the owner's approval of the W16 text; not yet confirmed. **Owner question:** is the
`#else` branch of an `#if 1` (Blur in `AcquireKernelBuiltIn`) as never-compiled as `#if 0`, so that the exception lets it
move? **Notes:** the dead `#else` text of Blur names `i`, whose compiled declaration an E step removed (said in that
commit); the unreachable `break;` after the No-Op default now sits in `AcquireDistanceKernel`.

## Lane c, third round: feature-texture.c, Recipe LS, E step 3 amended

**Recipe LS** (fdae9774e): copies of `u=0; while (a[u].m != e) u++;` differing in the integer member and value become
`u=FindName(a,offsetof(S,m),e);`, a `static inline` finder reading the member through its offset. **For review:** the
first `offsetof` form in the playbook. **E step 3 amendment** (b1dc4bc6f): an array element updated member by member
outside every OpenMP region may be passed as `&e`. Both came after the owner's approval of the W16 text; not yet
confirmed. feature-texture.c 5.98 to 7.39 (10 refactor commits). **Timing, for the owner:** for commit 13302b12e (E
element form) the median CPU time read 4.8% to 64% slower in repeated runs under a load average of 10-16 from the
other lanes (single runs 1.7 s to 9.7 s for one binary); minimum times were within 0.5%, valgrind instruction counts
equal (4,869,065,359 to 4,868,105,857), `nm` shows the helper inlined, and the whole chain against de97078b7 times
2.040 s to 2.040 s (instructions 4,904,726,881 to 4,871,441,653). The step was kept on that evidence; by the letter of
the median rule it would not be. **Upstream (GetImageFeatures):** a failed co-occurrence build leaks Q, its rows,
`sum` and the density arrays and reports a cache failure as MemoryAllocationFailed; the unwind `for (i--; ...)` skips
the failing row index; `density_xy` is cleared twice with different lengths; a benign race writes equal values into
`grays`; a gray-level search that finds nothing would run past `grays`.

## Lane b, splits: transform.c and vision.c reach 10.0

Recipe S splits (2026-11-07 ruling: the new file scores at least the original): transform.c to 10.0 with
`transform-region.c` 9.38; vision.c to 10.0 with `vision-components.c` 9.38. `tools/split_source.py` now finds the
include block without its comment (6a74372ef; no refusal rule changed). Registered in Makefile.am, Makefile.in,
Make.com and (transform-region.c only, as vision.c is not listed there) scripts/format_c_api_docs.
**Windows build (answers the open question):** the repository holds no Windows project files; CI generates them from
ImageMagick/Configure, and its MSVC compile line already lists every MagickCore .c file including earlier split files,
so splits need no Windows entries (earlier commit messages saying otherwise were wrong). Separately, the Windows jobs
fail on main since #86 with C2220 (warnings as errors) in annotate.c, colorspace.c, compare.c and composite.c.

## Lane d, morphology.c second leg: Recipe KT

**Recipe KT** (d4de2635e): a `switch` on an integer whose groups each assign `v=F("constant");` under sparse labels
becomes a `static const` table of `{key, constant}` and a loop; form A when `default:` shares a group (that element
last), form B when it has its own group (run unchanged after the loop). Checked by script against the groups. Added
after the owner's approval of the W16 text; not yet confirmed. Three flat steps (rule 2). The rest of the leg takes the
temporary drop to 3.02 that the 2026-10-29 owner rule allows for an early step of a plan measured end to end.
Upstream: `ScaleKernelInfo`'s negative-factor swap sets `kernel->minimum = 1;` where `= t` looks intended.

## Lane a, third round: opencl.c out of the Red band

opencl.c 2.85 to 5.03 and widget.c 1.19 to 1.28 (wide build; 43 refactor commits), each step built, warned, guarded,
oracle-checked and object-compared with `WIDE=1`. **Recipe EC** (9aac62d8e): a `||` chain comparing one
side-effect-free integer with distinct constants by `==` becomes a `static inline` label-run predicate (a `switch`
returning `MagickTrue`); duplicate labels would not compile. **TB amendments** (a912c5f3f, 2c9dbf7a0): character
constants of the basic character set as membership labels, and the guard wording. Added after the owner's approval of
the W16 text; not yet confirmed. **Owner questions:** may Recipe LB gain an `S break;` exit form (the DOCTYPE scan,
the same text in color, configure, delegate, locale, log, policy and type.c)? May a recipe add same-type wrappers so CF
merges twins calling library function pointers of different types? **Note:** commit ed27c19c5's message says
`RunOpenCLBenchmark` left Bumpy Road; it left Complex Method, and Bumpy Road only at dac546fe6, whose message corrects
it. **Upstream:** `RunOpenCLBenchmark` never destroys its `exception` or `imageInfo`; `LoadOpenCLDeviceBenchmark` leaks
the previous entry when a second `<device` arrives before `/>`; `RequestOpenCLDevice`'s single-device path never
increments `requested`.

## CI "Main" repaired for campaign A files

CI's "Main" workflow had failed on main since about #86. Campaign A refactors had introduced MSVC errors (effect-blur.c
C2440, shear-deskew.c C2664), `restrict`-qualifier discards that gcc and clang reject under `-Werror` (effect-blur.c,
colorspace.c, quantize.c), unused parameters (colorspace.c, quantize-posterize.c), default-only switches (compare.c)
and a shadowing local (fourier.c). Seven `fix` commits repair them, each with object code IDENTICAL, the guard passing,
the oracle on the touched functions and Code Health unchanged. New `tools/ci_linux_warnings.sh` (2265bd949) builds four
trees with CI's exact Linux flags (gcc and clang, without and with delegates) plus `-Wshadow`, discarded-qualifier
warnings and a scan for default-only switches; `tools/ci_warnings.sh` runs it, so every lane's verify loop does. The
merged W16 tree passes it on all 76 campaign A files. **For the campaign B side:** blob.c:3525-3526 and 4096 (unused
parameters and variable; the Linux jobs' first stop), type.c:1222 and 1233 (discarded `const`, MSVC C2664), type.c:790,
annotate.c, pixel-export.c and pixel-import.c (shadowing), utility.c:457 (implicit fallthrough), pixel-export.c:3412 and
pixel-import.c:3574 (default-only switches), utility.c:229 and quantum-import.c:4788, 5235 (MSVC only). Not reproducible
here: MSVC C4702 and Windows-only branches.

## Lane c, fx.c: Recipe EM

**Recipe EM** (3052b9eb1): a `static` predicate returning `(C) ? MagickTrue : MagickFalse` where C is an `==` chain of
a parameter against distinct enumeration constants (alone, after `a &&`, or before `|| r`) becomes a `static const`
table and a membership loop. Added after the owner's approval of the W16 text; not yet confirmed. **For the owner:**
EM and lane a's EC (an `==` chain becomes a label-run `switch` predicate) cover overlapping shapes; one may be enough.
Commit eedc41bae uses Recipe N for a tail reading three caller variables, below N's *Use when* of five (lane a's
6f54268cc did the same): should E admit these tails, or N's *Use when* be widened? fx.c 3.62 to 3.73.
**Upstream:** `ExecuteRPN`'s statistic cases test `channel > 0` where `ImageStat` tests `channel >= 0`; `aExtent` reads
`img->extent` where `ImageStat` reads `GetBlobSize(pfx->image)` and ignores `ImgNum`; the "NoSuchImage" message prints a
negative `ImgNum` through `%lu`.

## Lane b, layer.c and distort.c: Recipe FI step 8

**FI step 8** (6e7913783): a member-by-member fill far from its declaration may become a `const` initialiser when every
value is the address of a top-of-function variable (declaration moves to the end of the declarations) or an
identifier nothing writes between the innermost block's top and the fill (declaration moves to that block's top);
object code must stay IDENTICAL. Added after the owner's approval of the W16 text; not yet confirmed. layer.c 7.84 to
7.98; distort.c unchanged (its only rising step is too slow). **Owner questions:** (1) does the `#if 0` exception reach
`#if M` when M is `#define`d 0 unconditionally in the same file (layer.c `DEBUG_OPT_FRAME`; an earlier commit,
f4a8418dd of 2026-10-30, already moved two such blocks)? (2) may a label run whose labels all share the `default:`
body lose its labels (distort.c `SetDistortNoBestFit`, +0.19)? **Note:** `poly_basis_dx` returns derivative terms
without their power factors (`x*x` for the x cubed term); presumably the coefficients absorb them.

## Lane d, morphology.c third leg: Recipes PL and RT, and an oracle gap

**Recipe PL** (524c7a893): `switch` groups whose whole body is the same leaving statement peel into an LP-form predicate
tested before the `switch`. **Recipe RT** (8036abb58): three or more adjacent copies differing in one constant, each
leaving through the same `return(R);`, become a helper looping over a `static const` table. Added after the owner's
approval of the W16 text; not yet confirmed. morphology.c 3.10 to 3.16 (30 steps; step 1 a temporary drop with
`--no-verify` inside the measured plan). **Oracle gap:** a generator bug in the lane's first pass produced an empty TB
table, so `IsRotationUselessKernel` always returned false, and the oracle still passed all 1,557 RotateKernelInfo cases
(rotating a symmetric kernel changes no checked output). The lane caught it by script, reset its unpushed branch and
redid steps 5-30; the main session checked the merged table against the original 13 labels. The early return in
`RotateKernelInfo` needs a case that detects its removal (for the harness owner). **Owner questions:** FreiChen's basis
groups: allow a table-driven merge, or leave it at cc 16? **Upstream:** `ScaleKernelInfo`'s negative-factor swap sets
`kernel->minimum = 1;` where `= t` looks intended (now in `ApplyKernelScaling`, moved verbatim).

## Lane a, widget.c second pass

widget.c 1.28 to 1.44 (9 refactor commits, wide build). Reach was checked against the wide line coverage
(`build-oracle/oracle-wide.lcov`, 2026-10-28): no case reaches `XScreenEvent`'s Expose or ButtonPress cases or the popup
mapping run, so those steps rest on object code IDENTICAL. **Correction:** commit 2acc79850 (CN in `XScreenEvent`)
carries `Verified: oracle` citing 237 cases; that is true of the function, not of the changed lines, whose evidence is
the guard and object code IDENTICAL (what CN requires). **Owner questions:** (1) may MA admit a copy no case reaches when
only the caller's object code differs in register allocation, or should such twins stay one-sided? (2) should a
line-coverage reach check be required for oracle-level claims in the wide build, given that its lcov dates from
2026-10-28? **Tool note:** `tools/obj_identical.sh` prints "otool: not found" on Linux before DIFFERENT (the verdict is
right).

## Lane c, fx.c second leg: RV step 6, N step 8, BR step 8

fx.c 3.73 to 4.61 (14 refactor commits, every one rising), the ceiling with everything but `ExecuteRPN` done.
**RV step 6** (c164a865f): a caller of another return type passes `T *result`; the block may write up to four plain
locals through pointers; more than four parameters go in a const struct, and an earlier helper's struct may be passed
on. **N step 8** (309c8444b): statements of one function body count as siblings, so a later block running to the
function's end may pass an earlier sibling's struct on. **BR step 8** (1e37f5588): a block may write two to four
variables through a const targets struct and take inputs from at most two structs the caller already fills. Added after
the owner's approval of the W16 text; not yet confirmed. **Owner questions:** the struct fills are long
(`FunctionClosing` has 13 members, `ExpressionTargets` 6): is that readability cost acceptable? `ExecuteRPN`'s parked
per-pixel chain is the only lever above 4.61. **Upstream:** a bare `gray` is rejected in `-fx` (`GetConstantColour`'s
fall-through to the named colour never runs unless `(` follows); `gray(...)` after a failed query uses `colour` as the
failed `QueryColorCompliance` left it; `StartEleNdx` is a dead store.

## Lane c, composite.c third pass: a method table is faster but needs rulings

No non-chained dispatch form passes (see the plateau row). **Owner questions:** (1) may a per-pixel path use a
function-pointer method table when it is measured faster (composite.c: 11-15% faster, 17-21% fewer instructions on
Multiply, Xor, Darken, Overlay, SoftLight; recipe text sketched as "MD, Replace an Operator Selection with a Method
Table")? (2) may a step accept a transient Code Duplication finding when its measured plan removes it (41 functions
here, about 40 steps)? (3) may MT take three or more twins? **Note:** the oracle tolerates `__LINE__` in ImageMagick's
warning texts (`compose:args` on Blur), so refactoring changes stderr there.

## Lane b, xwindow.c (wide build)

xwindow.c 1.10 to 1.57 (22 refactor commits, existing recipes only). **Evidence note:** for `XGetWindowImage` and
`XImportImage`, 159 of the 173 cases that reach them exit non-zero on the baseline; commits 6f0d74a78 (P x3 in
`XGetWindowImage`) and afaae27fe (B `WindowImageContext`) change object code and rest on that weak function-level
reach. **Owner questions:** may a helper for code inside a compiled `#if defined(MAGICKCORE_HAVE_SHARED_MEMORY)` branch
be defined outside the `#if` (unused in an X build without shared memory)? May a block that logs through
`GetMagickModule()` move (it changes the function name in `-debug X11` output)? **Upstream:** `XMakeImageLSBFirst` tests
`window->immutable == MagickFalse` where `XMakeImageMSBFirst` tests `!= MagickFalse`; `XAnnotateImage` reuses the parsed
`y` offset as its row counter and re-parses afterwards, which leaves `y = rows` when the geometry has no y offset;
`XGetResourceInfo`'s `font_name[9]`/`[10]` and `pen_colors[9]`/`[10]` repeat "font0"/"pen0".

## Lane c, sweep of the 7-9 band

13 files reviewed against today's recipes; feature-texture.c 7.39 to 7.53 and fourier.c 6.45 to 6.52, the rest
unchanged (held by `#if defined` code, public signatures, file length or open owner questions). **Not merged:** a first
version extracted the column loop of feature-texture.c's parallel grays fill, which writes the shared `grays` table
(not the iteration's own slot) from inside the region; the lane rebuilt without it. **Owner questions:** (1) may a block
inside a parallel loop that writes a shared table through a pointer, with the stores exactly as upstream, be extracted
(feature-texture.c to 7.76)? (2) may CF admit whole parallel loops with an N-style input struct (feature-texture.c's
direction loops, ceiling 8.04)? (3) may a parameter read only by an OpenMP clause be marked `magick_unused`
(fourier.c 6.56)? (4) colorspace-convert.c `ConvertRGBToGeneric`: its cases differ only in the callee; a table of
function pointers would put an indirect call on a per-pixel path (as composite.c's method table). **N step 8
amendment** (ce647f64a): a struct the caller already fills for another static function's call may serve a later block
when every variable the block reads is a member. **Upstream:** the parallel grays fill races (threads write equal values
into shared `grays` entries); `p+=...;;` in the co-occurrence loop.

## Lane d, morphology.c fourth leg: MorphologyPrimitive clean

morphology.c 3.16 to 4.48 (23 refactor commits, every step but one H rising; helpers all inlined, no indirect calls,
no forced-inline attribute). **Amendments** (59b543867): MT step 8 (copies reading a struct the caller already fills
take `const Name *` beside `side`; a callee difference uses CF's `method`), CF step 4 (inputs through the shared
struct), N step 4 (const locals or member form chosen member by member). Added after the owner's approval of the W16
text; not yet confirmed. **Timing note:** step 11 (BlendKernelColumn) measured median +1.6%, minima +4% under load, with
fewer instructions (318.12M to 317.78M); kept on instruction counts. **Upstream:** the vertical-convolve column path
returns 0 rather than -1 on failure, so MorphologyApply misses a failed column pass; ErodeIntensity and DilateIntensity
leave before `changes[id]++` and always report 0 changes; the DilateIntensity comment says the kernel is not reflected
where the code reflects it.
