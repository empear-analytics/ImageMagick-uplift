# W38 round twenty-eight (2026-11-17)

Base `aa556433f` (main `bb16e023a` after #121 plus round 27). Three Opus lanes, one Sonnet reviewer per batch, every hot-path step timed in the OpenMP -O2 HDRI build (`MAGICK_THREAD_LIMIT=1` and 4 threads, 21 alternating rounds beside an A/A run). All reviewed work merged.

| File | Before | After | How |
| --- | --- | --- | --- |
| `compare-similarity-image.c` | 4.32 | 5.83 | lane b: RL, H, CF, new Recipe TV, new Recipe KX, E, CB; Code Duplication gone. |
| `compare-similarity-private.h` | 4.24 | 5.11 | lane b: N, D (DPC/NCC loops merged, one critical name), E, CB. |
| `fx.c` | 8.54 | 9.09 | lane c: 43 Recipe DT steps (object code identical over the run) and 20 E steps in the translator; only Number of Functions left. `ExecuteRPN` untouched. |
| `display-image.c` | 5.86 | 6.11 | lane c: two E steps on `XDisplayImage` (Large Method cleared). |
| `accelerate-resize.c` | 7.09 | 7.52 | lane a: D (b), two B (a 15-parameter pass struct), V, I, O, D (b). |
| `accelerate-motion-blur.c` | 8.30 | 8.43 | lane a: Y, I, I. |

**Gates on `effbb78a8`:** narrow 15,961 and wide 14,206 cases (six new oracle cases), 0 diverged, against the new baseline `bb16e023a`; `ci_linux_warnings.sh --campaign-a` OK on 150 files; `playbook_lint` 160 recipes, 0 problems; `check_trailers` 152 commits valid. **Sweep** (cs-mcp 1.5.16): campaign A 150 files, mean **9.64** (was 9.62), 82 at 10.0; MagickCore 9.35.

## Notes

- **Performance:** no performance stop this round. Lanes re-measured first runs above A/A + 0.03 (at most 1.059) and committed only when the re-runs fell within noise; reviewers re-timed lane b's cases in their own OpenMP builds and found no slowdown. One message (`9573fe88e`) omits an earlier failing run (about 1.037); the code is the parse-time translator, not per-pixel.
- **Function-pointer dispatch:** new Recipes KX and TV add six indirect calls in all (none per pixel, one per row or per metric call), timed clean.
- **One lock for two regions:** `91b0836a1` merges the DPC and NCC parallel loops; their two named criticals become one (no nesting, same protection).
- **Pre-existing upstream bug, not touched:** `fx.c` `GetPropertyNumber` destroys `*text`, formats it in `ThrowMagickException`, then destroys it again (also at `e25aff32c`).
- New playbook text, all mandate: Recipe B step 5 block form (a `const` struct initialiser may hold member accesses, casts and a dereference), Recipe DT for bare `&&`/`||` operands and Overall Code Complexity, Recipe E's `sizeof` extent parameter, new Recipes TV and KX.
- Tools: `tools/omp_cases/compare-similarity-fft.txt` and `-spatial.txt` (the cases lane b's evidence cites).
- The owner's stance on slowdowns (no slowdown for a Code Health gain; `ExecuteRPN` and `CompositeChannelColor` performance-bound) is recorded in AGENTS.md with this report.

## What holds the rest

- `compare-similarity-image.c` (5.83): Complex Methods held by cleanup-macro acquisition chains; `SimilarityImage`'s switch under an unbraced `omp critical`; fixed public signatures.
- `compare-similarity-private.h` (5.11): 17-function duplication among `Accumulate*Pixel` and the small `Get*Similarity` wrappers (each pair differs in two or more values); SSIM twins differ in their output variable.
- `accelerate-resize.c` (7.52): the horizontal/vertical twins need either reordering two pure getters or relaxing TI's one-caller rule (probed end about 8.14).
- `display-image.c` (6.11): Overall Code Complexity needs about 90 more steps (probed end 6.49); Number of Functions and Lines of Declarations remain.
- `fx.c` (9.09), `morphology-primitive.c` (9.09), `morphology-builtin.c` (8.79), `accelerate-point.c` (9.09), `animate-window.c` (8.81): no legal step left (one public root, Number of Functions or public argument counts).

## Owner questions

1. Confirm the new mandate texts (B block form with a dereference, DT bare operands, E extent parameter, Recipes TV and KX).
2. `91b0836a1`: may a merge ignore a different declarator order of zero-initialised per-row arrays?
3. `accelerate-resize.c`: allow reordering the pure `GetResizeFilter*` getters, or relax TI's one-caller rule for host code?
4. `display-image.c`: run the ~90-step chain in a dedicated lane (6.11 to about 6.49)?
5. Record as plateaus: `fx.c` 9.09, `morphology-primitive.c` 9.09, `morphology-builtin.c` 8.79, `accelerate-point.c` 9.09, `animate-window.c` 8.81?
