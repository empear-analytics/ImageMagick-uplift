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

| annotate.c RenderType | image, draw_info (`RenderTarget`, new) | oracle, careful (80%, `n101`), 218 cases | 2.33 → 2.53 |

| annotate.c RenderFreetype | image, draw_info, encoding (`FreetypeTarget`, new; one per call) | oracle, careful (79%, `n101`), 198 cases; the no-FreeType stub compiled separately | 2.53 → 2.77 |

### The owner's decision: no mutation run before a step (evening)

The owner stopped the per-step mutation runs: the harness was built under mutation
guidance and is finished, and the 3SX and Jellyfin campaigns did not gate steps this
way. *Two levels of evidence* now reads: a function some case executes
(`casemap.py`) is oracle-verified for every recipe; the guard level stays for code no
case reaches. AGENTS.md and CLAUDE.md say the same (`5a8c61007`). Steps from here on cite
case reach and the oracle run.

| File | Function | Recipe | Evidence | Code Health |
|---|---|---|---|---|
| policy.c | LoadPolicyCache (`PolicyLoadTarget`) | B | 15,903 cases; zero-configuration branch compiled separately | 3.85 → 4.39 |
| color.c | LoadColorCache (`ColorLoadTarget`) | B | 1,391 cases | 3.16 → 3.49 |
| matrix.c | GaussJordanElimination's macros | H | IDENTICAL preprocessed text | 6.76 → 6.84 |
| matrix.c | UnscrambleColumns, SwapMatrixRows, SwapVectorEntries, EliminateColumn | E (4 commits) | 202 cases | 6.84 → 7.17 |
| matrix.c | FindPivot (sets row, column, max) | O | 202 cases | 7.17 → 7.27 |
| matrix.c | GetMatrixRange, GetMatrixScale (MatrixToImage) | O (2 commits) | 22 cases | 7.27 → 7.45 |
| color.c | SetDeviceColorspace, SetICCColorspace (QueryColorFunction) | O (2 commits) | 143 cases, device- and icc-color among them | 3.49 → 3.55 |
| constitute.c | SetInterpretedProperty (ReadImage's caption/comment/label) | D | 14,969 cases | 4.45 → 4.48 |
| constitute.c | EncodeImageWithInfo (WriteImage's two encode blocks) | D | 13,088 cases | 4.48 → 4.52 |
| constitute.c | FindFallbackEncoder, SyncEndianFromMagickInfo (WriteImage) | O, E | 13,088 cases, 251 on the fallback path | 4.52 → 4.65 |
| fx.c | ExecuteRPN's six HSL-qualifier tests → existing IsHslChannelQualifier | P (several sites) | 767 cases | 1.93 → 2.15 |
| fx.c | TranslateExpression's two user-symbol tests → IsUnassignedUserSymbolRead | P (several sites) | 821 cases | 2.15 → 2.28, no Complex Conditional left |

Most of these were left by earlier tasks (M17, M21, M38, M48) only because the
recipe needed the oracle level and the function's mutation score was below the gate.

**New cases: SetMagickSecurityPolicy.** No case reached it (only the API calls it); a new
`imdriver` command, `secpolicy XML [NAME...]`, and six cases (a coder rule, resource
values, both, malformed XML, includes of a provided and a missing file) now do
(`56a0ad2ab`). The case
map was rebuilt (15,906 cases).

**Not done.** MorphologyApply: its method switch sets five variables and its per-stage
switch four (Edge also swaps `curr_image` and `save_image`), so Recipe O helpers would
take seven to nine arguments. distort.c's best-fit bounds: each case writes `min` and
`max` through macros defined mid-function that name the caller's locals. property.c and
colorspace.c tables: Recipe F left the score flat (above).

### Open: an intermittent divergence on write-mask cases

`resize/6ee45e9cfa` and `resize/6efddf0c0a` (rose under a write mask, `-scale`, written
as 32-bit floating-point MIFF) diverged three times today in full oracle runs: during the
ImportIndexQuantum and ImportGrayQuantum steps and once at 21:08, when the case diverged
in 4 of 5 runs on its own, with a peak error of 3.4e38 (FLT_MAX, a value no pixel had).
Minutes later it diverged in none of: 16 runs against the baseline, 16 of the candidate
against itself and 16 of the baseline against itself under a load average of 27, nor in
full uncached runs of all 13,088 WriteImage cases (baseline against itself, and the
candidate against the baseline). The baseline never diverged in any run. The pattern
fits a read of uninitialised memory whose content depends on the run's surroundings, but
it could not be reproduced to bisect. If it reappears, keep the candidate's output
(`build-oracle/work/runs/cand/`) and bisect quantum-import.c and quantum-export.c first:
the case reads and writes MIFF through both, and both changed today.

### ERDC (night)

The fresh reports after profile.c are unusable: transform, quantize, montage and cache
show 0 survivors with most kills "exit status 0 → 1, out.miff missing", and constitute
is partly affected; the earlier files look normal. The run was stopped. Those five
reports are quarantined on the Mac (`build-oracle/work/quarantine/`); nothing used them.
With the owner's decision above, night3 is not restarted.

**Recipe F - Hoist Function-Local Static Data (new).** A `static` local whose
initialiser is data and which nothing writes moves to file scope above its function.
Proof: `tools/obj_same_code.sh` (new), which compares the machine code and the data as
sets, since the compiler reorders the string pool when a literal moves (byte-identical
objects, Recipe L's proof, fail on that alone). It printed SAME CODE for the steps below
and DIFFERENT for a trial with one delegate name changed.

| File | Function | Data moved | Evidence | Code Health |
|---|---|---|---|---|
| version.c | GetMagickDelegates | the `#if`-assembled delegates string (124 lines) | oracle (9 cases), SAME CODE | 9.28 → **10.00** |
| signature.c | TransformSignature | the 64 SHA-256 round constants | oracle (1,867 cases), SAME CODE | 8.54 → 8.62 |

Tried and reverted (score flat): property.c GetEXIFProperty's tag table (304 lines; the
function stays Large at about 400 lines; the tool also flagged 4 bytes of alignment
padding) and colorspace.c TransformsRGBImage's `YCCMap` (236 lines, the function stays
at about 700). F pays only where the data is most of the function.

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
