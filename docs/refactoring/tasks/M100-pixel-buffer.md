# M100 - Recipe B on pixel.c's typed Export/Import helpers

Not a generated task file. M99 ended with the Mac's Recipe B candidates used up and the
rest waiting for ERDC's fresh mutation reports (`night3.sh`). This round finds one more
file the Mac can gate on its own. Branch `campaign/M100`, stacked on `campaign/M99`
(PR #53). Measured with `cs-mcp` 1.5.13, which Homebrew installed on the morning of
2026-10-27; M99 and the committed baseline used 1.5.12. The 1.5.13 sweep left untouched
files where the baseline has them (display.c 1.02, accelerate.c 2.06, opencl.c 2.85).

### Report (2026-10-27, Mac, afternoon)

**Result: pixel.c 1.10 → 1.73**, in 14 commits, one Recipe B each; and draw.c 1.79 → 1.90 (below).

| Function | Bundle | Evidence | Code Health |
|---|---|---|---|
| ExportCharPixel | map, quantum_map, pixels (`ExportPixelBuffer`, new) | oracle, ready (20/21, 1 unobservable), 29 cases | 1.10 → 1.11 |
| ExportDouble, Float, Long, LongLong, Quantum, ShortPixel | the same struct, one commit each | oracle, ready (21-28 of 21-28), 15-23 cases each | 1.11 → 1.27 |
| ImportCharPixel | map, quantum_map, pixels (`ImportPixelBuffer`, new; `pixels` is `const void *`) | oracle, ready (30/30), 129 cases incl. the dispatcher | 1.27 → 1.31 |
| ImportDouble, Float, Long, LongLong, Quantum, ShortPixel | the same struct, one commit each | oracle, ready (35-154 of 35-154), 14-20 cases each | 1.31 → 1.73 |

Each helper goes from six arguments to four and loses *Brain Method* with *Excess
Number of Function Arguments*. Its one call, in `ExportImagePixels` or
`ImportImagePixels`, passes `&buffer`, a `const` struct declared after the `roi`
assignments from the identifiers `map`, `quantum_map` and `pixels`, which nothing writes
before the call. `roi` stays a parameter: `&roi` would be an address-of initialiser,
which step 4 does not allow. The helpers unpack the members into locals of the old names
and types, so their bodies are unchanged. pixel.c had no *Typedef declarations*
section; the first step adds one after the includes.

**Why pixel.c was not ready before.** The merged reports showed five of these functions
with no case and the rest "not ready". Those reports predate both the `windrv` cases
(2026-10-04: `imdriver pixels export|import`, every storage type and map), which reach
every helper (14-30 cases each in the case map), and M39's extractions. A fresh run on
the Mac (Mull build `px100`, line profile rebuilt first, one `mutate.py --function` run
per function, uncapped in practice since no function has more than 135 cases) killed
896 of 899 mutants in the sixteen functions, with 3 unobservable: 100% adjusted
everywhere. Merged with the old reports, ImportCharPixel reads 60%: its 27 extra
survivors come from `mutation-erdc-sdl-pixel.json` and `sweep25`, at lines 2538-2671, a
pre-campaign layout; the function now spans 3358-3460. The commits cite the fresh run.

Full catalogue at the branch tip: 15,884 cases, 1 diverged, gaps/c5651ffad8 (logcfg), the known Mac harness artifact (M90). Baseline c8263ac3a5.

### Where pixel.c stops, at 1.73

- **The five Import functions M39 left unextracted** (Float, Long, LongLong, Quantum,
  Short): Recipe E moves their seven fixed-map blocks out, and each function improves
  (ImportFloatPixel: 9 bumps → 2, cc 60 → 25). But every one of them, taken alone,
  takes the file 1.73 → 1.71, because *Lines of Code in a Single File* leaves the review
  as the file passes a size threshold. A probe of all five, applied together and
  reverted, scored 1.71, 1.72, 1.74, 1.77, 1.80. Rule 2 reverts the first step, and the
  repository's pre-commit hook (CodeScene's safeguard) refuses any degrading commit, so
  no order of one-function commits gets there. **Question for the owner:** may these
  five go in as one commit, or the first three together (1.74, above the start)? That
  breaks rule 1's one function per commit, but each extraction is the same mechanical
  step M39 made nine times.
- *Excess Number of Function Arguments* stays on five functions, all public API:
  `ExportImagePixels`, `ImportImagePixels`, `InterpolatePixelChannel`,
  `InterpolatePixelChannels` and `InterpolatePixelInfo`. B cannot reach them, so the
  file-level finding stays.
- *Brain Method* stays on `InterpolatePixelChannels` and `InterpolatePixelInfo`: one
  `switch` per interpolation method with shared locals.

### draw.c GetFillAlpha (same afternoon)

**draw.c 1.79 → 1.90**, one Recipe B commit. M99 passed GetFillAlpha over: with
`draw_info->fill_rule` and `polygon_info[id]` ruled out as initialisers (a member access
and a subscript), five of its seven arguments seemed to remain. Bundling `mid`, `fill`, `x`
and `y` instead, in a `const FillAlphaSample` declared inside the `x` iteration just
before the one call, leaves four: `polygon_info[id]`, `&sample`, `fill_rule` and the
`stroke_alpha` out-pointer. Declared inside the parallel region, each thread has its own.
The function loses *Brain Method* together with *Excess Number of Function Arguments*.

Evidence: a fresh uncapped run (Mull build `fa100`, `--max-cases 400`, above its 356 cases)
killed 108 of 125 killable mutants, 86%, careful. The older reports' survivors for this
function were partly at stale positions (declarations and blank lines now) and partly
capped at 200 cases. The 17 survivors are boundary flips in the floating-point comparisons
of the alpha computation, which the step does not touch. 356 cases, 0 diverged.

The rest of draw.c's *Excess Number of Function Arguments* (IsNewSubpathPoint,
DrawClippingMask, DrawCompositeMask, IsDashSegmentPending, IsRenderableClass,
TraceArcPath, and the public DrawPatternPath) carries no Brain Method, and the public one
keeps the file-level finding, so B there would leave the score flat.

### Quantum, morphology and distort (evening)

**quantum-import.c 1.41 → 1.63, quantum-export.c 1.57 → 1.87, morphology.c 2.04 → 2.20,
distort.c 2.29 → 2.49**, eight Recipe B commits, each losing *Brain Method* with
*Excess Number of Function Arguments*.

| Function | Bundle | Evidence | Code Health |
|---|---|---|---|
| ImportRGBQuantum | `QuantumPixelSpan` (M99) | oracle, ready (95%, uncapped `q100`) | 1.41 → 1.48 |
| ImportIndexQuantum | the same | oracle, ready (95%) | 1.48 → 1.55 |
| ImportGrayQuantum | the same | oracle, careful (80%) | 1.55 → 1.63 |
| ExportGrayQuantum | export's `QuantumPixelSpan` (M99) | oracle, careful (85%) | 1.57 → 1.65 |
| ExportRGBQuantum | the same | oracle, ready (92%) | 1.65 → 1.75 |
| ExportRGBAQuantum | the same | oracle, ready (100%) | 1.75 → 1.87 |
| MorphologyPrimitive | method, kernel, bias (`MorphologyPass`, new) | oracle, careful (79%, ERDC fresh capped run) | 2.04 → 2.20 |
| GenerateCoefficients | number_arguments, arguments, number_values (`CoefficientInput`, new) | oracle, careful (86%, uncapped `q100`) | 2.29 → 2.49 |

**The 200-case cap, not the harness, made these functions "not ready".** M99's capped
runs gave the six quantum functions 18-51% adjusted and GenerateCoefficients 74%. The
uncapped `q100` run (`--max-cases 4000`) puts every one at careful or ready. A mutant
that survives only because the cases that would kill it lie beyond the cap is a
*capped* survivor, which the gate counts against the function. When a capped run says
"not ready" for a function reached by more than 200 cases, rerun it uncapped before
recording it as weakly protected.

MorphologyPrimitive's one call is in MorphologyApply's kernel loop, after an `if`
chain that sets `primitive` and `this_kernel`; the struct is declared just before the
call. MorphologyApply keeps Brain Method and seven arguments: it is `MagickPrivate`,
called from other files. GenerateCoefficients keeps its `DistortMethod *method`
out-pointer as a parameter (step 4 refuses `&method`); its two callers each declare
their own `CoefficientInput`. GetGeometry, PolaroidImage and GetImageDynamicThreshold
carry both findings too, but are public API.

**Load-induced divergences.** While `q100` ran, two full oracle runs each had one
`write-mask -scale` case diverge (`resize/6efddf0c0a`, `resize/6ee45e9cfa`, output
differs); alone they passed 3/3 and 5/5, on both the changed and unchanged code. A
third run had the *baseline* of two `compare -subimage-search` cases time out, and the
cache kept those timeouts, so they diverged on unchanged code until rerun with
`--no-cache`. Each affected step was reverted by the script and redone on a quiet
machine; every commit cites a clean run.

Full catalogue at `e77a54c39`: 15,884 cases, 1 diverged, gaps/c5651ffad8 (the known Mac
artifact).

### Harness notes

- **ERDC had not pushed since 2026-10-12.** `night3.sh` died silently on 2026-10-26 in
  morphology.c, most likely by the container's 64 GB limit with 28 jobs. Restarted on
  2026-10-27 with 16 jobs and a 4 GB per-process cap. Set for the whole script, the
  cap made the Mull build fail (`posix_spawn failed: Cannot allocate memory`), so it
  now wraps only the mutation step. The first five fresh reports (morphology,
  geometry, enhance, visual-effects, segment) arrived at 17:39; MorphologyPrimitive
  above uses them.
- **Homebrew moved fontconfig and pango.** Build directories configured before the
  upgrade keep the old Cellar include paths, so a source change to a file that includes
  them (type.c here) breaks the build. `linecov.py` failed on type.c this way. Fixed for
  `cov` with `./config.status --recheck && ./config.status`; `asan` and older `mull-*`
  directories still carry the old paths and need the same before reuse.
- **Uncapped quantum run.** A Mac run of the not-ready quantum functions at HEAD (Mull
  build `q100`, `--max-cases 4000`) costs about 90 s per surviving mutant
  (ImportIndexQuantum: 2,725 cases), roughly ten hours for the six functions. It finished at
  about 18:00 (results above).
