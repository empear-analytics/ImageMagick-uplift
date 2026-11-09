# M119 - second passes on draw, cache, blob, image, annotate and profile

Not a generated task file. Branch `campaign/M119`, from `origin/main` (67e25ec09, after
#84 and #90). Campaign B on the Mac, 2026-11-09. Measured with `cs-mcp` 1.5.16, the same
version as M115-M118 (a fresh sweep of MagickCore at the branch tip).

One subagent per file (Sonnet 5.5) under the session brief, one after another on the same
build. The session reviewed each report; it did not re-read every step.

### Report

| File | Code Health | Commits |
| --- | --- | --- |
| `draw.c` (second pass) | 2.51 → 2.62 | 11 |
| `cache.c` (second pass) | 2.67 → 2.67 | 0 |
| `blob.c` (second pass) | 2.71 → 2.71 | 2 |
| `image.c` (second pass) | 2.98 → **4.20** | 23 |
| `annotate.c` (second pass) | 3.35 → 3.93 | 23 |
| `profile.c` (second pass) | 3.58 → 3.58 | 1 |

60 refactor commits, no playbook change. `check_trailers.py`: 60 commits, 0 without valid
trailers. Every commit passed the build, `ci_warnings.sh`, the literal guard and the
pre-commit gate, and the oracle on the cases reaching its function (a divergence under
load was rerun alone with `--no-cache`).

The whole catalogue at the branch tip: 15,906 cases, 3 diverged: the known
`gaps/c5651ffad8` (logcfg), and `info/d333f9b818` and `gaps/2857e5295a`, both
`magick -list configure`. The last two are environmental: run by hand, the base build of
`origin/main` and the candidate print byte-identical output, and the two `image.c` and
`annotate.c` agents saw them diverge on builds with and without their changes.

`image.c` left Red (4.20). `draw.c`, `cache.c`, `blob.c`, `annotate.c` and `profile.c` stay
below 4.00.

### Commit messages with slips (not amended)

- `draw.c` c54c0b840 (Recipe L) says `Verified: guard`, but the oracle ran on 47 cases.
- `image.c` f43ec20d6 says cc fell 17 → 13 "after the previous commit and this one"; 13 was
  measured only after both. 98add620b blames `MAGICK_TEMPORARY_PATH` for two
  `-list configure` divergences without verifying the cause (they are not its own).
  fa4ea2210, f43ec20d6 and 95510ea66 are flat and kept under rule 2.
- `annotate.c` flat steps kept under rule 2: 3c7afd7f6, 416fdecb3, a366f5cd3, e350a606b,
  890b24248, 0b3b2b3ec.

### For the owner

1. **Recipe Q step 8 with a const struct of addresses** (`draw.c` `StrokeTargets`, in
   `TraceStrokePolygon`'s miter blocks, which also read arrays through an AR struct). Read as
   within Q; confirm, or add a sentence.
2. **A recipe for a switch case with several outputs** (`cache.c`: the Read/Write
   PixelCachePixels and Metacontent switches write `y`, `rows`, `length`, `q`). It would open
   four functions that hold much of the file's cc.
3. **Moving a static helper up, or a static forward declaration** (`blob.c` `BlobToImage`
   reusing `RestoreCustomStreamFilenames`; `image.c` `AcquireImage`, M116 question 6).
4. **Recipe N reading in `annotate.c` cff92e556** (a status that goes in by value and is
   returned fits N step 1 with E steps 3 and 4).
5. **A `(void)` cast for a dead store in an extract** (`annotate.c` a3ce41e9b, 890b24248): a
   set-but-unused local fails `-Wunused-but-set-variable`; E step 4 would declare it.
6. **Hoisting `ThrowFreetypeErrorException` under Recipe H** (`annotate.c`), and **Recipe E
   over compiled `#if` lines** (`FT_ENCODING_*`).
7. **Recipe D on `'8'` and `0x38` as one value** (`profile.c` `Update8BIMClipPath` and
   `Sync8BimProfile`; M117 question 7): a probe went 3.58 → 3.54 on Code Duplication.
8. **Recipe W with a targets struct** (`draw.c` `ConvertPathToPolygon`; probe +0.06, an
   extraction about +0.04), and the existing `TracePath` exit (`ThrowPointExpectedException`).
9. **One convention for where a helper sits relative to its doc comment** (`image.c` is
   inconsistent: early mask helpers between comment and function, later ones above).
10. **Recipe D variant for operator-differing helpers** (`image.c` mask-channel helpers now
    show in Code Duplication).
11. **Recipe N structs for five-input helpers** (`blob.c`), which the `ImageToBlob` probe
    showed are twin-prone (file fell to 2.62).

### Where the files plateau

- `draw.c` 2.62: Deep Nesting stays in `RenderMVGContent` (6), `TracePath` (5) and
  `ConvertPathToPolygon` (5), all behind question 8 or the earlier `TracePath` question.
  `TraceStrokePolygon` keeps cc 49. Reducing the six static functions to four parameters
  left the score at 2.62.
- `cache.c` 2.67: 22 Complex Method, 10 Large Method, 15 Bumpy Road, 18 Excess Arguments
  and 119 functions, spread over 20-odd functions each far over its threshold. One
  `GetVirtualPixelCacheNexus` extraction (E, helper cc 11) added a Complex Method and left
  the score at 2.67, and was reverted. `GetVirtualPixelRows` is reached by no case. Whole-
  catalogue-reach steps cost about 450 s each.
- `blob.c` 2.71: 124 functions, 28 Complex Method, 17 Bumpy Road; about ten near-twin
  typed wrappers or stream-type switches whose cases are one-liners or under uncompiled
  `#if`. `SetBlobExtent` keeps `MAGICKCORE_HAVE_POSIX_FALLOCATE` code the build does not
  compile. Probes reverted: `ImageToBlob` (2.62), `SeekBlob` (flat, new Complex Method).
- `image.c` 4.20: `AcquireImage` (cc 23, 223 lines; question 3); `SetImageInfo` (cc 27;
  14,972 cases, ~450 s a step, not tried); `CloneImage` (a page-scale extraction dipped to
  4.13 and was reverted); `NewMagickImage` and `SetImageColor` reach no case; `SmushImages`
  and `AppendImages` need more than four parameters for their loop heads.
- `annotate.c` 3.93: `RenderPostscript`, `RenderX11` and the Raqm functions are in code
  this build does not compile. `RenderFreetype` (cc 51, 322 lines) and `AnnotateImage`
  (cc 28) have blocks needing more than four inputs or outputs, `#if` lines, or the
  `ThrowFreetypeErrorException` macro. `RasterizeGlyphRow`'s x-loop writes `q` for the
  next iteration. Probes reverted: a second encoding helper (3.30), a 7-member struct for
  the gravity switch (3.73), the undercolor box (flat).
- `profile.c` 3.58: `ProfileImage` (Brain Method) has its colour switches stubbed in this
  Q16 HDRI build, so only guard-level recipes apply; `SyncExifProfile` cc 57 after 13
  commits; `UpdateClipPath` needs a context struct. The one kept step, in
  `GetXMPNumeratorAndDenominator` (232d833b4), removed a Complex Method and left the score
  flat.

### OpenMP

Two `annotate.c` steps touch `RenderFreetype`'s parallel row loop; both were read by the
session against the playbook, not taken from the agent's report.

- a5aa6b678 (Recipe PS): the loop body moved to `RasterizeGlyphRow`; the pragma, its
  `shared(status)` clause and the `for` statement stay in the caller. `status` is the only
  variable declared outside the body that the block writes; it goes by pointer, every use
  `(*status)`, as PS step 4 says. The `GlyphRows` struct is filled in the body, so each
  thread has its own.
- cff92e556 (Recipe N, 2026-11-05 amendment): the render branch lies outside every OpenMP
  region and holds the whole parallel loop, `#if` line, pragma, `#endif` and `for`
  statement, which moved unchanged; `status` and the counter `y` are a parameter and a
  local of the new function, and `shared(status)` names the parameter.
