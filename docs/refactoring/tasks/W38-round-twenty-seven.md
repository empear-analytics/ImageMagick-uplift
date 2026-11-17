# W38 round twenty-seven (2026-11-17)

Base `6546f5845` (campaign/W34 after round 26 plus the AGENTS.md record of the owner's answer to the round-twenty-six questions: "yes to the questions that lead toward CH 10; if performance will be much impacted, ask again"). PR #121 (rounds 23 to 26) was merged by the owner during the round; this round continues on `campaign/W38` (W34's round-27 merges plus main `bb16e023a`). Three Opus lanes, refilled as they reported, one Sonnet reviewer per batch, and a separate OpenMP performance audit of the merged campaign. All reviewed work merged.

## Results

| File | Before | After | How |
| --- | --- | --- | --- |
| `accelerate.c` | 3.62 | **10.0** | lane a: seven Recipe S splits through `accelerate-core-private.h` (10.0), then D/D/P; four more splits of the products with two family headers, then E steps. Products: `-resize` 7.09, `-contrast-stretch` 7.30, `-histogram` 7.73, `-motion-blur` 8.30, `-unsharp` 8.46, `-despeckle` 8.55, `-blur` 8.98, `-point` 9.09, `-local-contrast` 9.68, `-denoise` 10.0, `-rotational-blur` 10.0. |
| `opencl-environment.c` | 9.38 | **10.0** | lane a: `InitializeOpenCL`'s family moved to `opencl-initialize.c` (9.38): a relocation of its Lines of Code. |
| `composite.c` | 5.01 | **10.0** | lane b: **a relabel**: `CompositeImage` and 134 statics moved unchanged to `composite-image.c` (5.01), then an E and a PS step on `TextureImage`. |
| `fx.c` | 4.61 | 8.54 | lane b: `FxEvaluateChannelExpression` to `fx-execute.c` (9.68); the `ExecuteRPN` engine in `fx-execute-private.h` (4.87, 4.96 after one P step). |
| `compare-similarity.c` | 3.01 | 9.68 | lane c: **mostly relocated debt**: split into `compare-similarity-private.h` (4.24) and `compare-similarity-image.c` (4.32). |
| `distort-sparse.c` | 9.09 | 9.68 | lane c: two Recipe P predicates, six E steps. |
| `morphology-primitive.c` | 8.54 | 9.09 | lane c: four E steps. |
| `morphology-builtin.c` | 8.26 | 8.79 | lane c: E and D form (b) steps. |

**Gates on the merged tree (`d3ab65fef`, before the merge of main):** narrow 15,955 cases and wide 14,200 cases, 0 diverged; `ci_linux_warnings.sh --campaign-a` OK on 150 files (seven configurations); `playbook_lint` 0 problems; `check_trailers` 160 refactor commits valid since main. **Sweep** (cs-mcp 1.5.16): campaign A 150 files, mean **9.62** (was 9.60), **82 at 10.0** (was 77); MagickCore 9.34 over 250 files (was 9.31), red files 6 → 4.

## Performance: a measurement change

The oracle builds are configured `--disable-openmp`. Lane b's two Recipe E steps on `ExecuteRPN` timed within noise there, but its reviewer measured them **1.42 to 1.66 times slower** in an OpenMP -O2 HDRI build (`tools/omp_compare.sh build`, `MAGICK_THREAD_LIMIT=1`, 21 alternating rounds), on every `-fx` expression, including ones that run no moved code. Both were reverted (`1a9889a4d`, `e3f95cacc`) before merging. From then on every lane timed in the OpenMP -O2 build; lanes a and c re-timed their round's hot-path steps there (all within noise), and lane b's `fx.c` split timed within noise against `e25aff32c` (three runs of the `p[]` case: 1.075, 0.982, 1.028). A separate audit compares the campaign tip with `e25aff32c` in OpenMP builds over about forty hot operations; its result is reported separately.

**Performance stops (owner question):**
- `ExecuteRPN` (`fx-execute-private.h`): the per-pixel SD chain (5.05 → 5.95) measured 1.106 and 1.202 (A/A 1.007, 0.978) even in the oracle build; forced inlining about 1.19-1.21; a function-pointer table (6.81) 1.195 and 1.156; a hybrid 1.147. Not committed.
- `CompositeChannelColor` (`composite-image.c`): a family-table dispatch (5.01 → 7.27, oracle clean) measured in the OpenMP -O2 build Multiply 1.196, SrcAtop 1.142, Mathematics 1.239, Out 1.103, Screen 1.137 (A/A 0.993-1.031). Not committed.

## Review notes

No majors besides the ExecuteRPN slowdown above.
- The `accelerate.c` modulate split first timed 1.048/1.082/1.033 under load (A/A 0.980/1.035/0.989), then 1.013, 1.020 and a rotating 0.989/1.009 with `AccelerateModulateImage`'s instructions unchanged apart from `__LINE__` immediates: read as noise. Message-only amends with `--no-verify` on two commits (trees unchanged).
- `compare-similarity-private.h` first held a doubled block of six `#define`s the original did not have; removed in `7c50e69e7`.
- `split_branch_check` DIFFERENT classes read by the lanes: inlining changed by Recipe S's added `inline`; a dropped leading `endbr64` (now ignored by the tool, `35946f0d5`); GOT access.
- The DPC metric at 4 threads is nondeterministic in the baseline too (floating-point order in an `omp critical` accumulation).
- Tools: `split_branch_check.py` lists every differing function; `register_source.py`'s `am__objects_1` bug fixed with a test; new `tools/omp_cases/compare-similarity-large.txt`.

## Owner questions

1. `ExecuteRPN` and `CompositeChannelColor`: every form that raises their files costs 10-66% in the production-like build. Record `fx-execute-private.h` (4.96) and `composite-image.c` (5.01) as performance-bound plateaus, or accept a cost (how much)?
2. `composite.c` (10.0) and `compare-similarity.c` (9.68) rose by relocating debt; the products are the real targets. Keep counting such relocations, or require products to rise too?
3. Confirm the `split_branch_check` DIFFERENT readings above and Recipe P used only for Overall Code Complexity.
4. `morphology-primitive.c` (9.09) and `morphology-builtin.c` (8.79): one public root each, Lines of Code; record as plateaus, or allow a header used by one file?
