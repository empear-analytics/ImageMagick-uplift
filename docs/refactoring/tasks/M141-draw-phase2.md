# M141 - 2026-11-18, draw.c split, Phase 2 (continued)

On `main` after #127. Scores from cs-mcp through `tools/ch.py`. draw.c holds no OpenMP
region since the split, so its steps need no OpenMP check.

**Reading the scores near the duplication size limit.** cs-mcp stops reporting draw.c's
55-function *Code Duplication* once the file's code passes a size limit (M140's report).
draw.c is near that limit, so each commit says whether duplication was reported before
and after it. Where the step crossed the limit, the commit also gives a like-for-like
measurement, with both sides padded the same way so duplication stays hidden.

## Commits

| Commit | Recipe | Function | Effect | draw.c |
| --- | --- | --- | --- | --- |
| 772c2df40 | E | TracePath | the tail into FinishTracedPath, through the existing PathTrace | 4.14 -> 4.32 (duplication reported on both sides); no findings left |
| b79264bdf, 805be2433 | D, E | TraceBezier | three cleanup exits into ReleaseBezierBuffers; the coefficients into SetBezierCoefficients | 4.32 -> 4.50 (reported on both sides); no findings left |
| 3f8cc8453 | K | GetMVGMacros | the pop test into MatchMVGMacroPop | 4.50 -> 4.51 |
| dc7db2afc | N | GetMVGMacros | the macro extraction into AddMVGMacro | 4.51 -> 5.20, almost all of it duplication becoming hidden again; like for like 5.19 -> 5.20 |
| 5146a258a | CS | GetMVGMacros | the inner scan into ScanMVGMacro (below the Complex Method threshold this time) | 5.20 -> 5.45 (hidden on both sides); no findings left |

**draw.c now**, at 5.45 with duplication not reported, has five Complex Methods and four
Large Methods:
- **RenderMVGContent:** cc 35.
- **TraceMVGPrimitive:** cc 34.
- **TraceArcPath:** cc 10, 100 lines.
- **ParseMVGClass:** cc 9, 77 lines; one case.
- **IsValidListChar:** the allowed label run.

DrawPatternPath's five public arguments also stay.

## Gate

- **Every commit:** build, warnings, the guards, and the oracle on the function's cases.
- **Whole narrow catalogue at the tip:** 15,967 cases, 4 diverged. Three are the known
  ones. The fourth, resize/6ee45e9cfa, is the write-mask resize case ORACLE.md lists as
  random on the Mac: the cached baseline for the new `main` held one of its random
  outcomes, and with `--no-cache` it agrees (0 diverged). This branch touches only
  draw.c's parser.
- **check_trailers:** 6 refactor commits, all valid.
