# M131 - campaign B, night of 2026-11-14, second batch

Stacked on M130 (#110). This branch also merges origin/main after #108. Scores come from
cs-mcp 1.5.16.

Campaign B: **mean 8.52 over 91 files -> 8.77 over 94 files; files at 10.0: 27 -> 36**.

## What is in it
- **Lane b, round 6:**
  - Five files to 10.0: compress-lzw, coder-list, token-isglob, quantum, option-image.
  - Other gains: magick.c 8.29 -> 9.61, exception.c 8.28 -> 9.68, random.c 8.06 -> 9.45,
    coder.c 8.54 -> 9.38, memory.c 7.67 -> 8.42, quantum-type.c 8.85 -> 9.58,
    cache-view.c 7.78 -> 8.28.
  - New Recipes BW (reword a banner's repeated parameter text, under the owner's ruling of
    2026-11-13) and TP. Amendments to H, LP, PL and KC.
- **Lane b, round 7:**
  - Two files to 10.0: color-tuple, quantum-type.
  - Other gains: option.c 8.81 -> 9.38, string.c 8.54 -> 9.09, pixel.c 8.34 -> 8.88,
    locale.c 8.31 -> 8.85, color.c 8.21 -> 8.74, compress.c 7.21 -> 7.70.
  - New Recipe PT.
- **Lane a, round 5:**
  - list.c 8.03 -> 10.0, and image.c 6.55 -> 10.0 through three splits: image-mask.c 8.81,
    image-fill.c 7.85, image-clone.c 9.09.
  - Other gains: stream-read.c 9.02 -> 9.68, type.c 7.42 -> 8.22, policy.c 7.62 -> 8.18,
    log.c 7.23 -> 7.93, image-append.c 7.65 -> 8.54.
  - New Recipe SM.

## Gate on the merged tip (baseline c980213164, main after #108)
- Whole catalogue: 15,961 cases, 3 diverged, all known (info/d333f9b818, gaps/2857e5295a,
  gaps/c5651ffad8).
- `cxx_warnings --all`: OK on 190 files. `ci_warnings`: OK on the 72 changed files.
- The no-delegate clang build: 0 warnings.
- `check_trailers` valid, `playbook_lint` 0 problems.

## For the owner
- **OpenMP.** image-append.c 1220b9009 moves AppendImages' per-image loop, with its inner
  `omp parallel for` unchanged, into AppendImageList. The main session read it: every variable the
  region shares is declared in the helper outside the region, as before in the caller, and `y` is
  the private loop index. The run was single-threaded only.
- **Batched oracle runs.**
  - Lane b verified two batches (string/option/locale, color/pixel/compress) with one
    whole-catalogue run each, over a build holding the batch's steps. Every step still had
    ci_warnings, cxx_warnings, refactor_guard and `--calls`.
  - Lane a had several shared runs.
  - Each message says so.
- **Public API documentation reworded by BW:** memory.c `SetMagickMemoryMethods`.
- **Guard-level steps (no case reaches them):** magick.c MagickSignalHandler, random.c's four E
  steps (getentropy returns first on this Mac), option.c GetCommandOptions, two pixel.c
  LogPixelChannels steps, and the include element moved by SM in LoadPolicyCache and LoadTypeCache.
- **Error and trace names:** PolicyMap allocation errors and trace logs in coder, quantum,
  option-image and policy now name helpers (rulings of 2026-11-12 and 2026-11-13).
- **Flat commits kept as chain steps,** each named in its message.
- **Holds no recipe may lift:** most near-10 files left are held by Excess Number of Function
  Arguments on public functions, by `#if` code this build does not compile, or by cross-type
  duplication.
