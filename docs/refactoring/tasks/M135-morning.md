# M135 - morning of 2026-11-14

Stacked on M134 (#114). Scores come from cs-mcp through `tools/ch.py`.

Campaign B: **mean 9.00 over 96 files -> 9.05 over 97 files; files at 10.0: 41 -> 42**
(the new file is cache-access.c, split from cache.c).

## Files

| File | Before | After | Lane |
| --- | --- | --- | --- |
| compress.c | 8.06 | **10.0** | b (new Recipe WS) |
| geometry-parse.c | 7.75 | 8.81 | b (`#if 0` blocks moved verbatim, 2026-11-01 exception) |
| quantum-import.c | 7.29 | 8.28 | b |
| image-view.c | 6.24 | 6.84 | b (guard-level, Recipe W unreached form) |
| profile.c | 7.29 | 7.41 | b |
| pixel.c | 8.88 | 9.02 | b |
| cache.c | 2.90 | 3.52 | a (split + D/W/E/MT) |
| cache-access.c (new) | - | 8.54 | a (Recipe S split from cache.c) |

No change: memory.c 8.42 (findings in `#if` platform branches), cache-view.c 8.28,
locale.c 8.91, blob.c 2.71 (needs about 12 big functions cleaned before the score moves).

## Gate at the tip

- `tools/oracle/build.sh cand`, `tools/cxx_warnings.sh --all`: OK, 193 files.
- `tools/ci_warnings.sh` on the 8 changed files: OK.
- ci-clang-min build of libMagickCore: exit 0, 0 warnings.
- Whole catalogue: 15,961 cases, 3 diverged, all known (info/d333f9b818, gaps/2857e5295a,
  gaps/c5651ffad8).

## For the owner

- **Recipe WS** (writer/reader struct for the Huffman coder's `OutputBit`/`InputBit` locals):
  please confirm it is not the "context struct" extraction refused on 2026-10-26. compress.c's
  10.0 depends on it.
- **Recipe W unreached form** (751e50482): guard-level W on code no case reaches, proven by an
  identical object file. Awaits confirmation; image-view.c's steps rest on it.
- **cache.c split**: moves the accessor duplication into cache-access.c (you asked to hear about
  such splits). Trace logs in the new mask/cache helpers name the helper (ruling of 2026-11-13);
  `CacheEvent` debug logs moved into `LogMemoryPixelCacheOpen` (2026-10-08 decision).
- **resize/6ee45e9cfa** (write mask + scale, on the load-sensitive list) diverged in lane a's
  tip run and runs through the rewritten clip-mask path. It also diverged on M132 before that
  change, every per-step run was clean, the merged helper reads equal to both originals, and the
  M135 gate run did not diverge on it. Treated as pre-existing.
- Harness gap: an `imageview` op in the oracle driver would let image-view.c's parallel loops be
  refactored under oracle evidence.
- Weak reach: compress.c's decoder is reached by 5 cases, the encoder by 8.
