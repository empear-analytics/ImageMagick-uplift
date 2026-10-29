# M111 - annotate.c predicate, image.c SetImageInfo

Not a generated task file. Branch `campaign/M111`, stacked on `campaign/M110` (#73). Campaign B
on the Mac, afternoon of 2026-10-29. Measured with `cs-mcp` 1.5.16.

### Report (2026-10-29, Mac, afternoon)

**annotate.c 2.92 → 2.93; image.c flat at 2.71 (SetImageInfo cc 42 → 35).**

| Step | Recipe | Evidence | Code Health |
|---|---|---|---|
| `RenderFreetype`: the inter-word spacing test to `IsInterwordSpace(draw_info,text,code)`, the glyph's text pointer passed; Complex Conditional gone (the two left in the file are in `ContainsRaqmParagraphSeparator`, compiled only with Raqm, which this build lacks) | P | oracle, 198 cases | annotate 2.92 → 2.93 |
| `SetImageInfo`: the subimage specification block to `ParseSubimageComponent(image_info)`, with its own scratch buffer (nothing wrote `component` before it) | E | oracle, 14,972 cases | flat 2.71 (cc 42 → 37, bumps 8 → 6) |
| `SetImageInfo`: the multiple-image support test's body to `TestMultipleImageSupport(image_info,component,exception)`; `component` passed, since earlier statements write it and `InterpretImageFilename` can return without terminating it | E | oracle, 14,972 cases | flat 2.71 (cc 37 → 35, bumps 6 → 5) |

Whole catalogue at the tip: 15,906 cases, 1 diverged (`gaps/c5651ffad8`, the known logcfg
artifact). Line coverage: `IsInterwordSpace` 13,063 calls, `ParseSubimageComponent` 61,737,
`TestMultipleImageSupport` 478.

**Tried and reverted.** `AppendImages`: the row copy inside the parallel loop to a helper,
flat, with a new Code Duplication finding on the helper.

**What blocks the rest.** `AnnotateImage`'s gravity, text-box and decoration blocks each read
six or seven locals; `RenderFreetype`'s rasterising loop is an OpenMP region writing `status`
and pixel state together.
