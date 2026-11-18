# M139 plan - 2026-11-18, splitting draw.c

draw.c is 12,140 lines and 292 functions, scores 3.27, and has a ceiling of 7.10
even with every function finding gone: *Number of Functions in a Single Module* and
*Overall Code Complexity* are file-level. M138 took it from 123 function findings to
46 without moving the score. The way to 10.0 is Recipe S. **Recipe S needs the
owner's go-ahead per file**; this plan asks for it for draw.c.

All numbers below come from a dry run at 95b70fe74 (`tools/split_source.py` on a
scratch copy under `build-oracle/probe/split/`, scored by cs-mcp through
`tools/ch.py`). Nothing is committed yet.

## Why it is possible now

M137 found draw.c could not be split: the families shared too much. After M138, a call
graph of draw.c (keyword tables followed by address, public functions as boundaries)
shows five families, and only five statics shared between them:

| Static | Shared by |
| --- | --- |
| GetDrawValue (`static inline`) | draw-info lifecycle, MVG parser |
| TracePoint (`static inline`) | parser, primitive renderer |
| IsClosedEllipse (`static inline`) | parser, primitive renderer |
| SetTracedPrimitiveType | parser, primitive renderer |
| TraceRectangle | parser, primitive renderer |

These five functions and the macro `BezierQuantum` go into a new non-installed
`draw-core-private.h`. That is Recipe S step 2 under the owner's rulings of 2026-11-16:
a shared core header of any size, scored like a split product, with each including
file calling every non-inline helper.

## Phase 1: four splits (measured)

| Step | Product | Public roots | Functions | Lines | Score |
| --- | --- | --- | --- | --- | --- |
| 0 | draw-core-private.h | (five shared statics, BezierQuantum) | 5 | 94 | 10 |
| 1 | draw-affine.c | DrawAffineImage | 10 | 529 | 10 |
| 2 | draw-gradient.c | DrawGradientImage | 14 | 677 | 8.37 |
| 3 | draw-info.c | AcquireDrawInfo, CloneDrawInfo, DestroyDrawInfo, GetDrawInfo, GetAffineMatrix | 15 | 556 | 10 |
| 4 | draw-primitive.c | DrawPrimitive | 89 | 4,202 | 3.43 |
| - | draw.c (the MVG parser: DrawImage, DrawClipPath, DrawPatternPath) | | 164 | 6,616 | **3.81** |

Every product scores at least the original's 3.27, the Recipe S gate. `split_source.py`
accepted every step once `BezierQuantum` had moved to the header (it refused step 4
before: the macro was used on both sides). No public name, signature or linkage
changes; moved code is byte-identical.

Each step is one commit, in this order: the header first, then the splits. Each split:
- registered with `tools/register_source.py` (Makefile.am, Makefile.in, Make.com,
  format_c_api_docs);
- checked with the build in every configuration, `ci_warnings.sh`, `cxx_warnings.sh`
  and `ci_linux_warnings.sh --campaign-a`;
- checked with the whole oracle catalogue, narrow and wide;
- checked with `tools/omp_compare.sh` on `tools/omp_cases/draw-affine.txt` and
  `draw-gradient.txt` (the gcc-16 OpenMP build), plus a timing of each moved hot path
  against the baseline (the 2026-11-17 ruling).

Then `casemap.json` is rebuilt.

## Phase 2: grind the products (measured ceilings)

The same recipes as M138, now per file. The ceilings come from stub probes: the flagged
functions' bodies are emptied and the file is rescored.

| Product | After the split | Complexity findings gone | Code Duplication gone too | What stays |
| --- | --- | --- | --- | --- |
| draw.c (parser) | 3.81 | 7.10 | 8.81 | Number of Functions, Overall Code Complexity |
| draw-primitive.c | 3.43 | 7.78 | 9.38 | Lines of Code in a Single File |
| draw-gradient.c | 8.37 | 9.38 | 10 | - |

**New in the products: Code Duplication.** CodeScene did not report it on draw.c as one
12,000-line file. In the parser, about fifty `ParseMVG*` keyword handlers are flagged
as clones of one another; in draw-primitive.c the pairs ResetAlpha/ColorPixels,
ReplaceAlpha/ColorPixels, DrawAlpha/ColorPrimitive and the stroke tracers'
TraceMiterPointP/Q and TraceArcPointsP/Q; in draw-gradient.c the pad/reflect twins.

**Speed constraint.** The alpha/colour twins are where 460d384e0's merge cost 10.7%, so
they need a merge form that keeps each loop specialised. Recipe TI's forced-inline
selector is that form, and it needs timing.

**Complexity findings left after Phase 1:**
- **Parser:** TracePath, TraceMVGPrimitive, RenderMVGContent, GetMVGMacros,
  TraceArcPath, TraceBezier and ParseMVGClass.
- **draw-primitive.c:** TraceStrokePolygon, DrawPolygonPrimitive,
  ConvertPathToPolygon, DrawDashPolygon and GetFillAlpha.

## Phase 3: past the ceilings (needs rulings)

The parser and draw-primitive.c each keep one public root and many statics: the
keyword handlers and path tracers in one, the polygon fill and the stroke tracer in the
other. Splitting them further means moving a family of statics into a header that only
one file includes. That is W38's open **question 4** (a header used by one file),
unanswered.

Without it, the projected end state is:

| File | Projected end state |
| --- | --- |
| draw.c | about 8.8 |
| draw-primitive.c | about 9.4 |
| the other products | 10 |

With it, the projected path to 10 is:
- **Parser:** a keyword-handler header (the three keyword tables and their handlers) and
  a path-tracer header (TracePath and its per-command tracers).
- **draw-primitive.c:** a stroke header (TraceStrokePolygon's family).

## What not to expect

- **The original's score:** draw.c rises from 3.27 to 3.81 partly by relocating debt.
  W38's open question 2 asks whether such rises count; the products are the real
  targets.
- **Functions that stay as they are:** three have no oracle case and cannot be refactored
  (GetPolygonBounds, DrawBoundingRectangles, LogPrimitiveInfo; all land in
  draw-primitive.c); DrawPatternPath keeps its five public arguments; IsValidListChar is
  an allowed label run.

## Decisions for the owner

1. **Go-ahead for Recipe S on draw.c**: Phase 1 as above (the header and four splits).
2. **W38 question 4**: may a family header be included by one file only? Phase 3 depends
   on it.
3. **Order:** Phase 1 on its own branch (M139) after PR #124 merges, or stacked on it.
