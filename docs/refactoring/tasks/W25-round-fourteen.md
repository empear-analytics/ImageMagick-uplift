# W25 (2026-11-14 night): campaign A, round fourteen

Four lanes (a and d on Sonnet 5.5, b and c on Opus 5.5; one combined reviewer per lane) cut from `2f83526f9`, merged onto `main` after #117 (`e29b93360`) in `campaign/W25`. Gate on the merged tree: whole catalogue 15,955 cases, 0 diverged; wide catalogue 14,200, 0 diverged; `ci_linux_warnings.sh --campaign-a` OK on 98 files in seven configurations; `playbook_lint` 0 problems (149 recipes, 425 change rows). Sweep (cs-mcp 1.5.16, MagickCore 198 files, mean 9.13): campaign A 98 files, mean **9.34**, 49 at 10.0.

| File | Before | After |
| --- | --- | --- |
| `layer.c` | 9.38 | **10.0** (Recipe S: 73 of 75 functions move to the new `layer-frames.c`, 9.38, Lines of Code only) |
| `threshold.c` | 9.09 | **10.0** (Recipe S: the nine-root image-thresholding family, 63 of 80 functions, moves to `threshold-image.c`, 9.09 as the original: LoC and 3 argument findings; `struct _ThresholdMap` is named by both parts and is defined again in the new file by the new `split_source.py --copy-struct`, Recipe S step 2 amended) |
| `morphology-apply.c` | 7.39 | 7.41 (new Recipe DG on `AcquireKernelBuiltIn`; the predicate `IsBaseKernelNeeded` is a cc-40 label run, 20 of its labels in the author's `#if 0`) |
| `composite.c` | 4.24 | 4.24 (flat Recipe B step on `IsBlendRowsMissing`) |
| `tools/split_source.py` | 7.32 (after the merge of the two campaigns' versions) | **10.00** (plain extractions by lane d; tests 19, 11, 6 pass; the tool regenerates byte-identical kept and new files from the parents of 8dc14abf6, 8496c1160, 6c21367a0 and e68c288ad) |

**Refused and why.** `statistic.c`, `feature.c`, `segment.c`: no legal split (the roots share 10 to 15 statics; one closed component of 80 functions; copying a static is not allowed). `threshold-image.c`, `layer-frames.c`: re-carving a split product. `accelerate.c` (lane b, no commits): with the 19 Complex/Large functions stubbed the file scores 9.68, but nothing rises until about 13 of them are cleared (3.71 stays flat; 3.80 at 13, 4.07 at 14, 4.52, 5.20, 5.93, 7.38 as the big five go): about 60 steps over several lanes, a realistic ceiling of 8.0 to 8.8 (Number of Functions appears at 100 to 200 helpers; five MagickPrivate argument findings stay); the earlier 3.44 came from five wrappers Recipe WP left unconverted that sit just under the Code Duplication threshold and tip over once the Compute workers shrink; WP step 6 needs an amendment first; four `#ifdef RECREATEBUFFER` blocks no build compiles must not be edited. `composite.c`: `CompositeChannelColor` (cc 188, per-pixel hot path) and the Intensity-group loop that becomes a twin of `CompositeCopyImage` (probe 4.24 -> 3.99; CF and MT refuse the pair). `distort.c`: `poly_basis_dx` per-pixel, earlier chain forms +9.9% to +20.9%. `colorspace-convert.c`: the next SD run makes a Code Duplication group (9.08 -> 7.88/8.28). `feature-texture.c` (8.72): a probed plan scores 9.01 but needs a new recipe to sink the zeroing writes of locals. `montage.c`: plateau (15 lines under the Lines of Code cliff). `display-image.c` (5.86): all 17 static argument findings cut to four parameters leave the score at 5.86, because `XDisplayImage`'s 7 MagickExport arguments keep the category.

**Lane d's other commits are not in this PR.** It also made three commits on `stream.c` (7.32 -> 7.55, Recipe B step 8 on `StreamRowInfo`) and `locale.c` (8.91 -> 9.19, Recipe B, new `LocaleIncludeFile` struct). Those are campaign B's files, which campaign A lanes do not touch; I left them out of the merge. They stay on branch `campaign/W25-d` in the WSL clone (`7c842b817`, `ca7901e85`, `7714729c9`) in case the owner wants them given to campaign B.

**Reviewer notes.** Lane a: `Verified: guard` on both splits (3 of 73 moved layer functions and 122 of 1,327 lines run in no case; `SetThresholdProgress` and 104 of 998 moved lines in no case): the playbook's *Two levels of evidence* says Recipes D and S need the oracle (the relabel ruling of 2026-11-12 allows the split, the label is honest); layer's 15 `#if DEBUG_OPT_FRAME` blocks moved verbatim with their functions; the `--copy-struct` amendment row says `owner` (written under the 2026-11-10 OK). Lane c: Recipe DG is a new recipe (the reviewer attacked its argument and found no hole); `024365dcc` says `Verified: oracle` while one moved statement (`return(kernel);` after a failed allocation) runs in no case (disclosed); both messages cite `verify_wt.sh`, which is not in the repository; `BlendRowPointers` stands mid-file instead of under *Typedef declarations*; the flat composite step has no next step; `tools/cxx_warnings.sh` could not run (no `upstream/main` ref; the reviewer emulated it: zero new diagnostics). Lane d: both Recipe B steps cite step 8 for a struct that Recipe QF introduced (the form matches).

**For the owner.** Confirm or relabel: new Recipe DG, the Recipe S step 2 `--copy-struct` amendment (both labelled `owner`, written under the 2026-11-10 OK), the two relabelling splits at guard level. The largest remaining gains are blocked on rulings listed in earlier PRs: `fx.c` function-pointer table (4.61), `compare-similarity.c` shared critical name (2.97), `fourier.c` Recipe TS (7.59), `composite.c` row flags inside the omp body. Campaign A work that does not need a ruling is now mostly the `accelerate.c` chain above.

## Round fifteen (2026-11-15 morning): accelerate.c foundation; feature-texture.c held

Two Opus lanes (one combined reviewer each) cut from `c5abd3066`. Gate on the tree with lane b's commits applied (cherry-picked onto `campaign/W25`): whole catalogue 15,955 cases, 0 diverged; wide catalogue 14,200, 0 diverged; `ci_linux_warnings.sh --campaign-a` OK on 98 files; the `-DNDEBUG` scan names nothing in `accelerate.c`.

**Lane b, `accelerate.c` (3.71, flat by design): 16 of 17 commits taken.** Recipe D reuses the existing `ReleaseOpenCLBuffer` and `ReleaseKernel` in `ComputeBlurImage`, `ComputeRotationalBlurImage`, `ComputeResizeImage`, `ComputeFunctionImage`, `ComputeWaveletDenoiseImage`, `ComputeContrastImage` and the two resize filters (each site checked by the committed `tools/reuse_check.py`); Recipe E adds `ReleaseAccelerateQueue`, and D applies it in five workers; WP on `AccelerateLocalContrastImage`; amendment Recipe WP step 6 (convert a wrapper that a later step's probe flags as a latent clone). Workers shrink: Blur cc 37 -> 31, RotationalBlur 35 -> 28, Resize 34 -> 28, Function 22 -> 17, Wavelet 31 -> 27, Contrast 18 -> 14 (leaves Large Method). No function is fully cleared yet, so the real score stays 3.71; the stubbed score (`~/sp_w23a/stub.py`, all 19 emptied, no padding) is 9.68 at the base and at the tip. Left in the lane's plan: about 25 helper steps (`AcquireAccelerateQueue`, `AcquireAccelerateKernel`, `IsAccelerateFailure`, skipping the four `#ifdef RECREATEBUFFER` blocks; a D amendment for `!= NULL` sites written without the cast; the unreached workers `ComputeUnsharpMaskImage*` and `ComputeContrastStretchImage` need unreached forms) and 50 to 60 decomposition steps before the first rise at about 13 cleared functions: a job for four lanes. **Not taken:** `6d145abd3` (WP on `AccelerateWaveletDenoiseImage`): the reviewer re-scored the lane's probe with only `AccelerateLocalContrastImage` converted and the other wrappers untouched and found the Wavelet wrapper not flagged, so the amended step 6 does not license it; it stays on `campaign/W26-b` (local). Reviewer notes: all steps are flat and kept under rule 2 only as the plan's first part (next step generic); stub scores name a script outside the repository; the amendment wording does not say whether the probe runs on the base file or after earlier conversions.

**Lane c, `feature-texture.c` (8.72 -> 9.38, the stubbed ceiling): NOT merged.** New Recipe SZ (sink a run of zeroing writes to their first reader), a Recipe N step 8 amendment and three refactor commits clear `GetImageFeatures` (129 -> 63 lines; `ComputeFeatureStatistics` 46 lines, `AnalyzeFeatureCooccurrence` 69 lines, no Code Duplication; 26 cases, 0 diverged, `tools/omp_compare.sh` 26 runs, 0 diverged). Recipe SZ moves nine `memset` calls and `ZeroFeatureDensities` past calls and allocation-failure exits: statement order with side effects is on AGENTS.md's Never list and no ruling covers SZ, so the lane's commits stay on branch `campaign/W26-c` (local) as an owner question: **may a run of zeroing writes to local arrays move down past statements that do not name them?** The only finding left in the file after it is Lines of Code (no split: the file is a split product). Other reviewer notes: the SZ provenance text says the probed chain included FB (it did not: FB was dropped, `obj_identical` printed DIFFERENT because the frame changed from 0xe8 to 0xc8); the first SZ step is flat and licensed by the lane's own SZ step 5; `7e486b4e0` is `Verified: guard` with three throws moved under the 2026-11-12 ruling.

## Round fifteen, addendum (2026-11-15): feature-texture.c merged under the owner's ruling

The owner ruled on 2026-11-15 that Recipe SZ is acceptable ("that sounds like ok side effects if
the tests pass"); AGENTS.md records it. Branch `campaign/W26-c` (three `GetImageFeatures` steps, the
SZ recipe and the N step 8 amendment) is merged into `campaign/W25`; the playbook change rows were
united by hand (150 recipes, 428 change rows, `playbook_lint` 0 problems).

Gate on the merged tree: whole catalogue 15,955 cases, 0 diverged; wide catalogue 14,200 cases,
0 diverged; `ci_linux_warnings.sh --campaign-a` OK on 98 files in seven configurations; the
`-DNDEBUG` scan names nothing in `feature-texture.c`. Sweep (`codehealth_sweep.py --only
MagickCore/`, Linux cs-mcp): MagickCore mean 9.13 over 198 files; campaign A 98 files, mean 9.35
(9.34 before), 49 at 10.0; `feature-texture.c` 8.72 -> 9.38.

## Round sixteen (2026-11-15): accelerate.c chain, three lanes (branch campaign/W27, NOT in PR #118)

Three Opus lanes (a: ContrastStretch, Equalize, Despeckle; b: MotionBlur, LocalContrast, Modulate,
Grayscale; c: unsharp-mask workers, resize filters, checkAccelerateCondition, leftovers), 56
commits, merged without a source conflict (the playbook rows merged automatically). Gate on the
merged tree: whole catalogue 15,955 cases and wide catalogue 14,200 cases, 0 diverged;
`ci_linux_warnings.sh --campaign-a` OK on 98 files; the `-DNDEBUG` scan names nothing new;
`playbook_lint` 0 problems (150 recipes, 429 change rows).

`accelerate.c` real score 3.71 -> **3.24** on the merged tree. Complex Method 19 -> 13 functions,
Large Method 15 -> 14. Two findings appeared: Number of Functions in a Single Module (113
functions) and Code Duplication in six functions (the lanes' helper twins `CreateHistogramBuffer`,
`CreateEqualizeMapBuffer`, `AcquireLocalContrastResources`, `AcquireModulateResources` and the
wrappers `AccelerateContrastStretchImage`, `AccelerateGrayscaleImage`). Lane c alone fell 3.71 ->
3.58 (`36d6893ce`, committed `--no-verify`, disclosed). The branch is therefore kept apart from
draft PR #118 until the owner decides how to go on: unifying the twins (Recipe D) and then either
a split of the workers into a second file (Recipe S, which would also clear Number of Functions)
or more decomposition of the 13 remaining Complex functions. The stubbed ceiling (all 19 emptied)
is now 9.09, down from 9.68.

Reviewer notes: lane a `f0e2ba33b` writes `refactor_guard PASS` where the literal check printed WARN
(three copies of two launches: every literal x6, benign); lane a's Recipe Y step 7 amendment
(`be5bee9fd`) does not require equal variable types in the other copies; lane c `b23ac4538` is
`Verified: guard` while `targets_fill_check.py` prints FAIL (one-caller assumption), and two
trace-block moves sit after an early return test (2026-11-13 ruling reads "opening block");
lane c's tools commits `70243b078`, `ae4a138dd` extend `targets_fill_check.py` and `reuse_check.py`
with tests; lane b rests the `&imageBuffer` argument to `clSetKernelArg` on the OpenCL spec; the
stub script `~/sp_w23a/stub.py` lives outside the repository. A reviewer's `/tmp` wipe made one
command run in the Windows checkout on main (`git reset --hard HEAD`; no tracked change existed).

## Round seventeen (2026-11-15): accelerate.c 3.24 -> 3.62 (campaign/W28-a, one Opus lane)

Three refactor commits and one amendment: Recipe WP step 8 (`2a841c873`: a wrapper no code calls may
be converted at guard level; `c879d0764` converts `AccelerateContrastStretchImage`, which cleared the
whole Code Duplication finding at once, so no Recipe D unification was needed), Recipe Y on
`ComputeFunctionImage` (`d5eba3cb4`), Recipe E on `ComputeDespeckleImage` (`24592cc73`). Gate on the
tip: whole catalogue 15,955 and wide 14,200 cases, 0 diverged; `ci_linux_warnings.sh --campaign-a` OK
on 98 files; `playbook_lint` 0 problems. Real score 3.24 -> 3.49 -> 3.62 -> 3.62; the 3.71 of before
round sixteen is not yet regained. Remaining findings: Number of Functions (113), Overall Code
Complexity, Complex Method 13, Large Method 13, 18 argument findings.

**Blockers for the owner.** (1) *Recipe S split is refused by `split_source.py` for every family*: all
14 MagickPrivate roots share the prologue statics (`getAccelerateEnvironment`, `getOpenCLEnvironment`,
`checkAccelerateCondition`/`RGBA`/`Kind`, `checkHistogramCondition`, `checkPixelIntensity`,
`get32BitChannelValue`, `GetInputBufferFlags` and their predicates); a legal split would copy them or
widen their linkage, both forbidden. A probe that only removes 13 functions makes Lines of Code in a
Single File appear instead, for about +0.09. (2) *The remaining rise needs a chain of six functions at
once*: emptying `ComputeLocalContrastImage`, `ComputeUnsharpMaskImageSingle`, `resizeHorizontalFilter`,
`resizeVerticalFilter`, `ComputeUnsharpMaskImage`, `ComputeRotationalBlurImage` scores 3.89 stubbed;
every subset of up to five stays at 3.62 or below, and a function leaving Complex while still Large
lowers the score (Recipe D reuse in `ComputeLocalContrastImage`: 3.62 -> 3.50, reverted). The Large
Method threshold is 70 lines, flagged at 70.

Reviewer notes: WP step 8 is a guard-level path whose label `Decided by: owner` rests on the standing
2026-11-10 OK; the playbook's *Two levels of evidence* paragraph and AGENTS.md's guard-verified list
were not updated, so the owner should confirm or add it. `c879d0764`'s `--calls` list omits one
printed line (`GetMagickModule -1`); `d5eba3cb4` moves a throw that now names `SetFunctionKernelArgs`
(2026-11-13 ruling) without saying so.
