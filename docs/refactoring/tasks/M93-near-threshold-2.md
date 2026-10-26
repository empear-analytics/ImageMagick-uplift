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

### Addendum: case map rebuilt (2026-10-26, 11:40)

`casemap.json` was rebuilt from this branch's tip: 15,884 cases, 5,157 functions, every
case profiled, in 18 minutes on the Mac. The previous map is kept as
`build-oracle/work/casemap-2026-10-24.json`.

The new map corrects three statements in [M92](M92-near-threshold.md) and in its commit
messages, which came from the stale map:

| Function | Old map | New map | Correction |
|---|---|---|---|
| NegateColormap | 0 cases | 7 cases | The colormap path of NegateImage *is* reached; it is not a gap. |
| PrintImageDirectory | 0 | 1 | A case lists an image directory. |
| UnlinkInteriorElement (RemoveElementFromLinkedList's interior branch) | not mapped | 3 | The interior removal was exercised by the oracle run of that step. |

Also reached: EncodeBase64XMLContent (1 case), ForceCrossingEndpoints (29),
AcquireGaussianKernel (238), ComputeChannelMoments (414), PadMimeType (1).

These steps keep the evidence level they were committed with: the readiness verdict
comes from the mutation reports, which do not change. Their oracle runs did reach the
moved code. A fresh mutation run of these functions on the Mac, now that the map can
route their mutants to cases, would raise them to oracle level.
