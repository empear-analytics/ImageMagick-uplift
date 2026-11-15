# W29 session (2026-11-15 night): round eighteen, files held for OpenMP

The owner ruled on 2026-11-15 ("you can touch openmp files. If the test cases still pass, it's fine!")
that OpenMP regions may be edited when the catalogue stays at 0 diverged; AGENTS.md records it with the
agents' extra conditions (by-hand write-set reading, `tools/omp_compare.sh` at one and four threads, no
slowdown, changed `critical` names named). Three Opus lanes (a `composite.c`, b `compare-similarity.c`,
c `fourier.c`), one reviewer each, cut from `campaign/W29` (0def28fab); 26 commits. The playbook merge of
lanes b and c needed `addmerge.py`.

| File | Before | After | What moved |
| --- | --- | --- | --- |
| `composite.c` | 4.24 | **5.01** | new Recipes RI (parallel row loop filling its row struct once) and FL (twin row loop folded through a flag); Recipe D and five Recipe SD steps on `CompositeChannelColor` |
| `compare-similarity.c` | 2.97 | **3.01** | new Recipe MR: `GetMAE/MSE/AE/FUZZ/PDC/PASimilarity` fold through one metric driver; six named critical sections become one lock |
| `fourier.c` | 7.59 | **8.74** | Recipe TS re-landed from the held branch, new Recipe RG (parallel sections region into a helper returning the status), Recipe N; both transforms free of findings |

Gate on the merged tree: whole catalogue 15,955 cases and wide catalogue 14,200 cases, 0 diverged;
`ci_linux_warnings.sh --campaign-a` OK on 98 files; `playbook_lint` 0 problems (155 recipes, 435 change
rows); the `-DNDEBUG` scan names nothing new. Sweep: campaign A 98 files, mean 9.37 (9.35 before), 49 at 10.0.

**The reviewers' major finding, and what I did.** Reviewer a showed that every case in the lanes'
`tools/omp_cases` files runs its region on ONE thread even at `OMP_NUM_THREADS=4`:
`GetMagickNumberThreads` returns 1 below about 262144 pixels (four times that for `CompositeCopyImage`), and
the cases were at most 300x300. The lanes' "1 and 4 threads, 0 diverged" evidence was therefore vacuous
for race detection (the reviewer closed the gap for composite with 19 large cases, 38 runs, 0 diverged). I added
`tools/omp_cases/large-images.txt` (38 composite operators on 2200x2200 canvases, eight compare metrics with
and without `-fuzz` on 1200x1200, eight Fourier forms on 1024x1024) and ran OpenMP gcc -O2 builds with FFTW of
the base 0def28fab and the merged tip 6e5e26fe8: **152 runs at one and four threads, 0 diverged** (2m45s, real
parallel work). AGENTS.md now says an `omp_compare` result counts only with cases big enough.

**What is left.** `composite.c`: `CompositeChannelColor` (cc 141, 652 lines) stops because each further Recipe SD
level adds a nested switch on the per-channel path (clang instruction count for `Xor` +2.85% after five steps,
the next would pass 3%); clearing it needs a dispatch that does not deepen per run, for example a constant family
table (a new recipe, not tried; stubbed 7.27). `CompositeImage` (cc 28, 7 public arguments). `compare-similarity.c`
stays near the floor: SimilarityImage cc 37, DPCSimilarityImage 36, NCCSimilarityImage 28, GetPHASESimilarity 28
and more, Code Duplication in 24 functions (seven `Accumulate*Pixel` functions share a skeleton but their error
formulas differ in floating-point arithmetic); `GetMEPPSimilarity` was not folded because each row seeds a local
maximum from the shared maximum outside the critical section (a racy upstream read, kept). `fourier.c`: Lines of
Code (1,130; `split_source.py` refuses both public roots: shared statics), Overall Code Complexity, one Bumpy Road in
`InverseFourierTransformImage` held by `#if`/`#else` branches no build compiles. `prepress.c` was already 10.0.

**Reviewer notes for the owner.** All refactor commits are `Verified: guard`, honestly (failure branches run in no
case). Lane b accepts callgrind increases of +2.7% (MAE), about +1.2% (AE, Fuzz), +0.5% (MSE) for the merged loop
with wall-clock within noise: AGENTS.md's "slows by more than measurement noise" does not settle whether that counts;
the owner may rule. `MergeMetricValue` is `static inline`, not forced inline as Recipe MR step 4 wants (gcc inlines it;
clang and MSVC are not guaranteed). Recipe RI copies a caller's `CompositeRowInputs` whose `y` and `pixels` are
unset (C11 6.2.6.1p6 argument; valgrind or MSan may report the copy). Recipe TS and RG rows say `Decided by: owner`
though written under the standing OK (the legend would say `mandate`); the playbook's *OpenMP regions* section still
says never to move or edit a pragma and does not point at the AGENTS.md ruling. `ee22afb46` was amended message-only
with `--no-verify` after a garbled gcc figure. Upstream bug, not fixed: `ForwardFourierTransformChannel` tests
`if (magnitude_info == NULL) magnitude_info=RelinquishVirtualMemory(...)` where `!=` was meant (leak when only the
phase allocation fails). `fx.c` and `accelerate.c` were not part of this round.

## Round nineteen (2026-11-16 night): mid-range files probed, one rise

Three Opus lanes probed morphology-apply.c and distort.c (a), widget.c and xwindow-image.c (b), quantize.c,
animate-window.c and the 9.x group (c). Only lane c made a refactor commit: **`colorspace-convert.c` 9.08 -> 9.68**
by a new Recipe KV (`4e93f10f4`, replace a split call switch by a table of function pointers of two signatures;
`5a0822556` on `ConvertRGBToGeneric`: oracle 607 cases 0 diverged, timing ratios 0.965 to 1.011, `--calls` FAIL by
design since the calls became table entries, read by hand). Gate on the tip (lane c's own runs): whole catalogue
15,955 and wide 14,200 cases, 0 diverged; `ci_linux_warnings.sh --campaign-a` OK on 98 files.

Why the other files did not move (stub probes with `~/sp_w23a/stub.py`, no padding):
- **morphology-apply.c 7.41**: `IsBaseKernelNeeded` (cc 40) keeps 20 labels in a never-compiled `#if 0` block that must
  stay verbatim (stubbed alone 8.28); `MorphologyApply` has 7 MagickPrivate arguments; Number of Functions (173) in a
  split product; Overall Code Complexity needs a chain of about 54 flat extractions (+0.47).
- **distort.c 7.90**: `poly_basis_dx` (cc 17, per pixel; index-table form measured 5% to 20% slower in W16, order split
  clones `PolyBasisFn*`); `DistortImage` and `SparseColorImage` are MagickExport; Number of Functions (126, shared
  statics block Recipe S); Overall Code Complexity needs about 29 flat Recipe E/N extractions (+0.50, a full lane;
  Recipe DT would need a *Use when* amendment to cover it).
- **widget.c 7.78 and xwindow-image.c 8.28**: Number of Functions (368 and 246; no legal split, re-carving split
  products is not allowed) and MagickPrivate argument counts (all six, resp. 13 functions); even with every count cut to
  4 (illegal) they reach only 8.03 and 8.54; folding functions makes Overall Code Complexity appear.
- **quantize.c 8.81, animate-window.c 8.81**: Number of Functions and public arguments; `split_source.py` refuses every
  public root of quantize.c (shared statics, `MaxTreeDepth` macro); animate-window.c is a split product.
- **threshold-image.c 9.09, enhance-tone.c 9.09, effect-filter.c 9.09, layer-frames.c 9.38,
  colorspace-transform-image.c 9.38**: Lines of Code in split products, plus public arguments. **colorspace-convert.c
  9.68**: three argument findings on MagickExport/MagickPrivate functions.

Owner decisions that would unlock more: (1) a ruling that a split product may be carved again (the W21 round two rule
holds nine files at Lines of Code or Number of Functions); (2) public and MagickPrivate argument counts (the largest
group of remaining findings: 40 or so functions across the files above), which a changed signature would clear but the
rules forbid; (3) a long flat chain for `distort.c` (29 steps, +0.50) and `morphology-apply.c` (54 steps, +0.47).
Reviewer note on Recipe KV: its text cites "the AGENTS.md 3% rule" which AGENTS.md does not contain (other recipes cite
it the same way); the row says `Decided by: owner` where the legend would say `mandate`.
