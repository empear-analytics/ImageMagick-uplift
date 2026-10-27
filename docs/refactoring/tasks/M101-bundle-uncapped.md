# M101 - Recipe B after uncapped mutation runs

Not a generated task file. PR #54 (M100) was merged at `04ccce3dc` on the afternoon of
2026-10-27; this round continues on `campaign/M101`, stacked on that merge (`50e8137d9`,
also the oracle's baseline). Measured with `cs-mcp` 1.5.13.

### Report (2026-10-27, Mac, evening)

**quantum-import.c 1.41 → 1.73, quantum-export.c 1.57 → 1.87, morphology.c 2.04 → 2.20,
distort.c 2.29 → 2.49, configure.c 6.01 → 6.19, transform.c 2.65 → 2.71, shear.c 2.18 →
2.72**, one Recipe B per commit. Most remove *Brain Method* with *Excess Number of
Function Arguments*; configure.c, transform.c and shear.c lose their last argument
finding.

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
| ImportCbYCrYQuantum | `QuantumPixelSpan` | oracle, ready (100%, uncapped `n100`) | 1.63 → 1.73 |
| configure.c AddConfigureKey | cache, path (`ConfigureKeyTarget`, new; the literal `"[built-in]"` at all five calls) | oracle, ready (2/2, `n100`), 9 cases | 6.01 → 6.19, the file's last argument finding |
| transform.c CopyImageRegion | columns, rows, sx, sy, dx, dy (`CopyRegion`, new; one per call, amendment below) | oracle, ready (19/19, `n100`), 26 cases | 2.65 → 2.71, the file's last argument finding |
| shear.c XShearImage | width, height, x_offset, y_offset (`ShearRegion`, new; one per call) | oracle, careful (88%, `n100`), 43 cases | 2.18 → 2.45 |
| shear.c YShearImage | the same struct | oracle, careful (78%), 43 cases | 2.45 → 2.67 |
| shear.c CropToFitImage | x_shear, y_shear, width, height, rotate (`CropToFitRegion`, new) | oracle, careful (89%), 43 cases | 2.67 → 2.72, the file's last argument finding |

**Recipe B amendment: member accesses and casts.** Step 4 refused any initialiser that
reads a member (`shear.x`, `image->rows`) or casts, which left shear.c and transform.c
out of reach though every one of their argument findings is on a static function. Such
an initialiser is now allowed in a struct declared directly before the one call that
uses it, with no statement between: nothing can run between the two, so the struct holds
exactly what the call would have read, and the expressions have no side effect, so
reading them first changes nothing. The calls that need it each get their own struct
(`first_pass`, `second_pass`, `third_pass`, `crop_region`, `first_region` to
`fourth_region`).

**New cases: ShearRotateImage.** No case reached ShearRotateImage, where three of the
changed calls are: `-rotate` is RotateImage, a distortion, and nothing in the library
calls it. A new `imdriver` command (`shearrotate IMAGE DEGREES OUT`) and sixteen cases
(eight angles across the quadrants, 45 and 180 degrees, negative angles; rose and
rose_alpha) now reach it. Commit `e3c0292c2`, before the shear steps; the case map was
rebuilt (15,900 cases).

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

### Open question for the owner (carried over from #54)

pixel.c stops at 1.73: each of the five remaining fixed-map Recipe E extractions,
taken alone, dips the file to 1.71 (*Lines of Code in a Single File* threshold), and the
pre-commit hook refuses it; all five together reach 1.80. May they go in as one
commit? See [M100](M100-pixel-buffer.md#where-pixelc-stops-at-173).
