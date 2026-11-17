# M136 - 2026-11-14, before the break

Stacked on M135 (#115). Scores come from cs-mcp through `tools/ch.py`.

Campaign B: **mean 9.05 -> 9.11 over 97 files; files at 10.0: 42 (unchanged)**

## Files

| File | Before | After | Lane |
| --- | --- | --- | --- |
| property.c | 3.66 | **7.30** | c (about 50 commits; new Recipes PK, KA, KB) |
| image-fill.c | 8.12 | 9.68 | b (new Recipe RM) |
| utility.c | 7.53 | 7.84 | a |
| module.c | 6.48 | 6.63 | b |
| annotate.c | 5.81 | 5.83 | a |

Probed with no gain: token-glob.c, policy.c, deprecate.c, delegate-invoke.c, quantum-export.c,
pixel-export.c, pixel-import.c, stream.c, draw.c.

## Gate at the tip

- `tools/oracle/build.sh cand`, `tools/cxx_warnings.sh --all`: OK, 193 files.
- `tools/ci_warnings.sh` on the 5 changed files: OK.
- ci-clang-min build of libMagickCore: first run 10 unused-parameter warnings (Linux
  `-Wall -Wextra -Werror` would fail); fixed in 5d14f944e with `magick_unused(x)` signatures and
  `(void) encoding;` in annotate.c (object files IDENTICAL), then exit 0 and 0 warnings.
  `tools/cxx_warnings.sh` no longer hides parameters named in `magick_unreferenced` (52ffd71bb).
- Whole catalogue: 15,961 cases, 3 diverged, all known (info/d333f9b818, gaps/2857e5295a,
  gaps/c5651ffad8).

## For the owner

- **property.c `#if 0` groups** in SetImageProperty (8bim:, exif:, fx:, iptc:, pixel:, xmp:) hold
  13 of its cc 29. Your 2026-11-01 exception lets `#if 0` blocks move verbatim (geometry-parse.c
  used it); deleting them would need a new ruling.
- **LCMS ICC arm in GetMagickProperty** contains `LCMS_VERSION < 2000` lines this build does not
  compile; moving it into the handler table needs a ruling. GetMagickProperty went from cc 88 to 9.
- **module.c's three steps** were proven in a configure-only modules build (`build-oracle/cand-mod`,
  `--with-modules`) with obj_modulo MODULO-IDENTICAL; the playbook grants that kind of proof only to
  splits. Please confirm.
- **image-fill.c Recipe RM** passes row fetch/row method through function pointers inside the
  parallel loops (struct read-only, pragmas unchanged; two indirect calls per row).
- **draw.c** is held by findings spread over 66 of its 212 functions; stubbing the 58 shortest
  flagged ones moves it only to 3.55. It needs a Recipe S split.
- **annotate.c** sits at the function-count threshold; a caption-family split
  (annotate-caption.c, trial 9.38) is the next step.
- **pixel-export/import.c** splits need their static per-type families widened to MagickPrivate
  (open question); stream.c's halves would both need `struct _StreamInfo`.
- Weak reach: property.c's compose/maxima/minima/output/unique handlers have no case;
  TracePSClippath 5 cases, TraceSVGClippath 12.
- Leftover directory `build-oracle/cand-modules` (failed configure in lane b's worktree) can be
  deleted by hand.
