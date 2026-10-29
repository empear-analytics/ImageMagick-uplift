# M112 - Recipe W, identify.c, the sRGB table, xml-tree buffers

Not a generated task file. Branch `campaign/M112`, from `main` after #74 merged. Campaign B on
the Mac, evening and night of 2026-10-29. Measured with `cs-mcp` 1.5.16.

### Report (2026-10-29, Mac, night)

**identify.c 3.37 → 3.48, xml-tree.c 2.39 → 2.44, image.c 2.71 → 2.77, stream.c 3.13 → 3.14,
profile.c 2.60 → 2.92, pixel.c 2.36 → 2.38.**

Catalogue changes, each in its own commit before the steps that use it:

- Recipe D: a shared helper may write the copies' variables through pointers (as Recipe O) or
  return one of them. The owner left the question from #70 and #73 to the campaign ("as you
  see fit").
- **New Recipe W**, Extract Block Returning Success for an Early Return: a block in the middle
  of a function that leaves through `return(R);` moves to a `static MagickBooleanType`
  function, and the caller writes `if (Helper(...) == MagickFalse) return(R);`. No earlier
  recipe reached a mid-function `return`: Recipe E covers only the throw macros and blocks that
  end the function, and Recipe V covers only `goto`.
- Recipe W, step 3 amended: locals that nothing after the block reads don't count, and neither
  do locals written only on paths that return.
- Recipe F, step 2 amended: a static that isn't declared `const` qualifies when every use of it
  is `sizeof` or an argument to a pointer-to-`const` parameter.

| Step | Recipe | Evidence | Code Health |
|---|---|---|---|
| `IdentifyImage`: the `identify:locate` colorspace switch to `PrintColorspaceLocations(image,colorspace,report)`; the report struct, all five fields set, goes by value | E | oracle, 283 cases | identify 3.37 → 3.43 |
| `IdentifyImage`: the artifact listing to `PrintImageArtifacts(file,image)` | E | oracle, 283 cases | 3.43 → 3.45 |
| `IdentifyImage`: the colormap block to `PrintImageColormapEntries(file,image,exception)` | E | oracle, 283 cases | 3.45 → 3.47 |
| `IdentifyImage`: the time-to-live block to `PrintImageTimeToLive(file,image)` | E | oracle, 283 cases | 3.47 → 3.48 |
| `EncodePredefinedEntities`: the buffer-growing block to `ExtendXMLBuffer(source,length,extent,text)` | W | oracle, 31 cases | flat 2.39 (rule 2: the helper is shared in the next step) |
| `XMLTreeTagToXML`: its four identical blocks call `ExtendXMLBuffer`, each keeping its own `return` at the call | D | oracle, 31 cases | xml-tree 2.39 → 2.44 |
| `SetImageInfo`: the copy of a non-seekable blob to a seekable temporary file to `CopyImageToSeekableFile(image_info,image,component,exception)` | W | oracle, 14,972 cases (4 take the copy path) | image 2.71 → 2.77 |
| `WriteStreamImage`: the stream preparation to `PrepareStreamForWriting(stream_info,image,length)`; its `return(0)` becomes `return(MagickFalse)` | W | oracle, 261 cases | stream 3.13 → 3.14 |
| `SetsRGBImageProfile`: the 270-line ICC table `sRGBProfile` to file scope; `obj_same_code.sh`: SAME CODE | F | oracle, 9 cases | profile 2.60 → 2.92 |
| `InterpolatePixelChannel`: the alpha/pixel loading block of the average, bilinear, mesh, catrom and spline cases to `LoadInterpolatePixels(...)`; they differ only in the pixel count | D | oracle, 96 cases | pixel 2.36 → 2.38 |

Whole catalogue at the tip: 15,906 cases, 1 diverged (`gaps/c5651ffad8`, the known logcfg
artifact).
Line coverage at the tip: every new function runs (`PrintColorspaceLocations` 6 calls; `PrintImageArtifacts`, `PrintImageColormapEntries`, `PrintImageTimeToLive` 216 each; `ExtendXMLBuffer` 5,245; `CopyImageToSeekableFile` 15,904; `PrepareStreamForWriting` 12,029; `LoadInterpolatePixels` 841,804; `SetsRGBImageProfile` 20).

**Tried and reverted.**
- `LoadDelegateCache`: the DOCTYPE skip loop to a function returning `q`, 2.61 → 2.58 (its
  if-else chain makes the new function deeply nested). The attribute switch would move a
  Windows-only `#if` branch, so it is not touched.
- `IdentifyImage`:
  - the whole locate block, which returns, to a function: 3.43 → 3.37 (complex, large,
    five arguments).
  - the statistics colorspace switch, like the locations one: 3.43 → 3.42 (the new function
    has complexity 17).
  - with the features switch added as well: 3.22.
  - the transparent-pixel search: 3.43 → 3.39 (its nesting moves into the helper).
  - the registry listing: 3.47 → 3.32 (near-duplicate of `PrintImageArtifacts`).
  - the non-verbose summary: 3.48 → 3.39.
- `InterpolatePixelChannels`: the Average case's channel loop body, flat at 2.38, with six
  arguments.

**Not done.**
- `GetMagickPropertyLetter` (delegate.c) and `NewXMLTree` throw through `GetMagickModule()`,
  which names the function in the warning text.
- `OpenPixelCache`'s memory block returns two different values, so Recipe W does not apply.
- `XMLTreeInfoToXML`'s two buffer-growing blocks write plain locals (`xml`, `extent`), and its
  two processing-instruction loops differ in two places.
- `InterpolatePixelChannel`'s Blend case loads its pixels with a `MagickRealType` cast. It
  differs from the shared block in a second place, so it stays.
- `GetEXIFProperty`'s tag table: hoisted in M101, score flat.
