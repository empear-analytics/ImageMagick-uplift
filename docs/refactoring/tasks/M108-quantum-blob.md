# M108 - 12-bit and CbYCrY switches, 32-bit Index, blob.c

Not a generated task file. Branch `campaign/M108`, from `main` after #67 merged. Campaign B on
the Mac, day of 2026-10-29. Measured with `cs-mcp` 1.5.16.

### Report (2026-10-29, Mac, day)

**quantum-export.c 4.09 → 5.02, quantum-import.c 3.48 → 3.83, blob.c 2.21 → 2.30.**

| Step | Recipe | Evidence | Code Health |
|---|---|---|---|
| `ImportUnpacked12BitSample`: the six copies of the `% 3` channel switch in the 12-bit unpacked RGB and BGR import helpers, the switch value passed as `index`, the advanced `q` returned | D | oracle, 7,214 + 61 cases | import 3.48 → 3.78 |
| `ExportUnpacked12BitSample`: the first copy in `ExportUnpacked12BitBGRQuantum`; it writes `pixel` and `p`, so both go by address | O | oracle, 93 cases | export 4.09 → 4.14 |
| the same helper for the other five copies in the BGR and RGB export helpers | D | oracle, 93 + 6,806 cases | export 4.14 → 4.58 |
| `Import32BitIndexQuantum`: both branches of the float test end in `return(range_exception);`, so each moves to a function of the caller's type (`ImportFloat32BitIndexQuantum`, `ImportInteger32BitIndexQuantum`) | E (step 7) | oracle, 2,725 cases | import 3.78 → 3.81 |
| the same for `Import32BitIndexAlphaQuantum` | E (step 7) | oracle, 52 cases | import 3.81 → 3.83 |
| `ExportCbYCrYSample`: the `n % 3` sample switch, two copies (`ExportCbYCrYQuantum`, `ExportUnpacked10BitCbYCrYQuantum`); it has no default, so `quantum` goes in and comes back | D | oracle, 28 cases | export 4.58 → 5.02 |
| `OpenBlob`: the map-or-not test to the predicate `CanMapBlobFile`, operands in order (so `AcquireMagickResource` runs only when the format supports blobs, as before); Complex Conditional gone | P | oracle, 14,879 cases | blob 2.21 → 2.26 |
| `OpenBlob`: the memory-mapping block to `MapBlobFile` | E | oracle, 14,879 cases | blob 2.26 → 2.30 |
| `OpenBlob`: the mode switch, which sets `flags` and `type` together, to `GetBlobModeFlags` | O | oracle, 14,879 cases | flat 2.30 (OpenBlob cc 60 → 53, 314 → 270 lines) |
| `ReadBlob`: the file/pipe case's `switch (length)` to `ReadBlobFile`, returning `count` | E | oracle, 14,324 cases | flat 2.30 (ReadBlob cc 51 → 39, 217 → 169 lines, bumps 4 → 3) |

Whole catalogue at the tip: 15,906 cases, 1 diverged (`gaps/c5651ffad8`, the known logcfg
artifact).
Line coverage at the tip: every new function runs (`ImportUnpacked12BitSample` and
`ExportUnpacked12BitSample` 3,600 calls each; the four 32-bit Index helpers 52 or 9;
`ExportCbYCrYSample` 10,920; `ExportUnpacked10BitCbYCrYQuantum` 9; `GetBlobModeFlags` 76,036,
`MapBlobFile` and `CanMapBlobFile` 49,947, `ReadBlobFile` 14,093).

**Tried and reverted.**
- `ImportQuantumPixels`: its dispatch switch to a helper, 3.83 → 3.82 (the switch moves, its
  complexity with it, and the helper takes five arguments).
- `OpenBlob`: the multi-part filename block, flat, with a new five-argument finding.
- `WriteBlob`: the case matching `ReadBlobFile`, flat, with two new findings (a five-argument
  helper, and duplication with `ReadBlobFile`).

**Not done.**
- `property.c`'s Code Duplication is the assert-and-trace prologue of the small accessors;
  each logs its own name through `GetMagickModule()`, so sharing it would change the trace log.
- The import side's `n % 3` switches (five copies) write `p`, `pixel` and `quantum`, and
  `pixel` is a parameter of the helper that holds them, which Recipe O does not allow.

**Questions for the owner.**
- The 12-bit export helper is Recipe O on the first copy, then Recipe D reusing that helper
  for the other copies, its two written variables passed by address at each site. D does not
  say whether the shared helper may write through pointers; I read it as allowed, since each
  copy wrote the same two locals. Is that right?
- `Import12BitBGRQuantum` and `Import12BitRGBQuantum` (unpacked) are now the same function
  under two names, since the BGR one stores R,G,B (reported in #67). Leave both as they are?
