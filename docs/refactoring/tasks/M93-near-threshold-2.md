# M93 - near-threshold functions, second round

Not a generated task file. This round continues [M92](M92-near-threshold.md)'s sweep on
the functions it did not reach. Branch `campaign/M93`, stacked on `campaign/M92` (PR
#45). Measured with `cs-mcp` 1.5.12.

### Report (2026-10-26, Mac, late morning)

| File | Function | Steps | Code Health |
|---|---|---|---|
| cache.c | DestroyPixelCache (cc 9 → 4, no finding left) | E `RelinquishPixelCacheResources` (guard: not ready, 3 of 6; oracle ran too, 15,097 cases) | flat at 2.16 |
| feature.c | RenderHoughLines (cc 9 → 7, no finding left) | E `CopyBlobToPrimitive` (guard: no mutants; oracle ran too, 21 cases) | flat at 1.81 |
| threshold.c | OTSUThreshold (cc 12 → 8, Bumpy Road gone, no finding left) | E `FindOTSUThreshold` (oracle, careful) | flat at 2.08 |
| image.c | SetImageInfoFromExtension (no finding left) | E `FindExplicitFormatType` (oracle, ready, 14,547 cases) | flat at 2.20 |

**Reverted.**
- delegate.c GetDelegateList. One predicate, `IsListedDelegateMatch`, would serve both
  the decode and the encode test. That cuts one `GlobExpression` call, which the
  `--calls` guard reports.
- No case executes GetDelegateList, so the step would be guard-level P with reuse. That
  is the question still open with the owner from #16, so it was not taken.
- Applying the predicate to one site only would leave cc at 9, still flagged.

**Not taken.**
- SyncOrientationFromProperties: see M92.
- ParseSceneRange: its loop body writes the shared cursors `p` and `q`.
- ReadBlobByte: it throws (`ThrowBlobException`).
- ImportScaleScanline: its channel loop needs five parameters.

**Recommended next: rebuild `casemap.json`.**
- The map dates from 24 October. Every function split since then maps to no case, so
  readiness checks and `oracle.py run --function` must go through the original function.
- Several M92 and M93 steps could therefore only be recorded as guard-level.
- `tools/oracle/casemap.py` (no argument) rebuilds it with the coverage build.
