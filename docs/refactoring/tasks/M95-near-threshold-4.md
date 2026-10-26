# M95 - near-threshold functions, fourth round (wider criteria)

Not a generated task file. A rescan of MagickCore with wider criteria: cc ≤ 14 or ≤ 3
bumps, at most two findings, X11 and OpenCL files skipped. Branch `campaign/M95`, on
`campaign/M94`. Measured with `cs-mcp` 1.5.12; oracle baseline `ef0f98a05f` (`main` after
#43).

### Report (2026-10-26, afternoon)

6 refactor commits, one Recipe E each.

| File | Function | Effect | Evidence | Code Health |
|---|---|---|---|---|
| image.c | ResetImagePage (2 steps) | Complex Method (cc 13) and Bumpy Road (4 bumps) gone; one Bumpy Road (2) remains, in the helper `SetAbsolutePageOffset` | oracle, ready, 319 cases | flat at 2.20 |
| xml-tree.c | DestroyXMLTreeRoot (2 steps) | no finding left (cc 15 → 5, 3 bumps gone); helper `DestroyXMLRootAttributes` keeps 2 bumps | guard (no usable rows); oracle 142 cases | 2.18 → 2.24 |
| xml-tree.c | GetXMLTreeAttribute | no finding left (cc 14; E step 7) | guard; oracle 107 cases | 2.24 → 2.29 |
| property.c | GetIPTCProperty | Bumpy Road gone, cc 14 → 12 | oracle, ready, 14,911 cases | 2.02 → 2.07 |

**Reverted.**
- xml-tree.c InsertTagIntoXMLTree, step `InsertTagInOrder`. The helper would have been
  the file's first function (line 136 is the static `sentinel` array, not a function).
- CodeScene then counted the declarations above it into the helper and reported Code
  Duplication against nine other functions.
- This is the first-function quirk again: check `grep -n '^static\|^Magick'` for the
  first *function* before placing a helper at the top of a file.

**Not taken.** DestroyImageInfo, AutoOrientImage, RelinquishPixelCachePixels,
SyncAuthenticPixelCacheNexus, GetMedianPixel, WriteTo8BimProfile, CopyDelegateFile:
- a flat run whose helper would carry the same complexity;
- a single table-like `switch`;
- an uncompiled OpenCL branch;
- a return or throw in mid-function;
- loops that write two live cursors.

**An oracle note.** The `-hough-lines` matrix-on-disk cases diverged in one run.
- Both binaries give identical output when run directly, and the cases pass with
  `--no-cache`.
- The baseline cache for the new `main` (`ef0f98a05f`) had been recorded while a
  mutation run was loading the Mac.
- The branch's whole-catalogue run therefore uses `--no-cache`.

### Addendum: fresh mutation (2026-10-26, 15:57)

xml-tree.c was mutated again on the Mac. The Mull build was `mull-guard95` and the
report is `mutation-guard95-all.json`; the run used `--max-cases 300`.

- GetXMLTreeAttribute: 5 of 5 killed, **ready**. That step now stands at oracle level.
- DestroyXMLTreeRoot: 5 of 20 killed, 12 unobservable, 3 unmatched, **not ready**.
  - The function only frees memory, so most mutants change nothing the oracle can see.
  - Its two steps stay guard-level, as committed.
  - A leak checker in the oracle would be needed to see this code.
