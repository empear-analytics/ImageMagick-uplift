# M106 - PixelChannel pair, image.c, xml-tree.c and log.c predicates

Not a generated task file. Branch `campaign/M106`, stacked on `campaign/M105` (#65). Campaign
B on the Mac, early morning of 2026-10-29. Measured with `cs-mcp` 1.5.16.

### Report (2026-10-29, Mac, night)

| File | Step | Recipe | Evidence | Code Health |
|---|---|---|---|---|
| xml-tree.c | `IsInsideXMLTag`, three sites in `NewXMLTree` | P | oracle, 150 cases | 2.29 → 2.39 |
| log.c | `IsIncludeElementOpen` in `LoadLogCache`, as the other loaders have it | P | oracle, 15,903 cases (only the known logcfg case diverged) | 4.18 → 4.20 |
| image.c | `ParsePageGeometryOption` (page, tile-offset) in `SyncImageSettings` | D | oracle, 15,096 cases | flat 2.71 |
| image.c | `SetImageResolutionFromDensity` (two copies) in `SyncImageSettings` | D | oracle, 15,096 cases | flat 2.71 |
| quantum-import.c | `ImportPixelChannel` takes the span (6 → 4 arguments) | B | oracle, 287 cases | flat 3.21 |
| quantum-import.c | its depth cases (`tools/extract_switch_case.py`), then the 16/32-bit blocks | E | oracle, 287 cases | 3.21 → 3.29 |
| quantum-export.c | `ExportPixelChannel` takes the span | B | oracle, 353 cases | flat 3.78 |
| quantum-export.c | its depth cases, then the 16/32-bit blocks | E | oracle, 353 cases | 3.78 → 3.99 |

**`tools/extract_switch_case.py`** moves one case of any top-level switch that follows
computed locals: locals assigned before the switch and read by the case are passed by value,
the case's other locals (and loop indices used only in the loops that set them) are declared
afresh, and a case writing a local read after the switch is refused.

Line coverage at the tip: every function this batch created is executed by some case.
Whole catalogue: 15,906 cases, 1 diverged (`gaps/c5651ffad8`, the known logcfg artifact).

**Tried and dropped.**
- stream.c `StreamImagePixels` (cc 291): the seven storage-type cases extract correctly, but the
  helpers, and the per-map blocks inside them, are near clones; the file fell 3.13 → 2.05
  (2.66 with the blocks split). Not committed.
- image.c `AcquireImage`'s density copy: reusing `SetImageResolutionFromDensity` needs the
  helper above `AcquireImage`, making it the file's first function, which CodeScene charges with
  the include block (2.71 → 2.63). Not committed; a forward declaration would avoid it, but no
  recipe covers one.
- pixel.c `InterpolatePixelChannel`: its method cases read eight or nine locals and
  parameters each, so extracting them only trades the Brain Method for Excess Arguments findings.
- Complex Conditionals left in B files: annotate.c's are in Raqm code, not built here; blob.c's,
  profile.c's and xml-tree.c's other ones have side effects in the test; policy.c's needs
  `sizeof` of an array.
