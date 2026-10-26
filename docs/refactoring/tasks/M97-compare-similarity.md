# M97 - compare.c: the Get*Similarity family on fresh mutation data

Not a generated task file. M96's fresh mutation run gave compare.c a usable readiness
table for the first time. This task used it on the `Get*Similarity` family. Branch
`campaign/M97`, on `campaign/M96` (PR #50). Measured with `cs-mcp` 1.5.12; oracle baseline
`91d52cc5b3`.

### Report (2026-10-26, evening)

| Step | Function | Effect | Evidence |
|---|---|---|---|
| E `NormalizeChannelSimilarity` | GetMAESimilarity | cc 20 → 15, 4 → 3 bumps, 101 → 84 lines | oracle, ready, 10 cases |
| D (reuse) | GetMSESimilarity | cc 20 → 15, 4 → 3 bumps, 100 → 83 lines | oracle, ready, 37 cases |
| D (reuse) | GetSSIMSimularity | cc 35 → 30, 221 → 204 lines | oracle, ready, 20 cases |

compare.c stays at **1.83**. The file's findings are dominated by two groups:
- The FFTW-only functions, which CodeScene measures but this build does not compile:
  DPCSimilarityImage, NCCSimilarityImage, PhaseSimilarityImage, MSESimilarityImage and
  others.
- The per-row loops of each `Get*Similarity`. Their pixel loop adds into
  `channel_area` and `channel_similarity` inside a parallel region. That is Recipe K, but
  with seven parameters (image, reconstruct_image, p, q, columns and the two
  accumulators).

**Caught by the oracle.**
- GetMEPPSimilarity ends in the same normalisation text. On a trial reuse, 3 MEPP cases
  diverged, and the change was reverted at once.
- MEPP reads the reciprocal `area` after the block
  (`normalized_mean_error=mean_error*area`), so the block's write to `area` is live there.
- The reuse script had compared text but not liveness. It now refuses a block whose
  written variables are read after it.
- This is the oracle doing its job on a function where the guard alone would have
  passed: the literals and calls were unchanged.

**What is left in compare.c.**
- The Recipe K row loops above, which need Recipe B (bundle the arguments) first.
- AE, PHASE and PHASH end in different normalisations.
- The FFTW branch, which cannot be touched here.
