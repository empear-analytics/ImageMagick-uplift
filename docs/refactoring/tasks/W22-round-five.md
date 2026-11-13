# W22 session (2026-11-13 day): round five

PR 98 merged at its head 7c0d88744 (rounds one to three); rounds four and the Opus round, plus the `layer.c` MSVC fix (74c9f86d8), were in draft PR 104. This round (four lanes, one on
Sonnet 5.5 and three on Opus 5.5, two Sonnet reviewers per lane) is cut from `campaign/W22` 74c9f86d8. Gate on the merged tree: whole catalogue 15,955 cases, 0 diverged (baseline e5affc3c52);
wide catalogue 14,200, 0 diverged; `ci_linux_warnings.sh --campaign-a` OK on 92 files in seven configurations; `playbook_lint` 0 problems; the `-DNDEBUG -Wunused-parameter -Wunused-variable`
scan of every changed file against b772f03dc finds only campaign B's `annotate.c` and the `layer.c` helper already fixed for MSVC. Sweep: campaign A mean **9.12**, 44 at 10.0.

| File | Score |
| --- | --- |
| `composite.c` | 3.69 -> **3.90** (Opus; new Recipes RD, HN, GS on the parked chain) |
| `accelerate.c` | 3.27 -> **3.71** (Opus; new Recipe WP; Code Duplication gone) |
| `widget.c` | 7.31 -> **7.78** (Sonnet; `XNoticeWidget` has no finding; SD amendment for unreached functions) |
| `morphology.c` | 6.70 -> **6.93** (Opus; `AcquireKernelBuiltIn` and `MorphologyApply`; Recipe V step 1 amendment) |
| `distort.c` | 7.70 (no commit) |

**composite.c (Opus, lane c).** The 18 flat commits parked since W19c (Recipes RH, MP, the RH step 8 amendment; `SetCompositeCoverage`, `CompositeAlphaChannel`, `CompositeImagePixel`, the five `Is*Operator`
predicates merged by MP, `SetVirtualCompositeChannels`, `CompositeImageChannels`, the `BlendRequest` struct for Saliency/Seamless) are cherry-picked; then three new recipes, each in its own playbook commit
before its use: **RD** (8846c9683, 3.69 -> 3.58, `--no-verify`, the gate refused the probed first step): one relax driver for `SaliencyBlendImage` and `SeamlessBlendImage` taking a struct with the differing
images, the callers' relax and residual images by address, and one NULL-terminated list per failure stage of the caller's own locals in the caller's own release order, so every failure path frees the same images in the same order;
**HN** (4faeb63b5, 3.58 -> 3.70): Seamless's crop, sum and mean head becomes a helper that returns the mean image or NULL; **GS** (c048ee4de, 3.70 -> 3.90): the `FreeDivergentResources` macro of `BlendDivergentImage` becomes three
gradient sets, each released by its own function in the macro's order. `Number of Functions in a Single Module` is now reported from 8846c9683 on (the 2026-11-07 ruling allows it because the chain lifts the file). Callgrind of the whole
program at the tip against the base: Atop +1.45%, Over +1.24%, In +1.39%, Dissolve +0.86%, Saliency +0.27%, Seamless +0.30%. Left: `CompositeChannelColor` (cc 188, 802 lines; stub 6.28) and `CompositeImage` (cc 72; stub 5.01; both stubbed 8.28)
need the owner (the per-pixel compose switch on the hot path; nine caller variables written by the setup switch; progress-tail pragma blocks).

**accelerate.c (Opus, lane d).** New **Recipe WP** (fold wrapper prologues into one function and a guard in each callee): `getAccelerateEnvironment` runs the asserts in their original order, the trace log under the wrapper's own
`__func__` (or nothing for Despeckle, LocalContrast and WaveletDenoise), the check chosen by an `AccelerateCheck` selector (basic, RGBA, or RGBA then histogram with the same short-circuit), then `getOpenCLEnvironment` only when the check passes, so
`InitializeOpenCL` runs in exactly the calls where it ran before; each static `Compute*` worker starts with `if (clEnv == NULL) return F;` ahead of its own asserts and acquisitions; each converted wrapper is one statement. Blur (flat), Contrast (3.44),
Despeckle (flat), Equalize (3.71, Code Duplication gone). `Function`, `LocalContrast`, `RotationalBlur`, `WaveletDenoise` and `ContrastStretch` were not converted (flat, no finding). The five giant `Compute*` functions: no whole-function or shared-skeleton lever (every probe 3.71;
re-using the release helpers everywhere brings Code Duplication back, 3.44). Each worker's NULL guard adds 1 to cc and 2 lines to four functions that were already Complex and Large. The four logging wrappers now log the helper's `__LINE__`.

**widget.c `XNoticeWidget` (Sonnet, lane a).** Because no case executes `XNoticeWidget`, the lane wrote a new form, **Recipe SD, Unreached functions** (own playbook commit bf14aa98e), with `tools/switch_split_check.py` (the label-partition script,
20 planted-defect tests and seven plants on the real file) and an AGENTS.md line (f5f9085ba), and used it: SD twice (`RespondToNoticePointerEvents`, `RespondToNoticeWindowEvents`) and E step 7 (`DispatchNoticeEvent`, whole object file IDENTICAL). 7.31 -> 7.78. Reach: 0 cases and
0 of 113 lines in a fresh wide linecov. **The AGENTS.md line is the lane's addition for the owner's confirmation: it adds SD's unreached form to the guard-level evidence list.** The new tool scores 4.03 itself (the pre-commit gate scores only `.c`/`.cpp`); both new tools were created mode 755 where their siblings are 644.
`widget.c` is now held only by the argument counts of six `MagickPrivate` functions and Number of Functions (368).

**morphology.c (Opus, lane b).** `AcquireKernelBuiltIn`: the Blur case's ranked Gaussian loop becomes `AddRankedBlurGaussian`, and the Blur case body, with its `#if 1`/`#else` pair moved verbatim (the 2026-11-12 ruling, same pattern as `AcquireCometKernel`), becomes
`AcquireBlurKernel` (Bumpy Road gone from the whole file; cc 56 -> 50, LoC 140 -> 91); `MorphologyApply`: the `error_cleanup` tests (`DiscardMorphologyResult`; `Verified: guard`, no case executes it, obj_identical IDENTICAL), the stage-limit lookup, and a Recipe V step 1 amendment (a second label reached at once
after the block). Left: the first switch's 20 `#if 0` labels plus the 16-label break group (no recipe may delete the labels; moving them whole makes a cc-41 helper that is not a pure label run); `MorphologyApply` Large Method at LoC 73 (a probe dropping two more lines scored 7.04, but no recipe step frees them);
`MorphologyImage` has 5 public arguments. `distort.c` `poly_basis_dx`: three forms (an exponent table with product loops: +20.9%; a branch-free table of factors: +12.6%; the six low labels kept and the rest sent to `PolyBasisFnCubic`/`Quartic`: +20.9%, or +9.9% with `static inline`) all pass a bitwise check but are too slow on
`-distort Polynomial` order 5 (clang stops inlining dx and dy into `MapPolynomialPoint`); nothing committed.

**Reviewer notes.** Lane c: commit 6bb155636 (`CompositeImageChannels`, two `continue` to `return` rewrites) says `Verified: oracle`, but a branch-level coverage run shows both rewritten exits are taken by no case (read-correct, not oracle-backed);
the `guard OK` in six messages is a WARN (a literal `0.0` moved, an initialiser dropped); `--no-verify` on 8846c9683; the RD commit changes two functions (Saliency and Seamless) in one commit; RH and MP add per-row statements inside the `omp parallel for` body of `CompositeImage`
(composite.c is not on the 2026-11-12 OpenMP list; read by hand: they write only a per-row local flag and no shared variable; confirm); the 18 cherry-picked commits cite pre-rebase hashes; the RH step 8 amendment may be a new recipe by the catalogue's own test. Lane d: no case enables event logging, so the moved trace
statement is covered by no case; the whole file sits inside `#if defined(MAGICKCORE_OPENCL_SUPPORT)` and is checked in the wide build. Lane b: stale reach counts quoted from an old lcov (the conclusions hold; LoC figures off by one).
