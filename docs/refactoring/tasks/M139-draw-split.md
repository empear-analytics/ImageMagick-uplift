# M139 - 2026-11-18, draw.c split, Phase 1

Stacked on `campaign/M138` (PR #124 not yet merged). The plan is
[M139-draw-split-plan.md](M139-draw-split-plan.md); the owner approved Phase 1 on
2026-11-18. Scores come from cs-mcp through `tools/ch.py`.

## Results

| File | Before | After |
| --- | --- | --- |
| draw.c (the MVG parser: DrawImage, DrawClipPath, DrawPatternPath) | 3.27 (12,140 lines) | **3.81** (6,616 lines) |
| draw-affine.c (DrawAffineImage) | - | 10 |
| draw-info.c (Acquire/Clone/Destroy/GetDrawInfo, GetAffineMatrix) | - | 10 |
| draw-gradient.c (DrawGradientImage) | - | 8.37 |
| draw-primitive.c (DrawPrimitive) | - | 3.43 |
| draw-core-private.h (shared statics, BezierQuantum) | - | 10 |

The results match the dry run exactly. Every product scores at least the original's
3.27.

## Commits

| Commit | Step |
| --- | --- |
| d5e1cf134 | split off DrawAffineImage (10 functions, 432 lines) |
| 81fdf6459 | split off DrawGradientImage (14 functions, 580 lines) |
| 6cfdbfffe | new draw-core-private.h with GetDrawValue; split off the DrawInfo family (15 functions, 457 lines) |
| e583b0fa0 | header gains TracePoint, IsClosedEllipse, SetTracedPrimitiveType, TraceRectangle and BezierQuantum; split off DrawPrimitive (89 functions, 4,019 lines) |

## Gate, every split

- `tools/split_source.py` (verbatim moves) and `tools/register_source.py`
  (Makefile.am, Makefile.in, Make.com, format_c_api_docs).
- A sorted-line verbatim check: nothing lost.
- `ci_warnings` on every touched .c file and `cxx_warnings` on each new one.
- `refactor_guard.py --combined` and `--combined --calls`, all OK.
- Whole narrow catalogue: 15,967 cases, 3 diverged, all known, after each split.
- **OpenMP** (gcc-16 -O2 builds, a fresh configure for the new file list):
  - `nm` shows each public root and its regions in the new object.
  - `omp_compare.sh` at 1 and 4 threads on `tools/omp_cases/draw-affine.txt` and
    `draw-gradient.txt`, the oracle driver's reflect and repeat gradients, six
    2000x2000 drawing workloads and three reset commands: 0 diverged.
  - Timing (median of alternating runs beside an A/A): within noise after re-runs.
    Each commit message gives its numbers.

## Not run on this Mac

- **The wide catalogue:** the X11 and OpenCL builds are Linux-side. draw.c compiles no
  X11 or OpenCL branch.
- **`tools/ci_linux_warnings.sh`:** it needs Linux (`nproc`). It should run on the Linux
  machine before merge, as should the wide catalogue.

## Next (Phase 2 of the plan)

Grind the products:
- **Complexity:** the parser's TracePath, TraceMVGPrimitive, RenderMVGContent and
  GetMVGMacros; draw-primitive.c's TraceStrokePolygon, DrawPolygonPrimitive,
  ConvertPathToPolygon and DrawDashPolygon.
- **Code Duplication** that the smaller files now show: about fifty `ParseMVG*`
  keyword handlers, the alpha/colour twins (a merge form that keeps each loop
  specialised; 460d384e0's merge cost 10.7%), and the pad/reflect gradient twins.

Ceilings from the plan's stub probes: draw.c 7.10 / 8.81, draw-primitive.c 7.78 / 9.38,
draw-gradient.c 9.38 / 10. Past them, Phase 3 needs the ruling on a header used by one
file (W38 question 4).
