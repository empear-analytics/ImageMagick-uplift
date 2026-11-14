# M134 - campaign B, 2026-11-14 midday

Stacked on M133 (#113). Scores come from cs-mcp 1.5.16.

Campaign B: **mean 8.96 over 95 files -> 9.00 over 96 files; files at 10.0: 40 -> 41**
(token-next.c).

## What is in it
- **Lane a, round 7:**
  - constitute-codec.c 7.59 -> 8.73, through a split into the new constitute-write.c (9.92), and
    matrix.c 8.72 -> 8.81.
  - The requested splits of profile-sync, identify, delegate-invoke and color were all refused:
    shared statics, a single public root, or an undeclared helper.
- **Lane b, round 11:**
  - **tools/obj_modulo.sh fix** (ea644a3f0, f45730d56). On the Mac it compared no functions:
    Apple's arm64 objdump puts spaces before the tab, so the line filter matched nothing. It still
    printed MODULO-IDENTICAL. It now:
    - errors when it compares zero functions;
    - takes function names from nm;
    - compares relocations, so a changed callee counts;
    - compares arm64 `adrp` page operands through their relocations.
  - **Audit:** no commit between origin/main and this branch cites obj_modulo. The citing commits
    (campaign A, made on WSL/ELF, where the tool worked) were rerun on the Mac where possible
    (transform, segment, paint), and all still pass. About 75 X11/OpenCL commits need the WIDE
    build to rerun.
  - **token-next.c 9.38 -> 10.0.** Tokenizer timing: median 1.313 s before and 1.320 s after
    (300 passes over a 0.93 MB MVG), within the run-to-run spread.
- **Lane c, round 9:**
  - pixel-export.c, pixel-import.c and stream.c 6.89 -> 7.32 each.
    - New Recipe RW: one row walker calls a per-type row function through a pointer, once per
      row; the per-pixel loops are unchanged.
    - New Recipe QF: a shared pixel-queue fetch.
    - Timing equal to base: within 0.6% (export) and 1.1% (import) on 2000x2000 images.
  - property.c 3.62 -> 3.66, with new Recipes JT and XP.

## Gate on the pushed tip (baseline c980213164)
- Whole catalogue: 15,961 cases, 3 diverged, all known.
- `cxx_warnings --all`: OK on 192 files. `ci_warnings`: OK on the 77 changed files.
- The no-delegate clang build: 0 warnings.

## For the owner
- **WIDE build:** rerun the ~75 X11/OpenCL commits that cite obj_modulo on it (campaign A's
  machine).
- **Coverage gaps:** reported, not added.
  - A case reading `name-%d.png[1-2]` through ReadImages would allow merging
    PingSceneImages/ReadSceneImages. The probe takes constitute.c to 10.0.
  - A gray, depth-10, LSB import would allow quantum-import.c's SC step, to about 7.65.
- **Hand checks:** property.c's non-glob `%[artifact:x]` path has no oracle case; it was checked by
  hand against base.
- **OpenMP:** matrix.c 74aaa5a66 moves MatrixToImage's column loop inside its OpenMP loop body.
  It was checked by reading; this Mac has no libomp.
- **Open questions:**
  - Static -> MagickPrivate for splits.
  - Whether Windows `#if` branches may move in a split.
  - The AnnotateEvent log in a helper.
  - geometry-meta's struct.
