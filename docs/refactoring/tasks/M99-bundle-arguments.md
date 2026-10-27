# M99 - Recipe B on existing static helpers

Not a generated task file. M98 left Recipe B for existing static functions that carry
*Excess Number of Function Arguments*; this round applies it. Branch `campaign/M99`, on
`main` at the merge of #52. Measured with `cs-mcp` 1.5.12.

### Report (2026-10-26, Mac, evening)

One Recipe B per commit, plus one playbook amendment.

| File | Function | Bundle | Evidence | Code Health |
|---|---|---|---|---|
| identify.c | PrintChannelMoments | file, scale, moments (11 calls) | oracle, ready, 18 cases | 3.23 → 3.29 |
| identify.c | PrintChannelLocations | file, image, statistic, limit, statistics (10 calls) | oracle, ready, 6 cases | flat at 3.29 |
| identify.c | PrintChannelStatistics | file, `1.0/scale`, statistics (15 calls) | oracle, ready, 197 cases | 3.29 → 3.37 |
| vision.c | IsMetricOutsideRange | min, max, background id (7 calls) | oracle, careful, 16 cases | flat at 2.06 |
| vision.c | IsAreaOutsideRange | the same struct | oracle, 18 cases | flat at 2.06 |
| vision.c | IsNeighborOutsideImage | dx, dy | oracle, 59 cases | 2.06 → 2.10 |
| mime.c | LoadMimeCache | cache, exception | oracle, careful, 14 cases | 5.91 → 6.85 |
| locale.c | LoadLocaleCache | cache, locale, exception | oracle, ready, 1,495 cases | 4.43 → 5.09 |
| delegate.c | LoadDelegateCache | cache, exception | oracle, ready, 357 cases | 2.39 → 2.60 |
| log.c | LoadLogCache | cache, exception | oracle, ready, whole catalogue | 3.40 → 3.77 |
| type.c | LoadTypeCache | cache, exception | oracle, careful, 89 cases | 4.21 → 4.70 |
| configure.c | LoadConfigureCache | cache, exception | oracle, careful (fresh mutation), 9 cases | 5.35 → 6.01 |
| segment.c | Classify | cluster threshold, weighting exponent, verbose | oracle, careful, 29 cases | 3.65 → 4.05 |

**The loaders.** The eight `Load*Cache` functions share one shape: a recursive XML
loader taking the cache, the XML, its path, the include depth and the exception. The
cache and the exception reach every call unchanged, the recursive include call
included. Bundling them as a `const <Name>LoadTarget` takes the loader to four
arguments. That clears *Excess Number of Function Arguments*, and with it *Brain
Method*, whose detection counts the arguments a function accesses. That second finding
is why these steps pay so much more than the other B steps. Each caller declares the
struct after the cache is assigned; the include branch declares its own from the
loader's unpacked locals. The loader unpacks the members into locals of the old names,
so its body is unchanged. Forward declarations and doc comments follow the new
signature.

**One file at a time.** A file's score moves only when its last *Excess Number of
Function Arguments* finding goes: vision.c stayed at 2.06 until all three predicates
were done. A trial on montage.c showed the same; with three of its four functions
cleared it stays at 6.61. That makes a file worth taking only when every one of its
argument findings is on a static function B can reach.

**Amendment.** Recipe B step 4 now allows an initialiser that is arithmetic on
identifiers and literals (`1.0/scale`). It is computed once, at the declaration; step 5
keeps its operands unchanged until the last call. PrintChannelStatistics' 15 calls
needed it. The literal guard warns that 14 copies of `1.0` are gone, as expected.

**Not taken.**
- color.c LoadColorCache: a fresh capped mutation run killed 2 of 37; the oracle hardly
  observes the colour configuration it loads. Not ready, so not touched.
- policy.c LoadPolicyCache: one call is in the `MAGICKCORE_ZERO_CONFIGURATION_SUPPORT`
  branch, which this build does not compile; the signature cannot change without
  touching it.
- annotate.c RenderFreetype: a second definition sits in the uncompiled `#else` of the
  FreeType test.
- fourier.c: the FFTW branch is not compiled on the Mac.
- montage.c: GetMontageGeometry's out-pointers cannot be bundled (`&local` is not an
  allowed initialiser), and without it the file stays flat.
- attribute.c GetEdgeBackgroundCensus, splay-tree.c Splay, draw.c GetFillAlpha: after
  bundling the arguments that do not vary, five or more remain, and those vary per call.

**Fresh mutation.** configure.c, color.c and type.c were mutated again (Mull build
`load99`, `--max-cases 200`; report `build-oracle/work/mutation-load99-all.json`).
The first attempt found no mutants in LoadConfigureCache or LoadColorCache: the line
profile (`oracle.profdata`) dated from 23 October, so `llvm-cov export` dropped
every function whose code had changed since, and the mutants had no function to belong
to. `tools/oracle/linecov.py` (5 minutes) rebuilt it at HEAD and fixed this. Rerun
linecov before any fresh mutation run on a file refactored since the last coverage run.

### Addendum: the night of 2026-10-26

Fresh capped mutation on the Mac (Mull builds `cand99` and `bm99`, line profile rebuilt
first each time) made more static Brain Methods eligible.

| File | Function | Bundle | Evidence | Code Health |
|---|---|---|---|---|
| feature.c | TraceEdges | edge image, view, canny cache, lower threshold | oracle, ready (28/29 fresh), 12 cases | 1.81 → 1.94 |
| log.c | TranslateEvent | module, function, line (the event's location) | oracle, ready (17/49 fresh, 31 unobservable), 21 cases | 3.77 → 4.18 |
| quantum-import.c | ImportRGBAQuantum | number_pixels, p, q (`QuantumPixelSpan`) | oracle, ready, 3,183 cases | 1.15 → 1.17 |
| quantum-import.c | ImportBGRAQuantum | the same span | oracle, ready, 61 cases | 1.17 → 1.19 |
| quantum-import.c | ImportRGBOQuantum | the same span | oracle, ready, 61 cases | 1.19 → 1.23 |
| quantum-import.c | ImportCMYKAQuantum, ImportCMYKQuantum, ImportGrayAlphaQuantum, ImportAlphaQuantum, ImportBlackQuantum | the same span, one commit each | oracle, ready | flat at 1.23 |
| quantum-import.c | ImportBGROQuantum, ImportMultispectralQuantum, ImportBGRQuantum, ImportIndexAlphaQuantum | the same span, one commit each | oracle, ready or careful (bm99), 35-61 cases each | 1.23 → 1.41 |
| quantum-export.c | ExportBGRAQuantum, ExportBGROQuantum, ExportRGBOQuantum, ExportMultispectralQuantum, ExportCbYCrYQuantum, ExportBGRQuantum | a `QuantumPixelSpan` of export's types, one commit each | oracle, ready or careful (bm99), 28-93 cases each | 1.24 → 1.57 |
| composite.c | BlendSumImage | attenuate, sign (literals at all four calls) | oracle, ready (14/16), 20 cases | 1.60 → 1.69 |
| composite.c | BlendMaxMagnitudeImage | the two magnitude images | oracle, ready (15/17), 8 cases | 1.69 → 1.80 |
| composite.c | CompositeOverImage | clip_to_self, x and y offset (5 calls) | oracle, careful (53/71), 553 cases | 1.80 → 1.92 |
| fx.c | ExecuteRPN | channel, x, y | oracle, careful (174/210), 767 cases | 1.81 → 1.93 |
| quantize.c | RiemersmaDither | image, view, cube info (25 calls) | oracle, careful (33/44), 373 cases | 1.95 → 2.09 |
| quantize.c | Riemersma | the same curve, passed down its recursion | oracle, ready (5/5, sweep25), 363 cases | flat at 2.09 (KmeansImage) |

The span bundles the three values that describe the pixels a helper transfers, not the
image and quantum info; members keep `magick_restrict`, and the helpers unpack them
into locals, which their loops advance as before. quantum-export.c gets its own
`QuantumPixelSpan` with export's types (`const Quantum *p`, `unsigned char *q`).
ImportGray, ImportRGB, ImportIndex, ExportGray, ExportRGB and ExportRGBA gate not
ready in the fresh run (18-51% adjusted) and wait for ERDC's uncapped figures.

Every step after the span (from BGRO on) removed *Brain Method* along with the argument
finding; none of these files is out of argument findings yet, so each gain is Brain
Method alone, 0.04-0.14 a step.

The `bm99` run (Mull, `--max-cases 200`, line profile rebuilt first) is in
`build-oracle/work/mutation-bm99-all.json`. fx.c GetOperand gates ready but its calls
pass `&UserSymbol`-style out-pointers, which no B initialiser may be.

Not taken: distort.c GenerateCoefficients (fresh run 200/286, 74% adjusted: not ready);
draw.c GetFillAlpha (careful, but one member would be `draw_info->fill_rule`, and five
arguments would remain).
