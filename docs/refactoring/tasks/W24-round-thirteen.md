# W24 (2026-11-14): campaign A, round thirteen

Round thirteen's four lanes (a, b and d on Opus 5.5, c on Sonnet 5.5) finished their commits during the night, but the workflow stopped before its two reviewers per lane ran and before the lanes' final reports were written: **these commits are unreviewed and the figures below come from the commit messages and a fresh sweep**. They were cut from `c3ee375bf` (round twelve) and merged onto `main` (`ff6b744ec`, after #107) in `campaign/W24`; the merges needed only playbook merges by `addmerge.py`.

Gate on the merged tree: whole catalogue 15,955 cases, 0 diverged; wide catalogue 14,200, 0 diverged; `ci_linux_warnings.sh --campaign-a` OK on 96 files in seven configurations; `playbook_lint` 0 problems (148 recipes, 423 change rows); the `-DNDEBUG` scan names only files outside campaign A's lanes. Sweep (cs-mcp 1.5.16, whole MagickCore 196 files, mean 9.12): campaign A 96 files, mean **9.33**, 47 at 10.0.

| File | Before | After |
| --- | --- | --- |
| `morphology.c` | 6.93 | 10.0 (lane a split the kernel construction and morphology application family off into `morphology-apply.c`, 7.39) |
| `display-image.c` | 5.22 | 5.86 (lane c: `XImageWindowCommand` by LP chain form, `XLoadDirectoryImages`, `XBackgroundImage`, `XDisplayImage`) |
| `composite.c` | 3.90 | 4.24 (lane b: `CompositeImage` in 21 steps) |
| `feature-texture.c` | 7.97 | 8.72 (lane d: `GetImageFeatures`) |
| `distort.c` | 7.70 | 7.90 (lane d: `SetDistortNoBestFit`) |

**Lane a:** `AcquireKernelBuiltIn` (two steps), `ApplyKernelScaling`, `ShowKernelInfo`, `MorphologyApply`, the Recipe S split of `morphology.c`; amendments Recipe ST step 8 (a switch with no tail) and Recipe O step 8 (a targets struct the block passes on). **Lane b:** amendment Recipe LF step 5 (a case's leading break tests). **Lane c:** `tools/label_set_check.py` (with `--split`), `tools/loop_tail_check.py`; amendments Recipe LP chain form (step 7) and Recipe BC for unreached functions. **Lane d:** `tools/omp_compare.sh` (the OpenMP one/four-thread comparison); amendment Recipe N step 8 (shared struct plus read-only scalars).

**For the owner.** All of it was written under the 2026-11-10 OK and needs confirmation or relabelling (rows say `owner`): the new split of `morphology.c`, the six amendments above, the three new tools. Unreviewed means: the evidence labels (`Verified: oracle` versus `guard`), the stub scores and the recipe stretches in each message have not been checked by a second reader, as in earlier rounds; the whole catalogue passes. Still open from earlier rounds: fx function-pointer table, compare-similarity shared critical name, fourier Recipe TS, composite row flags inside the omp body, the third split of `xwindow.c` (merged with #107), `Flop`/`Flip` reading `x` and `y` unset, the `split_source.py` merged tool scores 7.32 (10.0 on the base).
