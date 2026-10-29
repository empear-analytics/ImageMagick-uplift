# W07 - campaign A, day session (2026-10-29)

Not a generated task file. Campaign A (Windows desktop, WSL2), branch `campaign/W07`,
stacked on `campaign/W06` (PR #69); the oracle's baseline is `main` at `c74bb179e`.
Measured with `cs-mcp` 1.5.16. Each file was worked by a subagent under one brief
(the loop of AGENTS.md, one recipe and one function per commit, every new helper free of
findings of its own), one file at a time; the session reviewed each batch before pushing.

### Report

| File | Code Health | Main recipes |
| --- | --- | --- |
| `transform.c` | 2.97 → **4.22** | E, D, O, N |
| `statistic.c` | 3.56 → **4.20** | E (step 7, loop body), N, B |
| `quantize.c` | 2.58 → 3.62 | E, D, G, N |
| `visual-effects.c` | 2.63 → 3.04 | P, E, N |
| `enhance.c` | 2.90 → 2.97 | E, O, D, N |
| `fx.c` | 2.28 → 2.85 | E, B, G, N, U, D |
| `distort.c` | 2.49 → 2.70 | E, G, O, N, D |
| `composite.c` | 2.02 → 2.57 | N, E, D |
| `morphology.c` | 2.20 → 2.55 | N, D, E, Q, V |
| `effect.c` | 1.59 → 2.02 | D, E, N, H, Q, B |

170 commits. Every refactor commit ran the oracle on its function's cases (0 diverged)
and passed the pre-commit gate; the whole catalogue ran after each file and at the tip:
15,900 cases, 0 diverged. `ci_warnings.sh` is clean on all ten files. The commit messages
say, per step, what the block reads and writes and why it may move.

### Catalogue changes (each in its own commit, `playbook_lint` clean)

- **Recipe N** - Extract Function Taking an Input Struct: Recipe E for a block that reads
  five or more of the caller's variables; the read-only inputs go in a struct filled
  directly before the call. The common blocker of the morning (a helper with 5+
  parameters carries Excess Arguments, which the gate refuses on new code).
- **Recipe Q** - Recipe K (accumulating block) with its inputs in a const struct;
  amended to cover sequential functions, with Recipe O's argument.
- **Recipe U** - Recipe T (block returning the loop status) with an input struct.
- **Recipe V** - a block ending in `goto <cleanup>` becomes a function returning
  `MagickFalse` for each `goto`; the caller jumps.
- **Recipe E amendments**: step 3, an array output updated element by element goes as a
  pointer; step 7, a loop's whole body that `continue`s moves (`continue;` becomes
  `return;`), and a switch case's tail block that `break`s with one output moves
  (`break;` becomes `return(v);`).

### For the owner

- **Function pointers to merge near-duplicates.** Two agents found pairs identical except
  for one called function (enhance.c's ContrastStretch/Equalize histogram pass, about
  +0.07; distort.c's Manhattan/Voronoi cases, +0.05). Passing the function as a pointer
  is not Recipe D as written, and it adds an indirect call per pixel. The distort.c step
  was taken out of the branch before it was pushed; it needs your ruling or a recipe.
- **Recipe B on Hull** (effect.c, 94b95ec8f): the struct is filled member by member
  directly before each of the four calls. W03 read step 4 as refusing Hull; this reading
  fills the struct anew for each call, which step 5 allows. Worth your confirmation.
- **Unreached paths**: Recipe V's failure paths in MorphologyApply (allocation failures),
  ExecuteRPN's cache-fetch and interpolation failures, TrimImage's empty-image path,
  distort.c's two-point Barycentric gradient (checked by hand on both builds,
  byte-identical) and PolynomialImage (2 cases) are checked by reading more than by the
  oracle. A few cases would close these.
- **Where files plateau**: the remaining bumps and nesting are mostly the parallel loops'
  `status`/progress blocks under `omp atomic`, near-duplicates differing by an operator
  (which become clones when extracted), and public signatures with Excess Arguments.
  enhance.c measured that its score responds only once several large functions lose all
  their findings.

### Noticed, not changed (possible upstream bugs)

- `DistortImage` and `HaldClutImage`: the parallel loop assigns the shared `status` per
  pixel, so a later success can mask a failure (as `MeanShiftImage`, W03).
- `fx.c` `GetProperty`: on the "is not a number" path `text` is destroyed, passed to
  `ThrowMagickException` as `%s`, then destroyed again.
- `composite.c`: `BlendRMSEResidual` resets `channel_residual` per row inside a
  `reduction(+)` loop; `SaliencyBlendImage` leaks images on some failure paths and can
  call `DestroyImage(NULL)` after a failed clone.
- `quantize.c` `ClassifyImageColors`: the `continue` after a failed `GetQNodeInfo` skips
  `index--`.
- `morphology.c`: `AcquireKernelInfo` leaks `kernel_cache` on a parse error; the reverse
  pass's single-row `k` starts differently for Distance and Voronoi.
- `effect.c` `SelectiveBlurImage`: the blended branch computes contrast from `image`, not
  `luminance_image`, and advances `luminance_pixels` without reading it.
- `enhance.c` `NegateImage`: the grayscale path returns `MagickTrue` even when a row
  failed. `distort.c` `SparseColorImage`: some cases test `image->colorspace` where
  Barycentric tests `sparse_image->colorspace`.
