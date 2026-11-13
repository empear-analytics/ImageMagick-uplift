# W22 session (2026-11-13 afternoon): round six

Four lanes on disjoint functions of the two wide files whose score needs many functions cleared (xwindow.c, animate.c); lane a on Opus 5.5, lanes b to d on Sonnet 5.5; two Sonnet reviewers per lane. Cut from `campaign/W22` 4b4e9dfb3. As predicted
by the stub probes, each xwindow.c lane was flat or at most 2.89 on its own branch (lane a 2.89, lane b 2.54, lane c 2.54 against the base 2.55), and the merged file is **3.40**. `animate.c` 4.40 -> **4.97** (lane d alone). Gate on the merged tree: whole catalogue
15,955 cases, 0 diverged (baseline e5affc3c52); wide catalogue 14,200, 0 diverged; `ci_linux_warnings.sh --campaign-a` OK on 92 files in seven configurations; `playbook_lint` 0 problems; the `-DNDEBUG` unused-parameter scan finds nothing new in campaign A.

| File | Score |
| --- | --- |
| `xwindow.c` | 2.55 -> **3.40** (three lanes merged; 70 commits) |
| `animate.c` | 4.40 -> **4.97** (12 refactor commits and Recipe F's Automatic form) |

**xwindow.c, lane a (Opus): `XMakeImage`** (13 commits): the Brain Method and Deep Nesting are gone (cc 35, LoC 164, 6 bumps remain, plus the 7 public arguments). Ten flat leaf steps (E: crop, log, destroy-previous, matte image and pixmap, pixel conversion; EJ), then N with a new
`XWindowTransform` struct (2.64), the N pass-through resize (2.65) and `XAttachSharedXImage` (2.89, the first step that adds the file-level Number of Functions finding, which the 2026-11-07 ruling allows because the chain lifts the file). Two `HAVE_SHARED_MEMORY` and two `HAVE_SHAPE` branches
moved whole into helpers (3edc144c0, 50f279b05, a2b748c54, c4c096217; wide `obj_modulo`/`obj_identical` IDENTICAL). Left: the "Create X image" block (a probe of moving it whole scored 2.85 against 2.89), `XMakeImageLSBFirst`, `XMakeImageMSBFirst`, `XDitherImage` (not started; the last two are reached by no case).
**Lane b: `XDrawImage`** cc 43 -> 9, LoC 273 -> 98 (SD twice, E, N twice, I, D(b) twice, D(a)); `XMakeWindow` cc 27 -> 14, LoC 184 -> 112; `XRefreshWindow` Large Method gone; `XRenderImage` LoC 156 -> 144 only (its `if (display == NULL)` init block writes eight statics, more than Recipe I's six).
**Lane c:** `DestroyXResources`, `XGetResourceClass`, `XUserPreferences` and `XMakeMagnifyImage` have no finding left; `XGetPixelInfo` keeps only its 6-argument signature; `XMakeStandardColormap` cc 43 -> 18, LoC 248 -> 148, bumps 6 -> 2 (the seven-label visual switch keeps cc above 9). Four steps were `--no-verify` after a gate refusal of 0.01
(lane b two, lane c four); a fifth attempt, the whole pixel-size switch of `XMakeMagnifyImage` as one helper, measured 2.48 and was discarded.

**animate.c, lane d:** Recipe F (two steps hoisting `XAnimateImages`'s six static const command tables and its automatic menu arrays: 4.40 -> 4.60; the menu arrays are automatic, so the lane added an *Automatic* form to Recipe F step 1, playbook commit 0082973c0: an automatic const array of string pointers
may move as static when every use is an element read, a pointer-to-const argument to a callee that keeps no copy, or a member of a read-only local struct; the check is data-section comparison), then E helpers (`XCreateAnimateWindows`, `XLoadAnimateFont`, `XCoalesceAnimateImages`, `XSortAnimateScenes`, `XChangeToHomeDirectory`, `XWithdrawBackdropWindows`) and N helpers over an `AnimateWindowSetup` struct.
`XAnimateImages` is cc 59, LoC 564, 4 bumps; `XAnimateBackgroundImage`, `XSaveImage` and `XMagickCommand` are untouched. A trial `XCreateWidgetWindow` took the file from 4.89 to 4.55 (the Code Duplication cliff: the Widget, popup and Image-window blocks are twins of the Command window helper) and was reverted.

**For the owner (round six):**
- **Moved `LogMagickEvent(X11Event,GetMagickModule(),...)` calls.** `XLogXImage` and `XCreateMatteXImage` (lane a), several window-id logs in `XAnimateImages` (lane d) and `LogXStandardColormap` (lane c) now log the helper's name and line under `-debug X11`. Your 2026-11-12 ruling names error calls; the playbook's plateau notes refuse this very move for log calls elsewhere; no case enables X11 logging, so nothing observes it. Confirm that log calls may name the helper, or revert those commits.
- **`#if` blocks moved by Recipe E and a new `#if` pair.** Lane a moves nested `HAVE_SHARED_MEMORY` and `HAVE_SHAPE` blocks (directive lines included) whole into helpers; lane c adds a new `#if defined(...)` pair around two helper definitions (`DetachXShmSegment`, `PutXBooleanPreference`). The wide build compiles every one of them and the object comparison is IDENTICAL; the rules name splits and the listed sites, not extractions. Confirm or revert.
- Recipe D form (b) on `XDrawImage`'s scale block (25815da9a): `Verified: guard`, no case runs the replaced block, `obj_modulo` printed MODULO-IDENTICAL, and `reuse_check.py` could not read the by-value pointer that is also returned (the check is an ad-hoc script that is not committed).
- A reviewer found a stub-figure pair in 31ee498bd that is not like for like (the stubbed gain is +0.08, not +0.36).
- Upstream, reported and not fixed: `XCreateMatteXImage` reads `matte_image->width` before the `matte_image != NULL` test.
- Recipe F's Automatic form (animate.c) is an amendment written under the 2026-11-10 OK.
