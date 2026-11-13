# W23 session (2026-11-13 evening): round seven

PR 104 (rounds four to six, the MSVC fixes and the `switch_split_check.py` refactor) merged at its head f313851c5; this round was cut from that tip. Four lanes (lane a on Opus 5.5, lanes b to d on Sonnet 5.5; two Sonnet reviewers each) on disjoint
functions of `xwindow.c`, `display.c` and `animate.c`. Gate on the merged tree: whole catalogue 15,955 cases, 0 diverged (baseline ee884d8fd9); wide catalogue 14,200, 0 diverged; `ci_linux_warnings.sh --campaign-a` OK on 92 files in seven
configurations; `playbook_lint` 0 problems; the `-DNDEBUG` unused-parameter scan finds nothing new in campaign A. Sweep: campaign A mean **9.19**, 44 at 10.0.

| File | Score |
| --- | --- |
| `xwindow.c` | 3.40 -> **5.96** (lanes a and b merged; each alone 4.33 and 4.57; out of the Red band) |
| `animate.c` | 4.97 -> **7.31** (lane d; `XAnimateBackgroundImage` has no finding left; `XAnimateImages` keeps only Large Method at 156 lines and its signature) |
| `display.c` | 2.68 -> 2.71 (lane c; only the Recipe W step `XCreateBlendMattes` raises it) |

**xwindow.c.** Lane a (Opus, 20 commits): `XMakeImage` (Bumpy Road and Deep Nesting gone; cc 15, LoC 91, 7 arguments remain), `XMakeImageLSBFirst`, `XMakeImageMSBFirst` and `XDitherImage` (Complex Method gone) through Recipe E helpers (XAcquireXYBitmapData, XAcquireZPixmapData,
XAcquireXImageData, the shared-memory helpers, `XPackColormapped*`/`XPackContinuous*`, `AcquireDitherMaps`, `SetDitherEntries`), P (`IsShapedImageWindow`), CN and H. Lane b (18 commits): `XRefreshWindow`, `XDrawImage`, `XMakeWindow`, `XMakeStandardColormap` and `XRenderImage` now carry only their
public argument-count findings (XDrawImage value-form helpers 3.52 -> 3.68; `XMakeStandardColormap` CT chain with a shared `XColormapContext` then Z for the switch, 3.88 -> 4.21, whole object IDENTICAL; `XRenderImage` by Recipe I with a const targets struct of statics, 4.21 -> 4.54); two playbook amendments
(Recipe E step 3: a block-scope static struct by address; Recipe I step 2: structure statics on CT step 2's terms). The stub probe says the rest of the file is held by the argument counts and by the findings in the remaining unconverted functions.
**animate.c** (lane d, 34 commits): `XAnimateBackgroundImage` through 14 helpers over two const structs (`AnimateBackgroundSetup`, `AnimateBackgroundTargets`) plus an early-targets struct, Recipe D form (b) reusing `XSortAnimateScenes`; `XAnimateImages` cc 59 -> 6, bumps 4 -> 0 (the whole event loop in `XRunAnimateLoop`, `XFinishAnimation`);
one step was `--no-verify` (985cc11a3, 5.71 -> 5.66: the 111th function brings the file-level Number of Functions finding; the next two commits raised the score past 5.71). **display.c** (lane c, 13 commits, 2.68 -> 2.71): `XTrimImage` and `XSaveImage` keep only their five arguments; `XChopImage` bumps 6 -> 4, cc 45 -> 38; `XCompositeImage` 6/49/333 -> 4/44/300; the file sits at 216 functions
by CodeScene's count and one extra function keeps 2.71 while a second drops it to 2.65, so the real lever is a reduction: probed, not committed. `XMatteEditImage`, `XPasteImage`, `XImageCache` (cc 80, the 61-label undo group) and `XImageWindowCommand` (cc 132) are untouched.

**For the owner (round seven).**
- **`#if` blocks and pairs, again (lanes a, b, d):** `XDiscardSharedXImage`, `XAllocateSharedSegment`, `XRemoveSharedSegment`, `XQueryShapeSupport` get new `#if` pairs around their definitions; `XReplaceWindowXImage`, `XApplyMatteShape`, `XPutRefreshImage` and `XCreateSizeHints` (with `PRE_R4_ICCCM`) move whole nested `#if` blocks. All are compiled by the wide build and checked by object comparison, but are not covered by your rulings.
- **Moved `LogMagickEvent` calls (lane d):** backdrop, image window, panel windows, `XCreateBackgroundXImage`, `XCreateSceneXImage`, `XCreateImageWindow`; and `GetMagickModule()` in moved throws now names the helper. Round six's question stands.
- **Recipe stretches the lanes flagged:** E by value of a fully filled mutable struct (`XImageRowSource`, `XDitherTables`; lane a); E step 4's two-outer-variable limit on `XCreateWindowTextProperties` (two block-scope statics; lane b); Z applied to a switch of helper calls (`XMakeStandardColormap`, lane b); Recipe Z step 7's targets struct used for read-only inputs (`AnimateBackgroundEarlyTargets`) and a combined N form for `XRunAnimateLoop`/`XFinishAnimation` (lane d). The moved text checks out in each; no behavior risk was found.
- **Evidence:** `9fecf7909` says `Verified: oracle` but only the not-shaped path of the moved block runs in the oracle; several stubbed-score pairs quoted in lane c's messages ('3.10 against 3.08') are not reproducible (stub-set differences).
- Two playbook amendments (lane b) and one (lane d) are written under the 2026-11-10 OK.
- `XMakeImageMSBFirst` and `XDitherImage` are reached by no case; `XMakeImageMSBFirst`'s texture and polarity blocks are identical to the LSB helpers (probe 4.20 -> 4.24), which Recipe D's unreached form cannot reuse because the caller is reached.
