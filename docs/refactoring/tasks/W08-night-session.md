# W08 - campaign A, night session (2026-10-29/30)

Not a generated task file. Campaign A (Windows desktop, WSL2), branch `campaign/W08`,
stacked on W07 (merged as #72) with `main` merged in at 97e0db6ba; the oracle's baseline
is `origin/main` at 727f2598d. Measured with `cs-mcp` 1.5.16, as W07. Each file was worked
by a subagent under one brief, one file at a time; the session reviewed each batch (every
new or amended recipe, every step that read a recipe closely) before pushing.

### Report

| File | Code Health | Main recipes |
| --- | --- | --- |
| `resize.c` | 4.78 → **6.50** | F, O, E, P, D |
| `feature.c` | 4.02 → **5.20** | G, E, N |
| `paint.c` | 4.14 → **5.10** | E, N, P, V |
| `layer.c` | 4.27 → **4.99** | E, D, P, O |
| `segment.c` | 4.05 → **4.93** | E, G, V, Y, D, N, P |
| `fourier.c` | 3.27 → **4.83** | D, E, P |
| `threshold.c` | 4.01 → **4.60** | N, E, Z |
| `vision.c` | 3.16 → **4.18** | E, N, D, P |
| `quantize.c` | 3.62 → **4.05** | E, N, D, H, T |
| `visual-effects.c` | 3.04 → 3.27 | E, N, H, B, Y, O, Q |
| `fx.c` | 2.85 → 3.08 | N, D, V, E |
| `distort.c` | 2.70 → 3.01 | N, Q, E, (member form) |
| `colorspace.c` | 2.80 → 3.01 | D, V, E, O, N |
| `composite.c` | 2.57 → 2.68 | H, T, D, V, E, O, P |
| `morphology.c` | 2.55 → 2.62 | V |
| `effect.c` | 2.02 → 2.54 | N, E, W, Q, U |
| `compare.c` | 1.83 → 2.18 | E, N, L, P |
| `enhance.c` | 2.97 (no step raises it) | - |

18 files, 231 commits (218 refactor steps, the rest playbook and tools), plus the merge of
`main`. Three files left the Red band (`fourier.c`, `vision.c`, `quantize.c`), and
`resize.c`, `feature.c` and `paint.c` passed 5.
Every refactor commit ran the oracle on its function's cases (0 diverged) and passed the
pre-commit gate, apart from the few commits on functions no case reaches, which say so
and were verified by the guard and by reading. The whole catalogue ran after each file
and at the tip: 15,900 cases, 0 diverged. CodeScene's PR gate passed on the pushes it had
finished checking by morning.

### Infrastructure

- **FFTW in the oracle** (53ac81813). ImageMagick's `configure` leaves FFTW out unless
  asked, so the WSL oracle builds had never compiled `fourier.c`'s transforms or
  `compare.c`'s `SIM*` functions, although PORTING.md counts on them. `build.sh` now
  passes `--with-fftw` (a no-op on the Mac). Baseline, candidate and coverage builds were
  rebuilt; `selfcheck --repeat 4` 0 nondeterministic, the case map rebuilt. This is what
  opened `fourier.c` (+1.56) and `compare.c`'s FFT region; the plateau rows the playbook
  holds for both files ("FFTW not compiled") are out of date.
- **`ci_warnings.sh` with `WIDE=1`** (9f35db3bf) checks the X11/OpenCL code against the
  wide build's configuration; before, it checked almost nothing in those six files.
- **Merged `main`** (97e0db6ba): the branch lacked campaign B's M108-M111, which the
  baseline has. Recipe E step 4 takes main's text (campaign B's amendment of 2026-10-28),
  which covers the reading the brief had used (see below).
- **`obj_same_code.sh` gave a false proof on Linux** (fixed in 192094c07). It needs
  `otool` (macOS); without it every listing failed inside a pipeline, both sides came
  out empty, and it printed SAME CODE having compared nothing. It now refuses with exit
  2. No commit relied on it: the Recipe F steps in `resize.c` were checked with an
  `objdump` stand-in (their messages say so), and the Recipe L steps in `compare.c` used
  `obj_identical.sh`, whose verdict is a byte comparison. A Linux port is left to you.

### The wide oracle cannot vouch for a change yet (for the owner)

With `WIDE=1` the full catalogue reports **153 diverged** cases, and exactly the same 153
when the baseline is built from the candidate's own commit (`--base HEAD`). So they are
not caused by any step: two wide builds of the same source disagree under the oracle,
while `selfcheck` (one binary against itself) is clean. About 120 are X11 `-window` cases;
the rest include `-resize 50%`, `-rotational-blur`, `-wavelet-denoise`, `-contrast`,
`-grayscale` - some of which use neither X11 nor OpenCL. Run by hand outside the oracle,
the two binaries give byte-identical output. A binary-dependent read of uninitialised
memory under the oracle's `MALLOC_PERTURB_` would fit; not confirmed. Until this is found,
a regression in those cases is indistinguishable from the noise, so **no step was taken
on `display.c`, `xwindow.c`, `widget.c`, `animate.c`, `accelerate.c` or `opencl.c`**. The
wide builds were rebuilt with FFTW (old ones kept as `*.nofftw`).

### Catalogue changes (each in its own commit, `playbook_lint` clean)

- **Recipe W** - Recipe J with an input struct (a parallel-loop block that writes
  `status=MagickFalse` and up to four accumulators or cursors). From `MotionBlurImage`.
- **Recipe Y** - Recipe V with an input struct. From `PlasmaImageProxy`.
- **Member form** (`name->member` instead of `const` locals, Recipe B step 3's form) in
  U and W, then N and Q: with `const` locals two sibling helpers (a plain and a blended
  sum) repeat each other's locals and are reported as Code Duplication.
- **U step 7**: a channel-loop body that `continue`s moves whole, `continue;` becoming
  `return(status);`.
- **V step 6**: a block that leaves through `return(c);` of one constant moves like a
  `goto` block; the caller returns `c`.
- **Q**: temporaries of a sequential block are declared in the new function; `&v` of a
  written scalar becomes `v`.
- **Recipe Z** - Recipe O with an input struct (a selection block that sets up to four
  locals, its inputs in a `const` struct). From `ColorThresholdImage`'s colorspace switch.
- **F step 2**: a non-`const` array whose every use is an element read or an argument to
  a `const` pointer parameter counts as never written and moves unchanged (`resize.c`'s
  `jinc_zeros`).
- **Recipe E step 4.** The brief told the subagents (W07 and W08) that a temporary the
  block writes before reading, and that nothing reads afterwards, is not an outer
  variable. My branch's playbook did not say so; main did (campaign B's amendment of
  2026-10-28), which I found only when merging main. I had written the same reading down
  as an amendment of my own (45990c7f8); the merge keeps main's text and drops my row.

### For the owner

- **Function-pointer merges** are now the main lever on several plateaus. `enhance.c`
  measured it: no legal plan moves 2.97, while merging its near-duplicate groups (the
  Modulate kernels, the row helpers, the FindStretch pair) measures 3.04 to 3.27.
  `morphology.c`'s two column loops and `composite.c`'s Darken/Lighten pairs are the same
  shape. A ruling (a recipe, or a refusal) would settle a good part of what remains.
- **Struct fills in hot loops.** The U and W steps in `effect.c` fill a 10-11 member
  struct per pixel and channel. Output is byte-identical; the cost is not measured.
- **The Hull reading of Recipe B** (W07) still awaits your ruling; nothing new used it.
- **Recipe E on a run of sibling statements.** E step 1 says "one nested block". A few
  steps moved a run of statements instead (`paint.c`'s `SetGradientRadii` and
  `SetGradientVector`, three in `resize.c`, one each in `layer.c` and `threshold.c`);
  each commit says so. The run writes only through a pointer the caller already holds,
  or ends the function (E step 7). Worth a ruling: an amendment, or a refusal.
- **Recipes asked for and not added** (described in the subagents' notes, each worth a
  measured gain):
  - a *sequential J/Q* (a block in sequential code that writes `status` and an
    accumulator and tests `status`, where a guard proves it true on entry): `vision.c`'s
    merge census (nesting 6);
  - *V with an out-parameter struct* (a block writing four caller variables and returning
    a constant): `fx.c`'s `GetFunction` coordinate block, its last Deep Nesting, +0.15 on
    a copy;
  - *cleanup then `return(v)`* where `v` was just set to NULL: `threshold.c`'s
    `GetThresholdMapFile`;
  - struct-valued read-only members in N and Q: `feature.c`'s `MeanShiftImage`;
  - V allowing dead cleanup writes on the failure path: `layer.c`'s
    `OptimizeLayerFrames`.
- **History edits before pushing.** One refactor commit (colorspace.c) had picked up an
  uncommitted hunk of my `ci_warnings.sh` change through `git commit -a`; the unpushed
  commits were rebuilt so the tool change sits only in its own commit (tree checked
  identical). Two subagents amended an unpushed commit of their own to fix its message.
  The brief now says `git commit -- <file>`.

### Where files plateau

- `compare.c` 2.18: the SIM row shells differ in more than one value; Complex Method from
  status/`continue` paths in parallel loops; unreached phase-correlation functions
  (`SIMPhaseCorrelationImage`, `SIMMagnitudeImage`, `SIMDivideByMagnitude`,
  `SIMFilterImageNaNs`, `SIMDerivativeImage`) were not touched - a case for
  `-metric phase` subimage search would reach them.
- `fourier.c` 4.83: `ComplexImages`' operator switch (eight inputs, two outputs in a
  parallel loop) wants an "O with input struct".
- `morphology.c` 2.62: `MorphologyPrimitive`'s intensity cases need a mutable output
  struct; `IterativeDistance` could reuse `MinimizeKernelDistance`, defined after it.
- `composite.c` 2.68: `CompositeImage` (about 1,800 lines, Brain Method) needs seven more
  case extractions, two pairs of which become clones.
- `colorspace.c` 3.01: the progress block under `omp atomic` and the parallel row loops'
  `continue` paths.
- `enhance.c` 2.97: duplication only a function-pointer merge removes; about 11 functions
  short of "Number of Functions in a Single Module".
- `fx.c` 3.08: probed further after the first pass; only `GetFunction`'s coordinate block
  (above) still moves the score.
- `resize.c` 6.50: `InterpolativeResizeImage`'s channel loop assigns and tests the shared
  `status`; `LiquidRescaleImage` is compiled (LQR is on) but **no case reaches it** - a
  `-liquid-rescale` case would open it.
- `threshold.c` 4.60: the parallel row loops' skeletons; trimming more of them joins the
  skeleton duplication group (the finding you suppress).
- `feature.c` 5.20, `paint.c` 5.10, `layer.c` 4.99, `segment.c` 4.93: the parallel loops'
  `status`/progress blocks, public signatures, and the recipes listed above; `layer.c`
  stopped at its commit budget with `CompareImagesLayers`, `CompositeLayers`,
  `ComparePixels` not yet probed.

### Noticed, not changed (possible upstream bugs)

- `fourier.c` `ForwardFourierTransformChannel`: the allocation-failure path tests
  `magnitude_info == NULL` before relinquishing it (should be `!=`), leaking it when only
  the phase allocation fails. `InverseFourier` copies after a failed
  `InverseQuadrantSwap`; `ForwardFourier` corrects the phase after a failed
  `ForwardQuadrantSwap`.
- `vision.c` `IntegralImage` tests `p` twice and never `q`; `AngleThreshold`'s
  `fabs(M11) < 0.0` is never true.
- `compare.c` `SIMMaximaImage` stores `p[i]`, not the NaN-filtered `pixel` as
  `SIMMinimaImage` does (probably harmless).
- `paint.c` `OilPaintImage`: on the clone-failure path `linear_image=DestroyImage(paint_image);`
  probably meant to assign `paint_image`.
- `resize.c` `InterpolativeResizeImage`: assigns the shared `status` per call in its
  parallel loop, and calls `InterpolatePixelChannels` once per channel although it does
  all channels.
- `feature.c` `CannyEdgeImage`: when `SetMatrixElement` fails, the `continue` skips
  `p+=...`, so the row pointer falls out of step with `x`; `TraceEdges` moves the centre
  in the middle of the 3x3 scan.
- `threshold.c` `GetThresholdMapFile`: its error paths `return(map)` right after
  `map=DestroyThresholdMap(map)` (always NULL, through a variable).
- `fx.c` `GetProperty`: the "not a number" path formats the already-destroyed (NULL)
  `text` with `%s` - "(null)" on glibc, undefined elsewhere - then destroys it again.
- W07's list stands (`DistortImage` and `HaldClutImage`'s shared `status`, and the rest).
