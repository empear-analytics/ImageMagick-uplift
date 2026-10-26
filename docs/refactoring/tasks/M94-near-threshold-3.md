# M94 - near-threshold functions, third round

Not a generated task file. This round continues [M92](M92-near-threshold.md) and
[M93](M93-near-threshold-2.md) on the near-threshold list, now with the rebuilt case
map. Branch `campaign/M94`, stacked on `campaign/M93` (PR #46). Measured with `cs-mcp`
1.5.12.

### Report (2026-10-26, Mac, midday)

7 refactor commits, one Recipe E each.

| File | Function | Effect | Evidence | Code Health |
|---|---|---|---|---|
| statistic.c | AddNodePixelList | no finding left | guard (not ready); oracle 80 cases | 2.28 → 2.29 |
| pixel.c | ConformPixelInfo | Complex Method and Bumpy Road gone; the helper keeps one Bumpy Road | guard (not ready); oracle 11,956 cases | flat at 1.10 |
| image.c | DestroyImage | no finding left (cc 11) | oracle, ready, 15,097 cases | flat at 2.20 |
| xml-tree.c | AddPathToXMLTree | Bumpy Road gone, cc 11 → 9 | guard (stale reports); oracle 3 cases | 2.16 → 2.18 |
| compare.c | GetPHASHSimilarity | Bumpy Road gone, cc 13 → 9 | guard (reports too stale to gate); oracle 14 cases | flat at 1.77 |
| composite.c | BlendMaskAlphaChannel | Bumpy Road gone, cc 11 → 9; a parallel loop's pixel loop under the OpenMP proviso | guard (no rows); oracle 20 cases | 1.58 → 1.60 |
| quantize.c | KmeansMetric | no finding left | guard (no rows); oracle 22 cases | flat at 1.95 |

**Tried and reverted.**
- pixel.c SetPixelChannelMask: Recipe H on `GetChannelBit`, then E of the
  channel-trait loop.
  - Both findings, Complex Method and Bumpy Road, moved into the helper (cc 10, 2
    bumps), and the score stayed flat.
  - The helper's alpha branch ends in `continue`, so it cannot be split further.
  - Both steps were dropped: the hoist has no follow-up without the extraction.

**Not taken.**
- SetPixelCacheExtent: its debug log records `__func__`, and its extend block returns
  from inside a preprocessor branch.
- ClonePolygonEdgesTLS: its blocks throw.
- IsExecutableToken, IsCacheMorphologyChanged, CCObjectInfoCompare: a single predicate
  or comparator ladder; nothing to extract.
- GetImageBackgroundColor: its loop adds into two accumulators (Recipe K territory).
- SetUserSpecifiedFormat: its blocks write two outputs or take three strings.
