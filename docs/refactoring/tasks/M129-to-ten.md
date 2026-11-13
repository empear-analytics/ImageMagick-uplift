# M129 - campaign B, rounds aimed at 10.0 (2026-11-13 evening)

Stacked on M128 (#108). This branch also merges origin/main after #105 and #106, so it carries
campaign A's W22. Scores come from cs-mcp 1.5.16.

Campaign B: **mean 8.01 over 85 files -> 8.31 over 89 files; files at 10.0: 8 -> 18**. The files
at 10.0 are now: artifact, client, geometry, locale-format, magic, matrix-gauss, monitor, pixel-gamma,
registry, semaphore, signature, string-argv, string-info, string-si, thread, token, type-list and
version.

## Owner rulings of 2026-11-13 (AGENTS.md, 76a7af8ad)
- A function's opening block, its `LogMagickEvent(TraceEvent,...)` included, may move into a helper.
- The `%` doc-comment banners may be reworded by a guard-level recipe where CodeScene counts them as
  duplication.
- The main session's lane brief had wrongly told lanes to keep `Throw*` calls in place, although
  AGENTS.md has allowed moving them since 2026-11-12. Several earlier plateau notes rest on that
  mistake, and the lanes are revisiting them.

## Lane b, round 4: six files to 10.0
| File | Before -> after |
| --- | --- |
| semaphore.c | 9.38 -> 10.0 |
| artifact.c | 9.09 -> 10.0 |
| string-argv.c | 9.06 -> 10.0 |
| registry.c | 8.79 -> 10.0 |
| signature.c | 8.62 -> 10.0 |
| string-si.c | 8.50 -> 10.0 |
| mime.c | 9.61 -> 9.92 |
| static.c | 9.09 -> 9.68 |
| configure-paths.c | 8.95 -> 9.18 |
| linked-list.c | 9.02 -> 9.09 |

New Recipe IX; amendment to KC.

## Lane c, round 4
| File | Before -> after |
| --- | --- |
| magic.c | 8.45 -> 10.0 |
| token.c | 7.82 -> 10.0 (the last step needed lane a's IsGlob split) |
| color-equivalent.c | 7.60 -> 9.68 |
| token-next.c | 7.99 -> 9.38 |
| configure.c | 7.91 -> 9.02 |
| token-glob.c | 7.59 -> 8.42 |
| magick.c | 7.69 -> 8.29 |

New Recipe RS; amendment to B.

## Lane a, round 3: splits and passes
| File | Before -> after | New file |
| --- | --- | --- |
| constitute.c | 6.71 -> 8.81 | constitute-codec.c 7.55 |
| policy.c | 6.70 -> 7.62 | policy-path.c 9.45 |
| type.c | 6.90 -> 7.42 | type-list.c 10.0 |
| stream.c | 6.64 -> 6.89 | stream-read.c 9.02 |
| list.c | 7.55 -> 8.03 | |
| identify.c | 6.97 -> 7.18 | |
| matrix-gauss.c | 8.17 -> 10.0 | |

New Recipe TR. Tools: `split_source.py` now moves grouped and wrapped forward declarations, also
inside `extern "C"`, with a test (2dde52859).

## Gate on the pushed tip (baseline 6870d49fbb, new main)
- Whole catalogue: 15,961 cases, 3 diverged, all known (info/d333f9b818, gaps/2857e5295a,
  gaps/c5651ffad8).
- `cxx_warnings --all`: OK on 185 files. `ci_warnings`: OK on the 69 changed files.
- The no-delegate clang build: 0 warnings.
- `check_trailers`: every refactor commit valid. `playbook_lint`: 0 problems.

## For the owner
- **Several commits verified by one oracle run.** Lane a's eight list.c commits, the constitute
  split with the policy O step, and two batches across codec, identify and policy were each
  verified by one run over a build holding several steps; each message says so.
- **Hot path:** `GetNextToken` now calls helpers and passes pointers. Output is byte-identical;
  speed was not measured.
- **Error-message names:** in configure.c, two error calls moved into helpers, so they report the
  helper's name (ruling of 2026-11-12).
- **Moved fontconfig code:** the type-list.c split leaves the fontconfig include run behind. There
  is no objcopy on this Mac for split_branch_check, so the lane compiled the file with and without
  the run instead: the objects are byte-identical.
- **A step with no score gain:** registry.c's second DL commit is flat (object code identical).
  Drop it if you prefer.
- **New `#if` around a helper:** semaphore.c's `ExitOnMutexError` sits under the
  `#if !MAGICKCORE_OPENMP_SUPPORT && MAGICKCORE_THREAD_SUPPORT` branch that this build compiles.
- **Weak reach:**
  - Reached by no case (guard level only): GetConfigureInfo.
  - Reached by 1 to 6 cases: ListMagickInfo 1, ListMagicInfo 1, RemoveElementByValueFromLinkedList
    5, IsEquivalentImage 6.
