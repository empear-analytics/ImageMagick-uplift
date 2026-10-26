# M92 - near-threshold functions across MagickCore

Not a generated task file. This task used a sweep, not a single file. Every MagickCore
file was reviewed with `cs-mcp`, and the sweep listed each function whose findings sit
one step above a threshold: cc 9–11, exactly 2 bumps, or 70–80 lines, with at most two
findings. Each step below takes one such function under its threshold, or makes
measurable progress on it, with one plain Recipe E or P. Branch `campaign/M92`, stacked
on `campaign/M91` (PR #44).

Measured with `cs-mcp` 1.5.12. X11 files (display.c, xwindow.c, widget.c) were left out:
this build does not compile them.

### Report (2026-10-26, Mac, late morning)

25 refactor commits in 15 files. Every step ran the oracle on the refactored function,
or on the function it was split from when the case map (24 October) predates the split.
"No finding left" means the function no longer appears in the file's review.

| File | Code Health | Functions | Steps |
|---|---|---|---|
| enhance.c | 2.84 → 2.90 | ContrastStretchImage (cc 27 → 23), EqualizeImage, WhiteBalanceImage (cc 22 → 19), LinearStretchImage (no finding left), NegateColormap (P, no finding left), ClipCLAHEHistogram (no finding left) | 9 |
| morphology.c | 2.00 → 2.04 | AcquireGaussianKernel (cc 10 → 9), AcquireRectangleKernel (no finding left) | 2 (guard) |
| statistic.c | 2.27 → 2.28 | AcquirePixelTLS, ComputeChannelMoments (both: no finding left) | 2 |
| list.c | 6.40 → 6.45 | GetImageFromList (no finding left) | 1 |
| histogram.c | 6.34 → 6.39 | GetNumberColors (no finding left; E step 7) | 1 |
| mime.c | 5.87 → 5.91 | PadMimeType (no finding left) | 1 |
| linked-list.c | 8.95 → 9.02 | RemoveElementFromLinkedList (no finding left) | 1 (guard) |
| geometry.c | 3.38 → 3.40 | GetPageGeometry (no finding left; 11,939 cases) | 1 |
| layer.c | 3.77 → 3.79 | RemoveZeroDelayLayers (no finding left) | 1 |
| identify.c | 3.21 → 3.23 | PrintImageDirectory (no finding left) | 1 (guard) |
| segment.c | 3.63 → 3.65 | ForceCrossingEndpoints (no finding left) | 1 (guard) |
| xml-tree.c | 2.15 → 2.16 | CanonicalXMLContent (no finding left; E step 7) | 1 (guard) |
| profile.c | 2.59 → 2.60 | PatchCorruptProfile (no finding left) | 1 |
| quantize.c | flat at 1.95 | ReduceImageColors (no finding left) | 1 |
| string.c | flat at 3.76 | PrintStringInfo (no finding left) | 1 (guard) |

Twenty functions now have no finding left.

**Tried and reverted.**
- enhance.c EnhanceImage: Recipe H on its macros, then E of the per-row loop.
  - CodeScene counts the 25-line `EnhancePixel` macro, now directly above the helper,
    toward that helper. The helper's body is about 55 lines; it measured 80 and was
    flagged as a Large Method.
  - The score fell 2.90 → 2.81. Both steps were dropped, because the hoist has no
    follow-up without the extraction.
  - This is the include-block quirk ([M79](M79-identify.md)) applied to hoisted macros: a
    multi-line `#define` counts toward the function that follows it.
- enhance.c SigmoidalContrastColormap: extracting the sharpen branch cleared Complex
  Method and Bumpy Road. CodeScene then paired the remaining branch with the helper as
  Code Duplication: they differ only in `ScaledSig` versus `InverseScaledSig`, and a
  macro cannot be a parameter. No net gain.
- cache.c AcquireCacheNexusPixels: the allocation `if`/`else` took both of its bumps
  into the helper.

**Not taken.**
- SyncOrientationFromProperties: 14,911 cases make its oracle run cost 6–18 minutes for
  a single bump. It is left for ERDC.
- TraceSquareLinecap, GetConfigureInfo, RemoveElementByValueFromLinkedList: their loops
  write two or more locals that are still live, or they return mid-function.
- IsModuleTreeInstantiated: it throws (`ThrowFatalException`).
- ImportScaleScanline: five parameters.
- Standard row-loop functions (ContrastImage, LevelImage, ...): what is left is the
  loop skeleton. That is the status checks, the `continue` on a failed fetch, and a
  progress block under `omp atomic`, which must stay where it is.

**Guard-level steps and their gaps.**
- The case map predates several splits, so the oracle ran on the original function.
  Where the moved branch may not be reached, the step is recorded as guard-level:
  - NegateColormap: no case reaches the colormap path of NegateImage.
  - PrintImageDirectory: no known case lists a montage's directory.
  - RemoveElementFromLinkedList: the interior removal; the reports' line numbers are
    stale.
- Rebuilding `casemap.json` on the Mac would turn several guard-level steps into
  oracle-level ones.
