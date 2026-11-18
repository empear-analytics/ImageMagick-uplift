# M140 - 2026-11-18, draw.c split, Phase 2 (first batch)

On `main` after #124 and #125. Scores from cs-mcp through `tools/ch.py`.

## Commits

| Commit | Recipe | Function | Effect |
| --- | --- | --- | --- |
| 66871c216 | D | GetReflectSpreadOffset (draw-gradient.c) | its opening block is GetPadSpreadOffset's body, now a call; one Code Duplication pair gone; draw-gradient.c stays 8.37 |
| 9200018f0 | docs | M138, M139 reports | object-code claims corrected (see below) |
| d96d88527 | Q | DrawDashPolygon | the dash-offset walk into SkipDashOffset; cc 24 -> 18, draw-primitive.c 3.43 -> 3.45 |
| 608a35c63 | D | ConvertPathToPolygon | seven identical allocation-failure exits into ThrowPolygonAllocationFailure; 187 -> 159 lines, 3.45 -> 3.50 |
| 65a038075, 3d568898a, 1ebd89b49 | N, N, Q | TraceStrokePolygon | the polygon assembly, the starting boxes and the first-segment scan into helpers; cc 43 -> 35, 362 -> 327 lines, 3.50 -> 3.55 |
| 6c5c0be35 | CS | RenderMVGContent | the first graphic context's setup and the release tail through render_state; cc 38 -> 35, draw.c 3.81 -> 3.83 |
| 5076e5f20 | Q | TraceMVGPrimitive | the text arm's cursor block into AdvanceMVGTextCursor; cc 43 -> 40; draw.c 3.83 -> 4.41, mostly an artifact (below) |

## Probed and dropped

- **GetPadGradientColor / GetReflectGradientColor** (draw-gradient.c):
  - A shared blend helper was itself reported as a clone (2 functions -> 3).
  - Recipe TI's merge removed the duplication, but the merged function is a Complex
    Method (cc 9) and GetSpreadGradientColor grew to a Large Method (73 lines), so
    8.37 -> 8.34.
- **TraceArcPointsP / TraceArcPointsQ** (draw-primitive.c, Recipe D merge into one
  TraceArcPoints):
  - The merge raised draw-primitive.c 3.43 -> 3.51, with 0 diverged on 47 cases and
    omp_compare 0.
  - gcc keeps the merged function out of line (`.isra`), where both twins had been
    inlined. A round-join workload at four threads measured +4.3% and +2.3% (A/A
    +0.3%, +0.4%), so the merge was not committed.
- **FlushSubpathEdge / FlushFinalEdge** (Recipe W, the shared edge store into
  StorePolygonEdge): the pair stays reported as duplicates and the count rises (12 -> 14
  functions); no gain.
- **The parser's keyword handlers:** a probe that strips their copy-in/copy-out
  boilerplate leaves 45 of about 55 clones. The duplication is in the parsing logic
  itself (token, option or value, assignment), so it needs a long chain of merges, not
  one step.

## A CodeScene artifact: duplication disappears above a size limit

draw.c's jump to 4.41 at 5076e5f20 is mostly not that step.

**What happened:** draw.c's whole *Code Duplication* finding (55 functions) stops being
reported once the file passes a code-size limit.

**Probes on the parent:**

| Parent with | Score | Code Duplication |
| --- | --- | --- |
| nothing changed | 3.83 | reported |
| only the moved block deleted | 3.87 | reported |
| one empty function added | 3.83 | reported |
| 60 comment lines added | 3.83 | reported |
| twelve trivial functions added (about 60 lines of code) | 4.38 | gone |

**What it means:**
- The 12,000-line draw.c never reported duplication, and its split products did. This
  is the same effect.
- The clones are still in the file.
- A step that shrinks draw.c back below the limit will bring the finding back.
- No step here pads a file to exploit this.

## A tooling error, found and corrected

`fndiff.py`, the scratch script that compares two objects function by function, called
`objdump` from a Python that runs under Rosetta. There `xcrun` cannot load, objdump
printed nothing, and the script reported no difference. Runs made with the OpenMP
environment sourced (anaconda out of PATH) worked; the others compared nothing.

The script now runs objdump by absolute path under the arm64 Python and fails on empty
output. Every claim that rested on an empty run was re-checked by recompiling both
versions:
- **The M139 split messages:** the claims hold up to alignment `nop`s.
- **66871c216's message:** amended before push; the step does change code, inlining
  GetScaledStopOffset, and was re-timed: within noise.
- **The M138 report:** its claim about 374142775 was false and is corrected there.

## Gate

- **Every commit:** build, warnings, guards, the oracle on the function's cases, and
  the OpenMP compare and timing.
- **Whole narrow catalogue at the tip:** 15,967 cases, 3 diverged, all known.

## Where Phase 2 stands

Every pair of twins left in draw-primitive.c and draw-gradient.c falls into one of
three kinds:
- **Per-pixel:** merges cost speed unless the merged function stays specialised.
- **Too different:** the twins differ in more than one value.
- **Still duplicates after a shared block is extracted.**

The complexity findings (the parser's TracePath, TraceMVGPrimitive and
RenderMVGContent; draw-primitive.c's TraceStrokePolygon, DrawPolygonPrimitive,
ConvertPathToPolygon and DrawDashPolygon) are the remaining lever toward the ceilings of
7.10 and 7.78, one large function at a time.
