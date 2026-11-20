# W49 round thirty-six (2026-11-20)

Campaign A, lanes `campaign/W49-a` (MagickPrivate signatures), `campaign/W49-b` (the X11 files) and `campaign/W49-c` (unbuilt code and tools), cut from `campaign/W45` `f82c1e937` (the head of PR #129, merged as main `e644889a9`) and merged into `campaign/W49` after review. Opus lanes in batches, one Sonnet reviewer per batch. Started on the owner's 2026-11-20 answers ("yes to all, as you recommend"; "I answer yes to that" for `MagickPrivate` signatures). Those rulings are in `agents-rulings-2026-11-20.md` for the owner to paste into AGENTS.md; an agent edit of AGENTS.md is blocked by the permission check.

## Result

Sweep (cs-mcp 1.5.16) on `f40d91b17`: campaign A 159 `.c` files, mean 9.81 → **9.87**, files at 10.0 **99 → 115**. MagickCore 9.49 → 9.53 (264 files).

| File | Before | After | How |
| --- | --- | --- | --- |
| `accelerate-motion-blur.c`, `accelerate-resize.c`, `accelerate-point.c` | 9.68 | **10.0** | Recipe B, MagickPrivate form |
| `morphology-primitive.c`, `widget.c`, `opencl-enqueue.c` | 9.68 | **10.0** | B, MagickPrivate form (`EnqueueOpenCLKernel`: 11 calls in 7 files) |
| `fx-execute.c`, `xwindow-text.c` | 9.68 | **10.0** | B, MagickPrivate form, callers in campaign B's `property.c` and `annotate.c` changed in the same commit |
| `xwindow-visual.c`, `xwindow-annotate.c`, `xwindow-pixel.c` | 9.68 | **10.0** | B, MagickPrivate form |
| `widget-text.c`, `widget-list.c`, `widget-menu.c`, `widget-dialog.c` | 9.68 | **10.0** | B, MagickPrivate form (`XDialogWidget`: 46 call sites) |
| `opencl-initialize.c` | 9.38 | **10.0** | OH mid-file conditional form (the `_WIN32` timer branches moved verbatim) |
| `display-image.c` | 6.49 | 9.68 | OH chain (8 headers), then six re-carves; capped by the MagickExport `XDisplayImage` |
| `animate-window.c` | 8.81 | 9.68 | OH chain (4 headers); capped by the MagickExport `XAnimateImages` |
| `accelerate-histogram.c` / `accelerate-contrast-stretch.c` | 8.85 / 7.30 | 8.95 / 7.61 | Recipe UB (the `RECREATEBUFFER` blocks moved verbatim into fragment headers); B |
| `accelerate-unsharp.c` | 8.55 | 8.56 | DI chain after the EnqueueOpenCLKernel change |

## Evidence

- Gates on `f40d91b17`, **baseline pinned to `46b61ab8d`** (the pre-round-32 main): `ci_linux_warnings --campaign-a` OK (159 files, 8 configurations); narrow 15,971 cases, **0 diverged**; wide 14,216, **0 diverged**; `check_trailers` 292 refactor commits valid since `46b61ab8d`; `playbook_lint` 186 recipes, 0 problems; `c4065_scan --self-test` passed.
- MagickPrivate steps: reviewers confirmed with `git grep` that no caller anywhere in the tree is left on an old signature, compiled every includer of each changed header with each side's own headers (only the bundled function and its callers differ at `-O0`; behaviour-neutral codegen differences at `-O2 -fno-inline` in callers of a changed `static inline`, described in the merge messages), and found no new release-build (`NDEBUG`) warning.
- Speed: every step is once-per-call host code, X11 UI code or object-identical; reading-1 tally +0. The two colorspace generic converters were stopped: bundling their arguments raised per-pixel Ir by 0.7-3.8 %.
- `--no-verify` (disclosed, inside stub-probed chains that end higher): `004333ff4`, `49eb456a6` (display-image.c chain), `7130a4b71` (accelerate-unsharp.c chain).

## Tools

- `tools/codescene_precommit.py` now gates `*-private.h` files. A header cs-mcp cannot score passes only when its exact content (path and git blob hash) is in `tools/noscore_headers.txt` (41 reviewed entries); reviewers found and closed three bypasses on the way (a new `.c` held to a low header's bar; K&R and `) MACRO {` functions hidden from a heuristic; a path-only allowlist). `tools/test_codescene_precommit.sh` covers them.
- `tools/c4065_scan.py` (the MSVC C4065 scan now skips `#if 0`); `HDRS=1` for `tools/obj_identical.sh` and `tools/obj_modulo.sh` (each side compiled against its own headers; they were blind to header-only changes); `tools/oracle/baseline_diff.py`; `casemap.py --update`.
- Five new oracle cases reach code no case reached before: frequency-domain PSNR subimage search (2), and the command-widget tracking, text-view drag and list-browser cursor paths (3). Their outputs are the same on `46b61ab8d` as on main `e644889a9`, so rounds 32-35 left them unchanged. `infra/0fd402f13a` and `xevents/0e762518fe` are on the load-sensitive list.

## New recipes and amendments (mandate 2026-11-19; owner confirmation pending)

Recipe UB (move an unbuilt block verbatim into a fragment header); Recipe B MagickPrivate form; Recipe OH mid-file form (a run after a kept item), chain bar, re-carve of a mid-file header, mid-file conditional form; Recipe B forward-declared typedef in a prototype block.

## Corrections recorded in merge messages

- `55df80dd7`: the ComputeEqualizeImage device path is reached by `opencl/3087d55cb7`, not only the `clEnv` test.
- `ce5ea8123`: five unchanged callers in `display-image.c` differ at `-O2 -fno-inline` in tail calls and epilogue placement as well as register allocation (behaviour-neutral).

## What holds the rest

- 37 files at 9.68 hold a MagickExport function's argument count (public signatures stay), as do `display-image.c` and `animate-window.c`.
- Next round: `XGetWindowInfo` (`xwindow-info.c`; `XFreeResources` is held: a caller sits in unbuilt `coders/dps.c`), the seven MagickPrivate functions in `xwindow-image.c` (8.28), and a table-driven merge of about 20 `XApply*Command` dialog twins in the display-image apply headers (7.32).
- `colorspace-generic-rgb.c`: speed stop. `accelerate-contrast-stretch.c`: no caller exists (`AccelerateContrastStretchImage` is never called), guard level only. `morphology-builtin-base-private.h`: the `#if 0` labels can only move with an edit, which the ruling forbids.

## Owner questions

1. Confirm the recipes and amendments above.
2. `opencl-initialize.c`'s mid-file include rests on reading the 2026-11-20 yes to #126 Q2 as covering mid-file includes in general, not only in X11 files. Correct?

## Upstream oddities noted, not touched

- `AccelerateContrastStretchImage` is declared and defined but never called.
- `ComputeEqualizeImage` and `ComputeContrastStretchImage` return through `ThrowBinaryException` after acquiring the queue and device, skipping cleanup.
