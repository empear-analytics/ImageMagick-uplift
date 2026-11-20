# W48 round thirty-five (2026-11-20 night)

Campaign A, lanes `campaign/W48-a` (widget split products) and `campaign/W48-b` (fourier and colorspace headers), cut from `campaign/W45` `9c9c31dbf` (PR #129) and merged back after review. Two fresh Opus lanes, one Sonnet reviewer per batch. Same rulings and readings as round 34.

## Result

Sweep (cs-mcp 1.5.16) on `deced329a`: campaign A 159 `.c` files, mean 9.78 → **9.81**, files at 10.0 **95 → 99**. MagickCore 9.47 → 9.49 (264 files).

| File | Before | After | How |
| --- | --- | --- | --- |
| `widget-file.c` | 8.81 | **10.0** | OH (X11 conditional form) carve, N in the new header |
| `widget-color.c`, `widget-font.c` | 9.38 | **10.0** | OH carves |
| `widget-command.c` | 9.38 | **10.0** | four-step E chain (steps 1-3 flat) |
| `widget-text.c`, `widget-list.c` | 9.09 | 9.68 | OH carves, E/N in the headers; held by MagickPrivate argument counts |
| `widget-core-private.h` | 9.68 | **10.0** | header Recipe B on `XEditText` (10 calls, 5 includers) |
| `colorspace-private.h` (upstream shared, 204 includers) | 7.43 | **10.0** | three OH carves of its predicates |
| `fourier-forward-private.h` | 9.38 | **10.0** | Recipe D (padding block → `PadFourierWidth`) |
| colorspace converter headers (16 before, 22 after) | 8.28-9.68 | 9.21-9.68 | OH carves; capped at 9.68 by the converters' argument counts |

New one-includer headers at 10.0: widget-color/-font/-file/-list-actions, widget-text-view, widget-list-entries.

## Evidence

- Gates on `deced329a`: `ci_linux_warnings --campaign-a` OK (159 files, 8 configurations); narrow 15,966 cases, **0 diverged**; wide 14,211, **0 diverged**; `check_trailers` 244 refactor commits valid; `playbook_lint` 185 recipes, 0 problems.
- Speed: every colorspace carve leaves all 204 includers' preprocessed units identical; reviewers measured `ModulateImage` inclusive Ir equal for HCL, HSB, HSI, HSL, LCHab and LCHuv, `-colorspace Lab`/`Jzazbz` equal, and the `txt:` writer equal. Two compiler register-allocation differences (enhance-color.o, fourier.o's `FourierSectionChannel`) had equal or lower caller Ir. Reading-1 tally for the round: +0.
- No `--no-verify`.

## Found on review and fixed

- **Tool gap:** `tools/obj_identical.sh` and `tools/obj_modulo.sh` compile both sides against the working-tree headers, so they cannot see a header-only change. One commit (`b1e5a4da5`, replayed as `52f9d1a10`) cited them for a header step; its message now states the real difference (one function, register allocation) and the Ir. An audit of this PR found one other header-only commit citing them (`806607fb5`, round 33), which also carries Ir evidence. Lanes now use header-aware comparisons (`~/sp_w48a/hdr_*.sh`).
- A missing Catalogue changes row (Recipe D amendment) and a weak reuse_check test, fixed before merge.
- Six messages in the colorspace chain say "colorspace-private.h 7.43 unchanged"; after a reordering the parent already scores 10.0. Recorded in merge commit `0717fb344`.
- `18782e091` dropped the execute bit on `tools/pp_identical.sh`; restored in `8f8e4c435`.

## New recipes and amendments (mandate 2026-11-19; please confirm)

Recipe D (return a block's output into a struct member; `tools/reuse_check.py` extended with tests); Recipe OH X11 conditional form (`5eee87541`) and its extension to one-includer OH products (`a88f73836`); `tools/pp_identical.sh` `WIDE=1`.

## What holds the rest

- Widget family: five files at 9.68 (`XProgressMonitorWidget`, `XMenuWidget`, `XDialogWidget`, `XTextViewWidget`, `XListBrowserWidget` argument counts).
- Colorspace converter headers: argument counts of the converters (9.68 cap) and per-pixel Complex/Large converters; `IssRGBCompatibleColorspace` and the Hue/Lab predicates are speed-bound (round 34 measurements).
- Everything else as in round 34's report.

## Owner questions (new)

1. The colorspace family now has 22 headers, several with one to four functions. Acceptable, or should some be merged once scores settle?
2. Should `tools/obj_identical.sh` and `tools/obj_modulo.sh` gain a header-aware mode (compile the .c against the REV's headers too)? The lanes' private scripts do this.
3. Confirm the Recipe D and OH amendments above.

## Upstream oddities noted, not touched

- `ForwardFourierTransformChannel`: `if (magnitude_info == (MemoryInfo *) NULL) magnitude_info=RelinquishVirtualMemory(magnitude_info);` uses `==`; on the error path where only `phase_info` failed, `magnitude_info` leaks.
- `ConvertProPhotoToXYZ` assigns `*X` twice; the first line (Display P3 coefficients) is overwritten.
