# M130 - campaign B, night of 2026-11-13/14

Stacked on M129 (#109). Scores come from cs-mcp 1.5.16.

Campaign B: **mean 8.31 over 89 files -> 8.52 over 91 files; files at 10.0: 18 -> 27**.

## What is in it
- **Lane b, round 5:** nine files to 10.0: string-copy, pixel-fuzzy, annotate-metrics, xml-tree-file,
  image-settings, linked-list, timer, splay-tree and mime. locale.c 7.09 -> 8.31 and compress-lzw.c
  9.38 -> 9.92. New Recipes BF (doc-banner duplication, owner ruling of 2026-11-13), AT, TM, IC
  and OS.
- **Lane c, round 5:** draw.c 2.68 -> 2.99, property.c 2.76 -> 3.21, cache.c 2.84 -> 2.90. New
  Recipe CK (a switch's groups moved through a const struct of the caller's state). Commit 7dcfb2e37
  moves all ten groups of TracePath at once, because fewer groups cross the function-count threshold.
- **Lane a, round 4:** log.c 6.21 -> 7.23, image.c 5.71 -> 6.55 (new image-append.c 7.65),
  annotate.c 4.67 -> 5.22, profile.c 4.76 -> 5.15 (new profile-sync.c 8.54), utility.c 6.43 -> 6.61,
  xml-tree.c 5.20 -> 5.34. blob.c is unchanged: its private structs block splits, and every case
  helper is itself Complex.
- **Lane c, round 6:**

  | File | Before -> after |
  | --- | --- |
  | identify.c | 7.18 -> 9.38 |
  | configure.c | 9.02 -> 9.61 |
  | image-filename.c | 7.32 -> 8.81 |
  | cipher.c | 7.72 -> 8.69 |
  | delegate.c | 7.96 -> 8.54 |
  | delegate-invoke.c | 7.14 -> 7.64 |

- **CI fix (also on M128 and M129):** #108's CI stopped on utility.c in two places. gcc's
  -Wimplicit-fallthrough fired in Base64Decode's case 2 (now `magick_fallthrough;`). MSVC C4101 fired
  on AcquireUniqueSymbolicLink's `status`, used only under `MAGICKCORE_HAVE_SYMLINK` (the declaration
  moved into that block). `tools/cxx_warnings.sh` now reports gcc-style fall-through.

## Gate on the pushed tip (baseline 6870d49fbb)
- Whole catalogue: 15,961 cases, 3 diverged, all known (info/d333f9b818, gaps/2857e5295a,
  gaps/c5651ffad8).
- `cxx_warnings --all`: OK on 187 files. `ci_warnings`: OK on the 76 changed files.
- The no-delegate clang build: 0 warnings.
- `check_trailers`: every refactor commit valid. `playbook_lint`: 0 problems.

## For the owner
- **OpenMP:** profile.c 919940ea3 moves ProfileImage's whole `omp parallel for` loop, pragma and
  `shared(status)` included, verbatim into TransformProfileImage. The main session read it. The new
  helper's locals are read-only copies made before the region, and `status` is still one shared
  local that the helper returns. Two PS steps in profile.c and one in image.c touch parallel-loop
  bodies. The oracle runs single-threaded, and this Mac has no libomp.
- **Coverage gap:** an oracle case importing a gray, depth-10, unpacked image with LSB endianness
  would let quantum-import.c's SC step go 7.29 -> ~7.65. Reported here, as AGENTS.md asks, and not
  added.
- **Batched oracle runs:** several lane a and lane c commits were verified by one run over a build
  holding several steps; each message says so.
- **Error calls and trace openings moved into helpers** in identify, delegate-invoke, cipher, mime,
  string, locale and compress-lzw (rulings of 2026-11-12 and 2026-11-13).
- **Weak reach:**
  - Reached by no case: the TraceStrokePolygon failure block, `papersize:`, and the TracePath
    throw paths. The 50 TracePath cases never take a throw.
  - Reached by 1 to 6 cases: PasskeyDecipherImage 3, PasskeyEncipherImage 6,
    PrintConvexHullAndBoundingBox 1, GetImageMask 6.
- **Flat commits kept as parts of a chain:** image-settings OS steps 1-4, one annotate-metrics step,
  one locale step.
