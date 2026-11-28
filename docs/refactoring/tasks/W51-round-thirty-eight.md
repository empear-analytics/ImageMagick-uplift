# W51 round thirty-eight (2026-11-28)

Campaign A, lanes `campaign/W51-a` (colorspace and compare headers), `campaign/W51-b` (display-image headers, X11, accelerate) and `campaign/W51-c` (carves and splits), cut from main `4178bbf3e` and merged into `campaign/W51` after review. Started on the owner's "make as much 10 as possible" (2026-11-27). The lanes ran from 00:30 to the morning; this PR holds every package that had passed review by 07:55. Packages still running then will come in later merges to the same branch.

## How the round ran

1. **Inventory** (106 agents): every campaign A `.c` file and every private header it includes was scored with cs-mcp on `4178bbf3e`. **104 were below 10.0** (44 `.c`, 60 headers). One agent per file worked out and stub-probed a route to 10.0. **66 had a legal route**; the other 38 are held by MagickExport argument counts or by the per-pixel Ir rule.
2. **Lanes**: three lanes took the 25 planned packages, then a queue of the remaining reachable files. Every package went to an independent reviewer that rescored, reran the oracle (`--base 46b61ab8d`) and checked the rules; up to two fix rounds; a package that failed three reviews was reset away.

## Result: 30 files to 10.0

| File | Before | After | How |
| --- | --- | --- | --- |
| `accelerate-kernels-private.h` | 2.03 | **10.0** | Recipe UB kernel-string form: 17 verbatim carves of the OpenCL kernel text; kernel strings byte-identical |
| `string-private.h` | 8.54 | **10.0** | OH upstream-shared carve into `string-interpret-private.h` (10.0) |
| `utility-private.h` | 8.81 | **10.0** | OH carve into `utility-file-private.h` (10.0) |
| `token-private.h` | 9.09 | **10.0** | OH carve into `token-utf8-private.h` (10.0) |
| `morphology-primitive-direct-private.h` | 9.38 | **10.0** | OH re-carve into `morphology-primitive-direct-row-private.h` (10.0) |
| `opencl-initialize-benchmark-private.h` | 9.38 | **10.0** | B on a token struct, OH carve into `opencl-initialize-doctype-private.h` (10.0) |
| `distort-linear-private.h` | 9.68 | **10.0** | BP chain (`CoefficientCall`, `CoefficientContext`) |
| `visual-effects-stegano.c` | 9.68 | **10.0** | Recipe S: `StereoAnaglyphImage` to `visual-effects-stereo.c` (9.68, MagickExport) |
| `histogram.c` | 9.68 | **10.0** | B, then S: min-max stretch to `histogram-stretch.c` (9.68, MagickExport) |
| `resize-rescale.c` | 9.68 | **10.0** | P, then S: interpolative and liquid rescale to `resize-liquid.c` (9.68, MagickExport) |
| `threshold-level.c` | 9.68 | **10.0** | S: range and white threshold families to `threshold-range.c` (9.68, MagickExport) and `threshold-level-private.h` (10.0) |
| `accelerate-histogram.c` | 9.38 | **10.0** | DT on two predicates |
| `display-image-crop-private.h` | 8.28 | **10.0** | CF, MA |
| `display-image-apply-effects-private.h` | 8.28 | **10.0** | IC, E, B chain |
| `display-image-annotate-private.h` | 9.09 | **10.0** | B, W twin merge, P |
| `display-image-tile-private.h` | 9.09 | **10.0** | eight B bundles, seven OH carves (all products 10.0) |
| `display-image-event-private.h` | 9.38 | **10.0** | two OH carves (`-event-windows` 10.0, `-event-handle` 9.38) |
| `xwindow-info.c` | 9.68 | **10.0** | B on `XGetWindowInfo`, S: `XFreeResources` to `xwindow-free.c` (9.68) |
| `compare-similarity-frequency-images-private.h` | 9.38 | **10.0** | four E steps (HN amendment) |
| `colorspace-jzazbz-private.h` | 9.68 | **10.0** | B, static-inline header form |
| `colorspace-rgb-to-jzazbz-private.h` | 9.68 | **10.0** | B |
| `colorspace-xyz-to-jzazbz-private.h` | 9.21 | **10.0** | H, B |
| `colorspace-jzazbz-to-xyz-private.h` | 9.22 | **10.0** | H, B, E |
| `colorspace-conversions-private.h` | 9.68 | **10.0** | B on six converters (callers include `tests/validate.c`) |
| `colorspace-xyz-to-rgb-private.h` | 9.68 | **10.0** | B on four converters |
| `colorspace-cie-to-rgb-private.h` | 9.68 | **10.0** | B on five converters |
| `colorspace-illuminant-xyz-private.h` | 9.68 | **10.0** | B on two converters |
| `colorspace-jzazbz-oklab-private.h` | 9.68 | **10.0** | OH trailing-run carve to `colorspace-rgb-to-oklab-private.h` (9.68), B |
| `colorspace-lab-to-rgb-private.h` | 9.68 | **10.0** | OH carve to `colorspace-lchab-to-rgb-private.h` (9.68), B |
| `colorspace-luv-lms-to-rgb-private.h` | 9.68 | **10.0** | OH carve to `colorspace-lchuv-lms-to-rgb-private.h` (9.68), B |

New files: 40 (35 headers, 5 `.c` split products). The split products that stay at 9.68 hold a MagickExport function with too many arguments, which no recipe may change; the original file is what reaches 10.0. Before/after scores were measured by the implementing agent and re-measured by the reviewer with `tools/ch.py` (cs-mcp).

## Evidence

- Gates on `df7b939db`, oracle baseline pinned to `46b61ab8d`: `ci_linux_warnings --campaign-a` OK (164 files, 8 configurations, C4065 scan); narrow 16,004 cases, **0 diverged** (infra/0fd402f13a settled as load-sensitive); `check_trailers` 125 refactor commits, 0 without valid trailers; `playbook_lint` 195 recipes, 0 problems; `c4065_scan` self-test passed.
- Wide 14,249 cases, 1 diverged: `xevents/37a863f961` (display menu Transform > Roll..., `out.ppm` differs). The full run shared the machine with the three running lanes (load average about 11). Rerun alone with `--no-cache` five times: 5/5 pass; the six Roll/Transform cases together pass. It is an X event script like the settled `xevents/bea57ff5d7` in the same run, and no commit of this round touches the Roll code (`XApplyRollToImage` and `XApplyRollCommand` are in `display-image-apply-filters-private.h`, unchanged; the dispatcher `display-image-apply-private.h` changed only in `XApplySaveCommand`'s call, `3a178718a`). Recorded as load-sensitive.
- Sweep (cs-mcp, `sweep25.sh`) on `df7b939db`: campaign A **164** `.c` files (159 plus 5 split products), mean 9.88, **121 at 10.0** (115 before; the five split products are at 9.68). MagickCore 9.67. Most of the round's gains are in private headers, which this `.c` sweep does not list.

- Every package reviewed by a separate agent: oracle `--function` (narrow, and `WIDE=1` for X11/OpenCL code), object comparisons where no case reaches the code, per-pixel Ir where objects changed, release flags, trailers.
- No `--no-verify`.

## Dropped or held

- **Dropped after three failed reviews:** `display-image-matte-private.h` (messages stated narrow-only reach counts and, after rewrites, cited rewritten hashes). Lane b was reset to before the package.
- **Held on the per-pixel Ir rule (reading 1):** `colorspace-rgb-predicates-private.h`, `colorspace-lab-compatible-private.h`, `colorspace-predicates-private.h`, `colorspace-rgb-to-luma-private.h`. Each reaches 10.0 in a probe, but the txt/sparse-color writers call these predicates once per pixel and every form measured adds instructions there: +0.013 % to +0.59 % against the 0.01 % cap (rgb-to-luma: +0.17 % per non-black pixel). Nothing committed.
- **Held on owner questions:** `accelerate-unsharp.c` (see below).
- **Not finished:** `display-image-window-setup-private.h` (the agent was cut off before its first commit; to be retried).
- Still running when this PR was opened: the remaining queue (colorspace converters, compare-similarity, display-image, `accelerate-contrast-stretch.c`, `enhance-level.c`, `resample.c`, `quantum-private.h`, `image-private.h`, `xwindow-image.c`).

## New recipes and amendments (mandate 2026-11-19; owner confirmation pending)

- UB, kernel-string form (`27d5d44bd`): verbatim carve of an OpenCL kernel-string run between macro calls.
- OH: upstream shared mid-file form (`eac2a6cb7`); declarations in that form's run (`3d7591af8`); mid-file conditional form carved in its middle (`002beb0d5`); nested placement for a carved-again mid-file header (`379eed7ea`); trailing run of a carved colorspace header (`60c536c16`).
- B: a test-program caller under `tests/` (`361387486`).
- BP step 1: plain identifiers of other names, mixed groups (`6ebc9c988`).
- HN: exits with releases, a transformed clone, twin heads (`39c0be7c3`).
- CF step 7: event form, an indirect call outside per-pixel code (`babba7452`).
- MA step 2 (d): a read-only operand that differs, passed by value (`c5d7517b5`).
- IC step 5: a member binding read through a unary `*` (`ce850e6c9`); IC oracle form of the evidence for reached functions (`10a55aab8`).
- W step 7: twin form with a buffer per copy (`795a58aa6`).

## Owner questions

1. Confirm the 14 amendments above.
2. Does reading 1 treat txt/sparse-color output as a per-pixel hot path? If not, or if about +0.02 % is acceptable there, the four predicate headers can finish.
3. May `AccelerateUnsharpMaskImage` be bundled under the MagickPrivate form, given that its only call is commented out in `effect-sharpen.c`?
4. May a guard-level E/I step move an `OpenCLThrowMagickException` into a static helper (so `GetMagickModule()` names the helper)?
