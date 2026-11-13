# M128 - campaign B, lanes round two and three, splits (2026-11-13)

This round continues M127's three parallel lanes (see `M127-parallel-lanes.md` for the set-up).
It is stacked on M127 (#106). Scores come from cs-mcp 1.5.16.

Campaign B: **mean 7.36 over 66 files -> 8.01 over 85 files** (19 splits). Files at 10.0: 3 -> 8:
client, geometry, locale-format, monitor, pixel-gamma, string-info, thread and version.

## Lane b, rounds 2 and 3
- draw.c 2.62 -> 2.68 and cache.c 2.67 -> 2.84. Both are held by throw exits and the
  function-count threshold.
- **geometry.c 7.40 -> 10.0**, campaign B's first file at 10 by refactoring.
- resource.c 7.04 -> 8.97, splay-tree.c 7.49 -> 8.81, matrix.c 7.45 -> 7.84 (8.27 after
  lane a's split), geometry-parse.c 7.04 -> 7.75, cipher.c 7.28 -> 7.72, list.c 7.09 -> 7.55.
- New Recipes UR and KP. Amendments to I, LX, TB, W and RT.

## Lane c, round 3
- property.c 2.53 -> 2.76 (18 commits). The SyncExifProfile chain from profile.c, applied to
  GetEXIFProperty, which no longer has any finding.
- What remains is in GetMagickProperty, InterpretImageProperties and SetImageProperty
  (cc 94-124, exported).

## Lane a, round 2: Recipe S splits
19 splits, one per commit, all under the owner's 2026-10-28 approval. For each one: a new cand
build; ci_warnings and cxx_warnings on both files; `refactor_guard --combined [--calls]`; nm showing
only the moved publics; and the oracle on the moved roots.

| Original | Before -> after | New file |
| --- | --- | --- |
| string.c | 6.79 -> 8.47 | string-info.c 10.0, string-copy.c 9.61 |
| delegate.c | 6.44 -> 7.96 | delegate-invoke.c 7.11 |
| quantum.c | 8.32 -> 9.38 | quantum-type.c 8.85 |
| option.c | 7.78 -> 8.81 | option-image.c 8.81 |
| pixel.c | 7.27 -> 8.28 | pixel-gamma.c 10.0, pixel-fuzzy.c 9.61 |
| matrix.c | -> 8.27 | matrix-gauss.c 8.17 |
| token.c | 7.82 -> 8.58 | token-isglob.c 9.68 |
| memory.c | 6.99 -> 7.67 | memory-quantum.c 9.36 |
| color.c | 7.86 -> 8.15 | color-tuple.c 9.04 |
| constitute.c | 6.39 -> 6.71 | constitute-pixels.c 9.20 |
| coder.c | 8.28 -> 8.54 | coder-list.c 9.68 |
| exception.c | 8.03 -> 8.28 | exception-throw.c 9.68 |
| xml-tree.c | 4.97 -> 5.20 | xml-tree-file.c 9.42 |
| compress.c | 6.99 -> 7.21 | compress-lzw.c 9.38 |
| locale.c | 6.88 -> 7.09 | locale-format.c 10.0 |
| annotate.c | 4.50 -> 4.67 | annotate-metrics.c 9.57 |
| image.c | 5.55 -> 5.71 | image-settings.c 9.09 |

Build lists changed: `MagickCore/Makefile.am`, `Makefile.in`, `MagickCore/Make.com`,
`scripts/format_c_api_docs`. Three more splits (geometry.c twice, list.c once) were reverted
before the merge (93ee18360). Lane b had changed both files in the meantime, and the merge
conflicted. geometry.c no longer needs a split, and list.c's split is being redone on the new
version.

blob.c stays at 2.71. A probe that empties all 28 Complex Method functions reaches only 7.05,
and the typed ReadBlob* split pays 0.06 and is refused by the tool (the define block sits inside
the include block).

## Gate on the merged tip
- Whole catalogue: 15,961 cases, 5 diverged. Three are the known ones (info/d333f9b818,
  gaps/2857e5295a, gaps/c5651ffad8). The other two are the hough matrix-on-disk baseline load
  timeouts described in M127.
- `cxx_warnings --all`: OK on 181 files. `ci_warnings`: OK on the 56 changed files.
- The no-delegate clang build (the CI container's configuration) compiles MagickCore with no
  warning.
- `check_trailers`: 195 refactor commits, all valid. `playbook_lint`: 0 problems.

## For the owner
- **Splits that cannot be checked on this Mac:**
  - image.c's OpenMP families need `split_branch_check` in a gcc `-fopenmp` build, which only
    WSL has.
  - utility.c, policy.c, magick.c and cipher.c need their Windows and modules `#if` branches
    compiled.
- **Splits that pay because a clone pair leaves the file:** string-copy, pixel-gamma,
  locale-format, token-isglob. These may read as moving duplication out of the detector's view.
- **Split tool bug:** `split_source.py` produced a non-compiling stream.c split, because it does
  not move multi-line forward declarations inside `extern "C"`. Lane a may fix the tool.
- **`ch.py` scores depend on the path:** the same stream.c scored 7.05 in a scratch directory
  and 6.64 at its real path. Every number above was measured at the real path.
- **Commit messages that lack detail:**
  - Some of lane b's messages say only that divergences were "known/settled", without naming
    the case. Its rerun logs were overwritten.
  - Lane c first verified groups of commits with one oracle run, then ran every intermediate
    tree. Those later runs are not in the messages.
- **cache.c:** the three new `Create*PixelCache` helpers take `CacheInfo` by value (E step 4)
  and carry `LogMagickEvent` blocks. Lane b relied on the 2026-10-08 ruling that trace logging
  is outside the oracle's contract.
- **Weak reach:**
  - Reached by no case: the distributed pixel cache branch (no case sets `cache:hosts`), and the
    moved functions SetStringInfoPath, StringInfoToDigest, RemoveImageOption, ResetImageOptions
    and CopyMagickMemory.
  - Reached by 1 to 7 cases: ConstituteImage 1, ListCoderInfo 1, cipher.c 3 and 6,
    ParseAffineGeometry 5, FormatTimeToLive 7.
