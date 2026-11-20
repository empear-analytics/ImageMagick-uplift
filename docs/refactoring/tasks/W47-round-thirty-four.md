# W47 round thirty-four (2026-11-19 evening to 2026-11-20)

Campaign A, lanes `campaign/W47-a`, `-a2`, `-b`, `-b5`, `-b6` and `-c`, cut from `campaign/W45` (PR #129) and merged back after review. Opus lanes in batches, one Sonnet reviewer per batch; every batch was merged only after its review. Started on the owner's answer to the round-33 questions: "Yes to the qs. Keep going". The main session's readings of that answer are in PR #129 and in the brief; they are not written into AGENTS.md (an agent edit of that file is blocked).

## Result

Sweep (cs-mcp 1.5.16) on `69a134280`: campaign A 159 `.c` files, mean **9.78**, files at 10.0 **90 → 95**. The round started with 150 files at mean 9.78; the nine new split products (widget and xwindow-import families) are in the count, so the mean is not comparable one to one. MagickCore 9.46 → 9.47 (264 files).

| File | Before | After | How |
| --- | --- | --- | --- |
| `fx.c` | 9.09 | **10.0** | MD moves, then an OH carve into `fx-translate-private.h`, carved again into six headers, then E chains; all six at **10.0** |
| `fourier.c` | 9.31 | **10.0** | N, then an OH carve (`fourier-forward-private.h` 9.38) |
| `xwindow-import.c` | 9.38 | **10.0** | S split (`xwindow-import-image.c` 10.0 after its own carve), shared header 10.0 |
| `widget.c` | 8.28 | 9.68 | browser header (10.0), S splits: prefs 10.0, menu 9.68, color 9.38, font 9.38, command 9.38, list 9.09, text 9.09, file 8.81; accessors for shared state |
| `accelerate-resize.c` | 9.30 | 9.68 | XE |
| `accelerate-unsharp.c` | 8.46 | 8.55 | N (unreached code, guard level) |
| `compare-similarity-image.c` | 9.02 | 9.09 | PS under the 0.01 % reading |
| `colorspace-private.h` (upstream shared, 204 includers) | 3.69 | 7.43 | EL, then OH carves; eleven converter headers 8.28-9.68 |
| `quantum-private.h` / `token-private.h` / `composite-private.h` | 7.55 / 6.94 / 8.82 | 7.78 / 8.74 / 9.09 | E chain; GD, GC, F; TB |
| `thread-`, `timer-`, `color-`, `widget-private.h`, `accelerate-blur-private.h`, `widget-browser-private.h` | 9.53-9.68 | **10.0** | P, E, E, B, B, HM/FG/MS |
| `compare-similarity-frequency-images-private.h` / `-phase-private.h` / `-correlation-private.h` | 9.09 / 8.03 / 8.99 | 9.38 / 8.54 / 9.01 | E; NF, NF, CF; HC |
| `fx-execute-private.h` | 4.96 | 5.01 | three ExecuteRPN case bodies, fx Ir equal |

## Evidence

- Gates on `69a134280`: `ci_linux_warnings --campaign-a` OK (159 files, 8 configurations including the new `gcc-nocomplex`); narrow oracle 15,966 cases, **0 diverged**; wide 14,211, **0 diverged**; `check_trailers` 213 refactor commits valid (range `46b61ab8d..HEAD`); `playbook_lint` 185 recipes, 0 problems. Baseline `46b61ab8d`.
- Speed. Steps in per-pixel or per-row code were measured by callgrind at one thread in the OpenMP -O2 HDRI build, mostly under `setarch -R`, by the lane and again by the reviewer. Steps taken under the 0.01 % reading: `MeasureSimilarityImage` region +433 Ir (PS) and +4,468 (CF; once per candidate in subimage searches), together +0.0015 %; quantum export at most +0.0052 % (16-bit), +0.0062 % in one MIFF case; `GetCorrelationTotals` +2 per region entry. Gains measured on the way: float16 import -1.3 %, `ConcatenateColorComponent` (`txt:` writer) -15 Ir per pixel, `GetCorrelationTotals` below the base. All header carves of `colorspace-*` leave every one of 427 preprocessed units identical; `ModulateImage` inclusive Ir equal for LCHab, LCHuv and HSL.
- `--no-verify`: `f15aaec40` (widget.c 8.28 → 7.10 inside the stub-probed chain that ends at 9.68), disclosed. Two playbook-only cherry-picks (`a930f5569`, `c25aca8c8`) were made with hooks off.
- New tools: `tools/exhaustive_int_predicate.sh` (all 2^32 inputs of a one-int predicate; reviewer planted faults); the `gcc-nocomplex` configuration in `tools/ci_linux_warnings.sh` (also sets `ac_cv_func_cabs/carg/cimag/creal=no`; with the header alone missing, upstream `fourier.c` does not compile).

## Stopped or rejected

- Stopped for speed: PS on `GetCorrelationTotals`' atomic progress block (+0.043 % on a small case; would give 9.38); the correlation `row` reading `totals` (+0.051 %); CB on `GetMEPPSimilarity` (+0.6 %); EL and pair forms on `IsHueCompatibleColorspace` and `IsLabCompatibleColorspace` (+0.07 to +2.9 % in `ConcatenateColorComponent` / `IsFuzzyEquivalencePixelInfo`).
- Rejected on review: the first fx.c carve (`9f6e618fa`): its run held the external-linkage `AcquireFxInfo`/`DestroyFxInfo`; redone with Recipe MD moves first. CF's first message claimed "once per compare call"; it runs per candidate in subimage searches: measured and the message rewritten before merge.
- Twelve lane commit messages had PowerShell backtick escapes turned into control characters; all were rewritten before merge (trees unchanged). Lanes now write messages through files and check them with `cat -A`.
- The twin-region extrema merge (-2.8 % Ir, no score gain) was not taken.
- Own defects fixed: an unused local left by `8fca823bc`; a set-but-unused `c` left by `5944898d3` in `xwindow-image.c` (clang warning on every non-VMS build).

## New recipes and amendments (mandate 2026-11-19; please confirm)

NF (name a formula as a function), MD (move a declared function's definition later), XE (unwrap the else of an if that leaves), EL (equality chain as a switch label set), HM (move a family header's helpers into the header it includes), MS (merge twins differing in one statement run through a selector struct). Amendments: OH step 1 (read-only pointer array in a run) and OH upstream-shared form; TB inverted and header forms; GD, GC (with exhaustive evidence); B step 5 one-line declaration form; FG wrapper form; S step 2 (written statics, structs and pointers shared through MagickPrivate accessors, one object each).

## What holds the rest

- `widget.c` 9.68 and many 9.68 files: a MagickExport/MagickPrivate argument count.
- `fx-execute-private.h` 5.01: ExecuteRPN, per pixel; case-group probes cap at 5.63 / 6.69 / 7.64 even if every group moved.
- `colorspace-private.h` 7.43: the Hue/Lab predicates (speed stops) and duplication between two three-label switches; the converter headers hold per-pixel argument and duplication findings.
- `compare-similarity-*` headers: every remaining step exceeds 0.01 % on a reaching case.
- `opencl-initialize.c` 9.38: a Form A run would carry `_WIN32` timer branches (owner question 3).
- `accelerate-unsharp.c` 8.55 / `accelerate-contrast-stretch.c` 7.30: unreached; further helpers flip on Overall Code Complexity; ceiling 9.68.
- `display-image.c`, `animate-window.c`, `xwindow-image.c`: #126 Q2/Q6/Q10.
- Upstream shared headers `string-`, `utility-`, `image-`, `xml-tree-`, `draw-`, `delegate-private.h`: twins of different types, campaign B callers, Windows-only branches.

## Owner questions (new)

1. Should the pre-commit hook score `*-private.h` headers? It checks only `.c`/`.cpp`; header steps and header drops inside chains were scored by hand.
2. Confirm the recipes and amendments above.
3. `opencl-initialize.c`: may a Form A run carry the `_WIN32` timer branches verbatim?
4. May oracle cases be added for unreached code: a frequency-domain PSNR subimage search (`PSNRSimilarityImage`; 8.54 → 9.09 probed), `XNoticeWidget`, `XProgressMonitorWidget`, the command-widget tracking and the widget double-click paths?
5. `xwindow-import-image.c` was read as covered by round-33 Q3 (a split product of xwindow-import.c), and `XGetSubwindow` joined that header.
6. Accessor rewrites in two unreached functions (`TrackCommandSelections`, `FollowCommandMotion`) were taken at guard level with a read-through and per-function instruction counts. Acceptable?

## Upstream oddities noted, not touched

- `IsUTFValid` (token-private.h) returns MagickFalse exactly for negative codes; its surrogate and 0xfffe/0xffff tests never decide.
- `fourier.c` does not compile where `<complex.h>` is missing but `cabs`/`carg`/`cimag`/`creal` are in libm.
- `xwindow-image.c` sign compare (`index < image->colors`), moved verbatim from upstream xwindow.c.
