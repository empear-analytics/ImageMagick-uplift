# W51 round thirty-eight (2026-11-28)

Campaign A, lanes `campaign/W51-a` (colorspace and compare headers), `campaign/W51-b` (display-image headers, X11, accelerate) and `campaign/W51-c` (carves and splits), cut from main `4178bbf3e` and merged into `campaign/W51` after review. Started on the owner's "make as much 10 as possible" (2026-11-27). The lanes ran from 00:30 through the morning. The first merge (07:55) took every package that had passed review by then; a second merge (09:00) added lanes a-c's next reviewed packages and lane d (`campaign/W51-d`, `accelerate-unsharp.c` under the owner's 2026-11-28 answers). Later packages come in further merges to this branch.

## How the round ran

1. **Inventory** (106 agents): every campaign A `.c` file and every private header it includes was scored with cs-mcp on `4178bbf3e`. **104 were below 10.0** (44 `.c`, 60 headers). One agent per file worked out and stub-probed a route to 10.0. **66 had a legal route**; the other 38 are held by MagickExport argument counts or by the per-pixel Ir rule.
2. **Lanes**: three lanes took the 25 planned packages, then a queue of the remaining reachable files. Every package went to an independent reviewer that rescored, reran the oracle (`--base 46b61ab8d`) and checked the rules; up to two fix rounds; a package that failed three reviews was reset away.

## Result: 39 files to 10.0

| File | Before | After | How |
| --- | --- | --- | --- |
| `display-image-apply-private.h` | 7.55 | **10.0** | with the new `display-image-magnify-private.h` (10.0) (third merge) |
| `colorspace-lab-compatible-private.h` | 9.38 | **10.0** | Recipe EL under the owner's 2026-11-28 txt and histogram ruling; txt/sparse `WriteTXTImage` -0.0104 % to +0.0341 %, histogram `GetNumberColors` +0.0141 % and +0.0256 %, every other path at most +0.0042 % (lane e, third merge) |
| `colorspace-rgb-to-xyz-cmy-private.h` | 9.68 | **10.0** | two OH carves (`colorspace-rgb-xyz-cat02-private.h`, `colorspace-rgb-to-cmy-private.h`), B (third merge) |
| `resample.c` | 9.68 | **10.0** | S: the EWA resampler to `resample-ewa.c`, `struct _ResampleFilter` to `resample-filter-private.h` (third merge) |
| `accelerate-contrast-stretch.c` | 7.61 | **10.0** | guard-level chain for the worker of a wrapper no code calls (new rule paragraph), LM gather, MA, D (second merge) |
| `enhance-level.c` | 9.68 | **10.0** | S: `LevelImage` and `LevelizeImage` to `enhance-level-image.c` and `enhance-levelize.c` (9.68, MagickExport), shared helpers in `enhance-level-private.h` (second merge) |
| `colorspace-rgb-to-cie-private.h` | 9.68 | **10.0** | B, with the table entries in `colorspace-rgb-to-cie-entries-private.h` (second merge) |
| `colorspace-rgb-to-hue-private.h` | 9.68 | **10.0** | OH carve to `colorspace-rgb-to-hsv-private.h`, B (second merge) |
| `colorspace-rgb-to-lab-private.h` | 9.68 | **10.0** | OH trailing-run carve to `colorspace-rgb-to-lchab-private.h`, B (second merge) |
| `accelerate-unsharp.c` | 8.56 | 8.84 | B MagickPrivate form on `AccelerateUnsharpMaskImage` (owner 2026-11-28), B on both Compute functions, D; *Complex Method* 1 and *Large Method* 2 left (the throw-helper probes lowered the file) |
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

- Gates on the second merge, `0317f3b56`: `ci_linux_warnings --campaign-a` OK (166 files); narrow 16,004 cases, 1 diverged: `infra/0fd402f13a`, the case recorded as load-sensitive (load average about 24 during the run); rerun alone 3 times, 0 diverged each. Wide 14,249 cases, **0 diverged** (8 settled as load-sensitive). `check_trailers` 154 refactor commits valid; `playbook_lint` 195 recipes, 610 change rows, 0 problems. Sweep: campaign A 166 `.c` files, mean **9.90**, **123 at 10.0**; MagickCore 9.68.
- Gates on the third merge, `91d3a05d1`: `ci_linux_warnings --campaign-a` OK (167 files); narrow 16,004 cases, 1 diverged: `infra/0fd402f13a` again (load average about 26), rerun alone 3 times while the lanes kept the load average near 25: 0 diverged twice (17 s and 233 s), 1 diverged once (247 s; its report was not kept); the case is recorded as load-sensitive in the 2026-11-20 rulings, and no commit of this round touches the code it runs; wide 14,249 cases, **0 diverged** (7 settled as load-sensitive). `check_trailers` 162 refactor commits valid; `playbook_lint` 195 recipes, 613 change rows, 0 problems. Sweep: campaign A 167 `.c` files, mean 9.90, **124 at 10.0**; MagickCore 9.68.
- **Merge correction:** the first try at the second merge resolved `PLAYBOOK.md` with the old section-level resolver, which dropped 47 lines of lane b and 4 of lane d (lines outside the conflict). It was reset before any push and redone keeping both sides' change-log rows, the only real conflict. Checked after: every line any lane added is present, and every line of main's playbook is present unless a lane removed it.
- Every package reviewed by a separate agent: oracle `--function` (narrow, and `WIDE=1` for X11/OpenCL code), object comparisons where no case reaches the code, per-pixel Ir where objects changed, release flags, trailers.
- No `--no-verify`.

## Dropped or held

- **Dropped after three failed reviews:** `display-image-matte-private.h` (messages stated narrow-only reach counts and, after rewrites, cited rewritten hashes). Lane b was reset to before the package.
- **Held on the per-pixel Ir rule (reading 1):** `colorspace-rgb-predicates-private.h`, `colorspace-lab-compatible-private.h`, `colorspace-predicates-private.h`, `colorspace-rgb-to-luma-private.h`. Each reaches 10.0 in a probe, but the txt/sparse-color writers call these predicates once per pixel and every form measured adds instructions there: +0.013 % to +0.59 % against the 0.01 % cap (rgb-to-luma: +0.17 % per non-black pixel). Nothing committed.
- `colorspace-rgb-predicates-private.h` and `colorspace-lab-compatible-private.h`: the owner accepted the txt/sparse-color rise and then extended the acceptance to histogram text output (2026-11-28); lane e is redoing both under the extended ruling. The hue predicates and `ConvertRGBToCMYK` stay held (their rise is in image processing paths).
- **Not finished:** `display-image-window-setup-private.h` (the agent was cut off before its first commit; to be retried).
- Still running at the second merge: the remaining queue (colorspace and compare-similarity headers, display-image headers, `resample.c`, `quantum-private.h`, `image-private.h`, `xwindow-image.c`).

## New recipes and amendments (mandate 2026-11-19)

Confirmed by the owner on 2026-11-28 ("Amendments are ok"):

- UB, kernel-string form (`27d5d44bd`): verbatim carve of an OpenCL kernel-string run between macro calls.
- OH: upstream shared mid-file form (`eac2a6cb7`); declarations in that form's run (`3d7591af8`); mid-file conditional form carved in its middle (`002beb0d5`); nested placement for a carved-again mid-file header (`379eed7ea`); trailing run of a carved colorspace header (`60c536c16`).
- B: a test-program caller under `tests/` (`361387486`).
- BP step 1: plain identifiers of other names, mixed groups (`6ebc9c988`).
- HN: exits with releases, a transformed clone, twin heads (`39c0be7c3`).
- CF step 7: event form, an indirect call outside per-pixel code (`babba7452`).
- MA step 2 (d): a read-only operand that differs, passed by value (`c5d7517b5`).
- IC step 5: a member binding read through a unary `*` (`ce850e6c9`); IC oracle form of the evidence for reached functions (`10a55aab8`).
- W step 7: twin form with a buffer per copy (`795a58aa6`).

Added after the first merge, confirmation pending:

- Two levels of evidence, new paragraph *Unreached functions: the worker of a wrapper no code calls* (`aa29c5d05`).
- LM step 8, gather form: locals gathered into a new struct local (`dc7b73244`).
- MA step 3, new input struct with two bases (`0406804c1`); MA step 8, precomputed levels form (`3a6494671`).
- D: a helper in a directly included private header (`1eff8cc09`).

`5cba0d023` records the owner's 2026-11-28 answers in the playbook.

## Owner answers (2026-11-28)

1. The 14 amendments above: confirmed.
2. Reading 1: the txt and sparse-color writers, and then histogram text output, may take the measured rise (about +0.02 % to +0.07 % of the writer); every other path keeps 0.01 %.
3. `AccelerateUnsharpMaskImage` may be bundled under the MagickPrivate form (the commented-out call in `effect-sharpen.c` stays verbatim).
4. A guard-level E/I step may move an `OpenCLThrowMagickException` into a static helper.

## Owner questions

1. Confirm the five amendments added after the first merge.
