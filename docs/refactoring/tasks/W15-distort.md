# W15 lane b: distort.c, then the next campaign A files

Campaign A, branch `campaign/W15-b` from `main` 9db184dd4, 2026-11-07, worktree `~/im-w14b` with
its own base and candidate builds. Harness: Claude Code (Claude Opus 5.5), WSL2 Ubuntu. Code
Health from `cs-mcp` 1.5.16 through `tools/ch.py` (the committed baseline records 1.5.12; every
before and after score below is from 1.5.16). Owner rulings of 2026-11-07 used: the file may
pass 110 functions when the chain lifts it; label-run helpers may be *Complex Method* findings on
their own; Recipe S splits that `tools/split_source.py` accepts are allowed when the new file
scores at least what the original scored; no new OpenMP exceptions. The unreviewed Recipe N
step 2 amendment of `campaign/W15` (792cfaac0) was not used.

| File | Score | Commits |
| --- | --- | ---: |
| `distort.c` | 5.77 -> **7.15** | 12 code + 2 playbook |
| `enhance-tone.c` | 5.80 -> 5.99 | 1 (Recipe S) |
| `enhance-level-colors.c` (new) | 9.36 | (same) |
| `colorspace.c` | 7.35 -> 7.44 | 1 |

Every code commit passed the build, `tools/ci_warnings.sh`, `refactor_guard.py` (literals `OK`;
`--calls` moving only `+2` per new helper; `--combined` for the split), the oracle on the cases
that reach the function and the pre-commit CodeScene gate. No `--no-verify`. Whole catalogue:
15,900 cases, 0 diverged, after the `distort.c` chain (9ae3ea8fd) and again at the end of the
code commits (193f43562).

## distort.c: 5.77 -> 7.15

| Commit | Recipe | What moved | Score |
| --- | --- | --- | ---: |
| eda340558 | SW (rule 1 set of 3) | `poly_basis_fn`'s cases 15-20, 10-14, 6-9 -> `PolyBasisFnQuintic`, `...Quartic`, `...Cubic`, along the order comments (alone 5.68, 5.66, 5.65: the 111th function) | 5.77 -> 5.93 |
| 2a6a7aeb1 | SD | per-pixel mapping switch: Barrel, Shepards, default -> `MapDistortLensPoint`; `DistortPointInput tile` filled per pixel | 5.94 |
| fbc78d398 | SD | Polynomial to Plane2Cylinder + default -> `MapDistortCurvedPoint` | 6.05 |
| 84857df0b | Z | the rest of the mapping switch -> `MapDistortPoint(&tile,&s,&validity,&d)` | 6.18 |
| 412b34ba3 | SD (flat rule 1 set of 3) | best-fit switch: default label run -> `SetDistortNoBestFit`; Cylinder pair -> `SetDistortCylinderBestFit`; Polar/DePolar -> `SetDistortPolarBestFit` (targets struct, five written) | 6.18 (6.06, 6.11, 6.18) |
| 44747fbf3 | Z | the best-fit block -> `SetDistortBestFit(&fit,&geometry,&bestfit)` | 6.30 |
| 7c00dbda0 | playbook | Recipe PS step 6: member form and `const` initialiser for the input struct | - |
| 90edc9099 | playbook | Recipe N step 8: a PS nested-loop helper and PC's head share one struct | - |
| 9d80e97e7 | PS (step 7 nested, 8, 6 amended) | the column loop's body -> `DistortImagePixel(&line,&targets,i,&status)` | 6.51 |
| a72713c98 | PC (N step 8 amended) | the row body's head -> `DistortImageLine(&line,&status)` | 6.62 |
| 9af5a4589 | H | `DistortImageTag` above the function (`pp_identical` IDENTICAL) | 6.62 |
| 60ff40145 | N step 1 | the main block's statements with the whole parallel loop -> `RenderDistortImage(&render)`, returning `status` | 6.72 |
| 38643334a | I | geometry, best fit, viewport, verbose output, scale -> `LayoutDistortImage(&layout,coeff,&layout_targets)` | 6.80 |
| 9ae3ea8fd | Y | image creation and main block -> `CreateDistortImage(&create,coeff,&distort_image)` | 7.15 |

The end state was built on a scratch copy first (7.15) and every step measured on the way; the
order was chosen so that no step lowers the file: Z or PC applied before the SD steps or the
column-loop helper made a *Complex* or *Large* new function and lowered the file (6.18 -> 5.82,
6.18 -> 6.12, 6.30 -> 6.21, 6.30 -> 6.18), so the innermost pieces went first.

**Why two amendments.** The column loop's body (fourteen read-only inputs, `status` assigned from
`ResampleDistortPixel`, `validity`, `pixel` and `q` carried from pixel to pixel) is exactly PS step
7's nested-loop form with step 8's targets, but PS binds its struct as `const` locals, which makes
the helper a *Large Method* (about 87 lines by count). The member form, which N step 4 and Q
already admit, gives 60 lines and no finding. The row head is PC's shape, but with a struct of its
own PC's helper would bind sixteen locals and refill the row struct from them; sharing the PS
struct (N step 8's later-block form, which named only N, I and Y) gives 52 lines. Both are
amendments (relaxed preconditions of the same transformation), each in its own commit with a
*Catalogue changes* row, under the mandate of 2026-10-24.

**Performance** (oracle builds, no OpenMP, 1500x1500 gradient, median of 7, base 9db184dd4 vs
candidate): SRT 1.11/1.10 s, Perspective 1.22/1.23, Polar 1.50/1.49, Barrel 1.50/1.48, Shepards
1.03/1.03. The per-pixel struct fills (`mapping`, then `tile`) and the extra calls do not show.

**Final findings (7.15):** *Number of Functions in a Single Module* (125); *Overall Code
Complexity*; *Complex Method* `poly_basis_dx`, `poly_basis_str` (cc 22, label runs),
`SetDistortNoBestFit` (cc 9, label run); *Excess Number of Function Arguments* `DistortImage`
(6) and `SparseColorImage` (5), both public. *Lines of Code in a Single File*, *Bumpy Road*,
*Large Method* and every finding of `DistortImage` but its signature are gone.

**What stays blocked, and why.**

- `poly_basis_dx` and `poly_basis_str`: the split along their own order comments (as for
  `poly_basis_fn`) makes the new helpers and `poly_basis_fn`'s a *Code Duplication* group (5.60
  and 5.44 at the time, 6.73 now); other partitions found by search clone-free (one at 6.29) were
  not used, as that is choosing groupings to dodge the clone detector. Even stubs that leave six
  cases clone (7.32).
- `SetDistortNoBestFit`: a run of seven labels (one under `#if 0`) and `default`, one body.
- Recipe S: refused for `DistortImage` and `SparseColorImage` (both reach `GenerateCoefficients`
  and its 50 statics, `SyncDistortRow` and `SetDistortProgress`), so the function count stays.

## Next files

Lowest campaign A files after `distort.c` (scores at the start of this lane, 1.5.16), skipping
`composite.c`, `morphology.c` and `fx.c` as instructed:

- `compare.c` 2.97: saturated (W13/W14 measurements); nothing in the new rulings reaches it.
- `feature-texture.c` 5.25: blocked by the per-iteration-slot question (no new OpenMP
  exceptions). Moving whole parallel loops out lowers it (3.98 with eight loop helpers, 3.90 with
  five, 4.35 with the three sequential ones); W14's legal-only chain needs about 18 helpers in one
  rule 1 commit to pass 5.25 and ends near 6.2. Not started.
- `enhance-tone.c` 5.80 -> 5.99: the one family `split_source.py` accepts, `LevelImageColors`,
  split off (613c80785). The new `enhance-level-colors.c` scores 9.36 (LevelImageColors cc 12 and
  its public five arguments); its `invert` branch twins `LevelColorsPass` with `LevelizeImage` for
  `LevelImage`, a function-pointer merge no recipe covers.
- `fourier.c` 6.33: `#if defined` pairs, owner question unchanged.
- `segment.c` 7.09: the rest is in the unreached `GetImageDynamicThreshold` (guard-only recipes).
- `layer.c` 7.10: accepted splits are flat (1,341 lines left); the rest is `DEBUG_OPT_FRAME`,
  `ThrowImageException` and the scan clone cliff.
- `colorspace.c` 7.35 -> 7.44: the label-run ruling admits `TransformsRGBImage`'s 28-label group
  as an SW helper (193f43562). The same for `sRGBTransformImage` measures 7.46 in its real shape,
  but SW step 3 would then make the tail's `sRGBTableInput` the shared struct and the helper take
  five parameters; not done.
- `quantize.c` 7.38: past 110 functions is allowed now, but `Riemersma` moved whole or split still
  lowers the file (7.19, 7.12).
- `attribute-depth.c` 7.56: `split_source.py` accepts `GetImageDepth` or `SetImageDepth` (8.16
  each), but they carry compiled-elsewhere `#if !defined(MAGICKCORE_HDRI_SUPPORT)` branches, which
  the *Preprocessor branches* rule keeps in place: a question for the owner, like `fourier.c`'s.
- `enhance.c` 7.72: a split of the one-line `AutoLevelImage` or `NormalizeImage` raises it to 7.96
  only by moving one member of a reported duplication group out of CodeScene's view; declined.

A sweep of every campaign A file's public families through `split_source.py --out` found no other
split that raises its original.

## New files for the Windows build configuration

- `MagickCore/enhance-level-colors.c` (registered in `Makefile.am`, `Makefile.in`,
  `MagickCore/Make.com` and `scripts/format_c_api_docs` by `tools/register_source.py`).

## Upstream behaviour noticed, not changed

- `DistortImage`: when the render fails, `distort_image` is destroyed (NULL) and then, for an Arc
  distortion with best fit and no viewport, `distort_image->page.x = 0;` dereferences it.
- `DistortImage` (now `LayoutDistortImage`): when `PrintDistortVerbose` fails, the function returns
  NULL without releasing `coeff`.
- W14's notes on `SparseColorImage`'s clone and storage-class failure exits (they leak `coeff`)
  still stand.

## For the owner

1. The two playbook amendments (7c00dbda0, 90edc9099) were added under the mandate and await
   review.
2. 412b34ba3 is a flat rule 1 set (6.18 -> 6.18): it was kept under rule 2 as the targeted
   function moved towards its thresholds (cc 44 -> 32) inside a measured plan whose next step
   raised the file; rule 1's last sentence says a flat set is decided "as rule 2 decides a single
   flat step", then "kept when a targeted function leaves a finding category". `DistortImage` did
   not leave one in that commit. If the stricter reading is meant, that commit should have waited
   for a bundle with the Z step, which rule 1 does not allow (two recipes).
3. Questions still open from W14 and not covered by today's rulings: `#if defined` pairs
   (`fourier.c`, now also `attribute-depth.c`'s splits), the per-iteration slot of a shared
   function-local struct (`feature-texture.c`), the parked `--copy-static` splits
   (`enhance-tone.c`).
