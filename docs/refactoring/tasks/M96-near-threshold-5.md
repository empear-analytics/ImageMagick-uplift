# M96 - near-threshold functions, fifth round

Not a generated task file. This round continues [M95](M95-near-threshold-4.md)'s wider
list. Branch `campaign/M96`, on `campaign/M95` (PR #49). Measured with `cs-mcp` 1.5.12;
oracle baseline `91d52cc5b3` (`main` after #48).

### Report (2026-10-26, afternoon)

5 refactor commits, one Recipe E each.

| File | Function | Effect | Evidence | Code Health |
|---|---|---|---|---|
| quantize.c | QuantizeImage | Bumpy Road gone, cc 15 → 9 | guard (not ready); oracle 368 cases | flat at 1.95 |
| cache.c | OpenPixelCacheOnDisk | no finding left (cc 12) | guard (no rows); oracle 70 cases | flat at 2.16 |
| shear.c | DeskewImage | cc 12 → 10, 97 → 86 lines | oracle, careful, 18 cases | 2.13 → 2.18 |
| compare.c | GetImageDistortions | no finding left (cc 12, 2 bumps) | guard (stale reports); oracle 62 cases | 1.77 → 1.83 |
| compare.c | GetImageDistortion | Complex Method gone (cc 9) | guard; oracle 165 cases | flat at 1.83 |

**Reverted.**
- quantize.c QuantizeImages: the same E on its tree-depth block. Its block differs from
  QuantizeImage's (no alpha or gray adjustment), so the two helpers are not one Recipe
  D. CodeScene still paired them as Code Duplication, and the score fell 1.95 → 1.91.

**Not taken.**
- ScaleKernelInfo, DrawStrokePolygon: one movable block brings cc from 12–13 to
  11, which is still flagged.
- AutoOrientImage: a single table-like `switch`.

**Where the near-threshold vein ends.**
- Five rounds (M92–M96) took the functions that one plain extraction can bring under a
  threshold.
- What is left in this band is one of five kinds:
  - predicates and comparators;
  - single `switch` dispatchers;
  - blocks that throw or return mid-function;
  - loops writing two live cursors;
  - helpers that would need five or more parameters.
- The larger remaining weight is in compare.c's `Get*Similarity` family (cc 15–34, Code
  Duplication across 14 functions). That needs Recipe K inside parallel loops, and
  fresh mutation data for compare.c first.

### Addendum: fresh mutation of compare.c (2026-10-26, 16:48)

compare.c was mutated in full on the Mac. The Mull build was `mull-compare96`, the report
is `build-oracle/work/mutation-compare96.json`, and the run used `--max-cases 200`.
- 694 mutants in 35 minutes.
- 504 killed, 48 unobservable.
- 101 no-coverage: 89 in the FFTW-only region, which this build does not compile, and 12
  in IsImagesEqual.

The old reports could not be gated at all; this one can.

| Verdict | Functions |
|---|---|
| ready | GetSSIMSimularity (sic), GetPHASESimilarity, GetMEPPSimilarity, CompareImages, GetMAESimilarity, GetMSESimilarity, GetFUZZSimilarity, GetPASimilarity, GetPHASHSimilarity, AccumulateFUZZPixel, AccumulateAEPixel, AddPHashDifferences, GetPSNRSimilarity, GetDSSIMSimilarity, GetSimilarityForMetric, GetImageDistortion, and the small predicates |
| careful | SimilarityImage, GetDPCSimilarity, GetNCCSimilarity, AccumulatePAPixel, AccumulatePDCPixel, AccumulateMEPPPixel, AccumulateMAEPixel, AccumulateMSEPixel, GetImageDistortions |
| not ready | AccumulateCorrelationPixel (9 of 14), GetRMSESimilarity (2 of 5) |
| no cases | IsImagesEqual |

Three compare.c steps were committed at guard level: GetPHASHSimilarity (M94),
GetImageDistortions and GetImageDistortion (this task). Their functions now gate ready,
careful and ready, so those steps stand at oracle level as well. The `Get*Similarity`
family, the largest remaining weight in compare.c, can now be refactored at oracle
level.
