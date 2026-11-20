# W50 round thirty-seven (2026-11-20)

Campaign A, lanes `campaign/W50-a` (the X11 MagickPrivate signatures), `campaign/W50-b` (the display-image dialog twins) and `campaign/W50-c` (clean-ups), cut from `campaign/W49` `14745d7a7` and merged back into `campaign/W49` (PR #130) after review. Started on the owner's "1. Yes. 2. Yes. Continue" (the round-36 recipes confirmed; mid-file includes allowed in any file).

## Result

Sweep (cs-mcp 1.5.16) on `fa14c2be1`: campaign A 159 `.c` files, mean 9.87 → **9.88**, files at 10.0 115 (unchanged); MagickCore 9.53. Most of the round's gains are in private headers, which the `.c` sweep does not list.

| File | Before | After | How |
| --- | --- | --- | --- |
| `xwindow-image.c` | 8.28 | 8.54 | all seven MagickPrivate signatures bundled (Recipe B), FB/FI/E pre-steps to keep callers under Large Method; only Number of Functions left |
| `accelerate-histogram.c` | 8.95 | 9.38 | Recipes I, JN (new), H, LM step 7 on the reached OpenCL path |
| `animate-window-command-private.h` | 9.38 | **10.0** | re-carve into the new `animate-window-menu-private.h` (**10.0**), then E steps |
| `display-image-apply-effects-private.h` | 7.32 | 8.28 | Recipe DC (new), B chain, re-carve into the new `display-image-apply-filters-private.h` (8.03) |
| `display-image-apply-private.h` | 7.32 | 7.55 | B on `XMagickCommand` (26 call sites) and `XApplyCropAndScale` |
| `display-image-draw-private.h` / `-draw-undo-private.h` | 7.78 / 7.32 | 8.03 / 7.55 | B steps |

## Evidence

- Gates on `fa14c2be1`, oracle baseline pinned to `46b61ab8d`: `ci_linux_warnings --campaign-a` OK (159 files, 8 configurations); narrow 15,971 cases, **0 diverged**; wide 14,216, **0 diverged**; `check_trailers` 343 valid; `playbook_lint` 188 recipes, 0 problems.
- Every batch reviewed by a separate agent: whole-tree `git grep` for old signatures, call sites paired with their old calls by script (134 for lane a's seven functions), captured variables checked for writes between declaration and call, every includer compiled with each side's own headers (only the changed functions differ at `-O0`), no new release-build warning.
- Speed: X11 UI and once-per-call OpenCL host code only; no timing needed. Reading-1 tally +0.
- No `--no-verify`.
- **Windows CI:** the fix for #129's warnings (`f82c1e937`) left `IsBaseKernelNeeded`'s `return(MagickFalse)` unreachable (MSVC C4702), which kept the Windows builds red on main and here. Removed in `ebae5d59c` (objects identical); on that commit all three Windows builds passed.

## Dropped or held

- Dropped before merge: `XGetWindowInfo`'s bundle and its pre-step (`xwindow-info.c` stays capped by `XFreeResources`, whose caller is in unbuilt `coders/dps.c`, so the step could never raise a file).
- Held: the `XApply*ToImage` twin merges (every probe left the filters header flat at 8.03; the tiny per-command clones that keep their own buffers hold the duplication finding); the `xwindow-image.c` carve (needs the timed LSB/MSB packer and magnify twin merges first: carved headers would score 6.89-7.78).
- `021b106b1` joins two wrapped lines (tokens and object identical) only to keep `XROIApplyRegion` under the Large Method line; disclosed.

## Corrections recorded in merge messages

- `ee116f58e`: 26 call sites (not 29), 4 in block form (not 5).
- `e542c23d4` merges lane b's batches 3-4 (its subject says 1-2).

## New recipes and amendments (mandate 2026-11-19; owner confirmation pending)

Recipe JN (drop a conditional jump to the label that directly follows it; not across a loop's closing brace); Recipe DC (run twin dialog commands through one runner taking a command struct); Recipe LM step 7 (initialiser form); Recipe B step 5 (the one-line declaration form also inside the block form and to stay under Lines of Code in a Single File).

## What holds the rest

- `xwindow-image.c` 8.54: Number of Functions; a carve needs the per-pixel packer/magnify twin merges first (timed).
- `display-image-apply-private.h` 7.55: duplication across the 14 `XApply*Commands` dispatchers (a keyed-table step is next).
- `display-image-apply-filters-private.h` 8.03: the tiny per-command clones.
- `xwindow-info.c` 9.68, `accelerate-unsharp.c`, `accelerate-contrast-stretch.c`, `colorspace-generic-rgb.c`, `compare-similarity-image.c`, `statistic-image.c`: as in round 36. 37 files at 9.68 hold a MagickExport argument count.

## Owner questions

1. Confirm Recipes JN and DC and the LM step 7 and B step 5 amendments.

## Upstream oddities noted, not touched

- `XApplySpreadToImage` calls `EdgeImage`, not `SpreadImage` (also at `46b61ab8d`).
- The `XMakePixmap` banner in `xwindow-image.c` shows `XMakeStandardColormap`'s old format line (an upstream copy-paste).
- `ComputeEqualizeImage` still returns through `ThrowBinaryException` after acquiring the queue and device.
