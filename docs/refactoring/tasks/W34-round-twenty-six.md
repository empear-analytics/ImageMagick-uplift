# W34 round twenty-six (2026-11-16)

Base `7c8383522` (campaign/W34 after round 25 plus the AGENTS.md record of the owner's answers to the round-twenty-five questions: duplicated object code accepted if it does not really hurt performance; "if the test cases pass": the second family header, an annotate/draw timing harness, declaring `opencl.c`'s external global in a private header, and a `morphology-apply.c` chain through a large header). "Test cases pass" was read as the narrow and wide oracle catalogues with 0 diverged plus the timing and OpenMP checks of the earlier rulings; every lane ran both catalogues on its tip. Question 6 (`xwindow-image.c` as a plateau) was not answered, so no lane carved it. Three Opus lanes, one Sonnet reviewer per batch. All lane work merged.

| File | Before | After | How |
| --- | --- | --- | --- |
| `opencl.c` | 9.09 | **10.0** | lane a: two splits, `opencl-enqueue.c` (9.68) and `opencl-environment.c` (9.38, Lines of Code), with `openCL_library`, `default_CLEnv` and `openCL_lock` declared `extern` in `opencl-private.h`. |
| `morphology-apply.c` | 8.26 | **10.0** | lane c: a chain of two splits, `morphology-primitive.c` (8.54) and `morphology-builtin.c` (8.26), through `morphology-apply-private.h` and the family header `morphology-apply-kernel-private.h` (9.68 each); one disclosed `--no-verify` (`655240ed7`, intermediate 8.01). |
| `distort.c` | 8.40 | 9.09 | lane a: `distort-private.h` (70 shared statics, 9.09) and the `SparseColorImage` split to `distort-sparse.c` (9.09) in one commit, then a three-step Recipe SW chain on `poly_basis_dx`. |
| `xwindow-annotate.c` | 9.09 | 9.68 | lane b: new `tools/xannotate_harness.c` and `xannotate_compare.sh`; Recipe TH merge of `SetAnnotateBoxPixel`/`SetAnnotatePenPixel`, then two E steps; held by `MagickPrivate` argument counts. |

**Gates on the merged tree (`140b9140e`):** narrow 15,955 cases and wide 14,200 cases, 0 diverged; `ci_linux_warnings.sh --campaign-a` OK on 135 files (seven configurations); `playbook_lint` 158 recipes, 464 change rows, 0 problems; `check_trailers` 101 refactor commits valid. OpenMP: `morphology-apply.c` omp_compare 32 runs and `distort.c` 22 runs at 1 and 4 threads on unseeded large cases, 0 diverged, four-thread speed-ups shown; new `tools/omp_cases/morphology.txt`. Every timed path within noise (re-measured at 21 runs where an 11-run ratio passed 1.03). **Sweep** (cs-mcp 1.5.16): campaign A 135 files, mean **9.60** (was 9.59), **77 at 10.0** (was 75); MagickCore 9.31 over 235 files (was 9.30).

## Review notes

No majors.

- **`morphology-apply.c`: the debt moved.** The kept file is 598 lines at 10.0; about 80% of the old 7,106 lines sit in `morphology-primitive.c` (8.54) and `morphology-builtin.c` (8.26) with the same findings (Lines of Code, Overall Code Complexity, `IsBaseKernelNeeded`'s Complex Method, `MorphologyApply`'s seven arguments). The gain is the kept file's score and the separation of kernel construction, built-ins and the pixel pass. The chain meets the ruling's conditions (every product and header at least 8.26).
- **`opencl.c`:** the ruling named `openCL_library`; the lane also declared `default_CLEnv` and `openCL_lock` the same way, and moved the definitions of `cache_directory` and `cache_directory_lock` (in no header, used nowhere else) and the `accelerate-kernels-private.h` include with the environment family (mandate forms). Every symbol is defined once; nothing outside the library changes. `split_branch_check` shows DIFFERENT for the new files only through lost cross-file inlining and GOT access; the wide oracle is the evidence.
- `655240ed7`'s message cites the omp cases that `e7cb72e2c` commits afterwards; the intermediate commit is red on the CodeScene gate (disclosed).
- `xwindow-annotate.c`: the `XTransferAnnotatePixels`/`XTransferDrawPixels` merge was not taken: CodeScene no longer reports them as a clone pair and the merge would drop the file to 9.38.
- Lane a reported that repeated `rose: -blur 0x1 null:` runs in one warm OpenCL cache directory sometimes abort (1 of 25 on the baseline, 2 of 25 on the tree); fresh directories, as the oracle uses, never did.

## Playbook changes

- Recipe S step 2: an external-linkage variable both parts name is declared `extern` in the existing private header (owner, for `openCL_library`; wording and checks the lane's); three mandate forms (an `#if` above a run, an object-defining include moving with its only user, an external variable only the family names moving with it); a split chain with a disclosed intermediate drop (owner; the product-score condition the lane's).
- Recipe TH *Use when*: copies the oracle reaches but not on every path, with the harness beside the oracle (owner ruling; wording the lane's).
- Tools: `split_source.py` (`#if` above a run, object-defining include), `twin_fold_check.py --rename`, `tools/xannotate_harness.c`, `tools/xannotate_compare.sh`, `tools/omp_cases/morphology.txt`.
- Known plateaus rows for `morphology-apply.c`, `distort.c`, `opencl.c` where they exist.

## What holds the rest

- `morphology-primitive.c` (8.54), `morphology-builtin.c` (8.26): one public root each; Lines of Code, Overall Code Complexity, a Complex Method in an `#if 0` label run, seven `MagickPrivate` arguments.
- `distort.c` (9.09): Lines of Code and `DistortImage`'s 6 arguments; `distort-sparse.c` (9.09): Overall Code Complexity (E steps not yet probed); `distort-private.h`: Lines of Code, Primitive Obsession.
- `opencl-environment.c` (9.38): Lines of Code (1,778 lines), a further split possible.
- `xwindow-image.c`, `widget.c` (8.28): see round 25.

## Owner questions

1. Accept the extension of the `openCL_library` ruling to `default_CLEnv` and `openCL_lock`, and the mandate moves of `cache_directory`/its lock and the object-defining include?
2. Accept `split_branch_check` showing DIFFERENT on OpenCL split products when the wide oracle is clean and the differences are only lost cross-file inlining and GOT access?
3. `morphology-apply.c` reached 10.0 by moving its debt into two products at 8.54 and 8.26. Keep it, or should such chains require the products to rise too?
4. Leave `XTransferAnnotatePixels`/`XTransferDrawPixels` unmerged (no longer flagged; merging drops the file)?
5. Record `xwindow-image.c` (8.28), and the files held only by `MagickPrivate` argument counts at 9.68, as plateaus?
6. Lift `split_branch_check.py`'s ten-function listing limit?
