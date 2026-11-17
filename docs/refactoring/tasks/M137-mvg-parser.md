# M137 - 2026-11-17, draw.c's MVG parser

Stacked on M136. M134, M135 and M136 (#114-#116) were merged into their stacked base
branches, not into `main`: `main` holds campaign B through M133. `campaign/M137` starts
from `campaign/M136` with `origin/main` merged in (0812f3440), so its PR to `main` carries
M134-M137. Scores come from cs-mcp through `tools/ch.py`.

Campaign B: **mean 9.11 over 97 files; files at 10.0: 42 (unchanged)**

## What was probed and dropped

- **annotate.c caption split** (annotate-caption.c, 9.38): annotate.c stays 5.83. The
  caption functions hold none of its findings, and its render family cannot leave
  (`RenderType` is a static that both public functions call). Not taken: it would raise
  only the campaign mean.
- **draw.c Recipe S splits**: draw.c stays 3.27 whichever family leaves. DrawPrimitive
  (66 functions, 3670 lines, after moving TracePoint, TraceRectangle and BezierQuantum
  into a private header) gives draw-primitive.c 3.04, below its original; DrawGradientImage
  8.02, DrawAffineImage 8.69; all three gone leaves the MVG parser alone at 3.27. The
  parser roots (DrawImage, DrawClipPath, DrawPatternPath) share one core that the
  keyword tables reach by address, so `split_source.py` cannot separate them.
- **Stub probe of draw.c's 66 flagged functions**: all stubbed 7.10; any one region
  (parser, the functions above it, those below it) stubbed alone 3.27; all but the
  parser 3.76; all but RenderMVGContent and TraceMVGPrimitive 5.39. The file score
  moves only once nearly every finding is gone, so per-step progress shows in function
  findings, not in the score.

## Commits

| Commit | What | Effect |
| --- | --- | --- |
| a69440f9d | tools: `split_source.py` moves a typedef only another moving typedef names | draw.c's EdgeInfo/PolygonInfo; byte-identical output on six real splits; new test |
| d3c34dd4b | RenderMVGContent, Recipe CS: four segment calls into PrepareMVGPrimitive | cc 43 -> 40; 476 cases, 0 diverged |
| 2229f6b73 | playbook: new Recipe SO | sibling copies in several functions, one returned and one pointer-written variable |
| a9ce834a3 | the four MVG push handlers, Recipe SO: the `pop <keyword>` scan into FindMVGPopKeyword | ParseMVGPushClipPath leaves Complex Method; PushGradient cc 22 -> 16, bumps 3 -> 2; PushPattern cc 19 -> 13, leaves Bumpy Road; 18 cases, 0 diverged |

draw.c stays 3.27 throughout: 66 flagged functions before, 65 after.

## Next in the parser

- **TraceMVGPrimitive** (cc 51, 444 cases): a `switch` over the primitive type inside a
  CS helper that copies the state into locals and writes them back, so SD's split cannot
  hand a helper `state` directly; its arms repeat a coordinate check, a Trace call, a
  pointer re-read and `i=j+coordinates`. Needs a recipe for that copy-in/copy-out form.
- **RenderMVGContent** (cc 40): the cleanup tail would take seven parameters under Recipe
  E; the remaining status breaks sit around debug log lines that must stay in the caller.
- **ParseMVGPopGraphicContext**: its nested clip-mask test is close to, but not the same
  text as, `IsClipMaskChanged` (operand order, `n != 0`), so Recipe P cannot reuse it.
- **ParseMVGClipPath** (two bumps): an inverted guard would flatten it, but Recipe G
  covers *Deep, Nested Complexity* with `return`/`continue`, not a `break` in `do { }
  while (0)`.

## Gate at the tip

- Every commit: `tools/oracle/build.sh cand`, `tools/ci_warnings.sh` and
  `tools/cxx_warnings.sh` on draw.c, `refactor_guard.py` and `--calls`, the oracle on the
  refactored functions' cases, and the CodeScene pre-commit gate: all passed.
- Whole catalogue at a9ce834a3 against `origin/main`: 15,961 cases, 3 diverged, all known
  from M136 (info/d333f9b818, gaps/2857e5295a, gaps/c5651ffad8).
- Nothing pushed.

## For the owner

- **Recipe SO** (new, under the 2026-10-24 mandate) needs your confirmation.
- **casemap.json rebuilt** (2026-11-17): the map of 2026-11-12 did not know the helpers
  extracted since (TraceMVGPrimitive and the MVG keyword handlers showed 0 cases). The
  old map is kept as `build-oracle/work/casemap-2026-11-12.json`.
- **This Mac's `python3`** is an x86_64 anaconda build: tool tests that call `gcc`
  (`tools/test_split_source.py`) fail under Rosetta, 17 of 29, unrelated to any change.
  Run them with `arch -arm64 /usr/bin/python3`.
- **resize/b41fd3fe32** (write mask + `-scale`, 32-bit float) diverged once in a full run
  under load and passed alone three times; the same family as `resize/6ee45e9cfa`, which
  ORACLE.md lists as random on the Mac. It is not on `LOAD_SENSITIVE` yet.
