# M127 - campaign B, parallel lanes (2026-11-13)

Campaign B (Mac) ran like campaign A since W14 for the first time: three lanes, each a git worktree
`~/im-m127{a,b,c}` on `campaign/M127-{a,b,c}`. Each lane had its own candidate build, its own oracle
baseline at `origin/main` (e5affc3c5), a copy of the case map and `ORACLE_JOBS=3`. Each lane was one
Claude Opus 5.5 subagent under the same brief. The main session checked every lane tip (trailers,
`ci_warnings`, the new `cxx_warnings`, scores), merged it into `campaign/M127`, and fast-forwarded
the lane before its next round. Stacked on M126 (#105, the C++/MSVC build fixes). Scores come from
cs-mcp 1.5.16.

Campaign B: 66 files, mean **7.23 -> 7.36**; 109 refactor commits.

| File | Score | Lane |
| --- | --- | --- |
| `color.c` | 6.13 -> 7.86 | b |
| `profile.c` | 3.58 -> 4.76 (out of Red) | c, round 2 |
| `constitute.c` | 5.60 -> 6.39 | a |
| `type.c` | 6.23 -> 6.90 | b |
| `locale.c` | 6.28 -> 6.88 | c |
| `stream.c` | 6.15 -> 6.64 | b |
| `compress.c` | 6.58 -> 6.99 | c |
| `xml-tree.c` | 4.56 -> 4.97 | a |
| `utility.c` | 6.04 -> 6.43 | b |
| `policy.c` | 6.38 -> 6.70 | c |
| `log.c` | 5.89 -> 6.21 | a |
| `annotate.c` | 4.20 -> 4.50 | a |
| `delegate.c` | 6.27 -> 6.44 | c |
| `string.c` | 6.64 -> 6.79 | c |
| `memory.c` | 6.87 -> 6.99 | c |
| `image.c` | 5.53 -> 5.55 | a |

These were tried and stayed where they were:
- `image-view.c`: no case reaches it, and its iterators are OpenMP regions.
- `module.c`: the oracle build does not compile it.
- `property.c`: the main lever is GetEXIFProperty (stubbed alone, the file scores 2.74). Lane c's
  third round is applying the SyncExifProfile chain to it.

**New recipes** (each marked with its lane): TC (drop a trailing conditional `continue`, lane c),
KF (an enumerator-keyed call switch becomes a table of function pointers, b), SP (split a
predicate's disjunction at a leading run, b), BC (extract a loop-body tail that returns whether it
broke, c), LW (merge twin list walks that differ in the link member, a). **Amendments:** W (up to
three caller locals by pointer), PA (any integer selector, a single-call group body), GD (the exit
may be `break;` or `continue;`), all from lane c. The lanes inserted recipes at the same place, so
each merge was resolved by keeping both sides whole (`playbook_lint`: 105 recipes, 0 problems).

**Gate on the merged tip:**
- Whole catalogue: 15,961 cases, 5 diverged. Three are the known ones: info/d333f9b818,
  gaps/2857e5295a and gaps/c5651ffad8.
- The other two, matrix/f2913328b4 and matrix/e1095950b7 (`-hough-lines` with `-limit memory 0
  -limit map 0`), time out on the **baseline** under the load of three lanes, and they did so
  again when rerun alone. Run by hand, both binaries finish (base 15.6 s, candidate 18.0 s) and
  write byte-identical `out.miff`. These are recorded as load timeouts.
- `tools/cxx_warnings.sh --all`: OK on 162 files.
- `check_trailers`: 109 refactor commits, all valid.

**For the owner:**
- Batched steps:
  - Lane b's DT commits each cover about ten predicates of one file in one commit
    (`obj_identical`).
  - stream.c KF, utility.c W and type.c NG were verified by one shared oracle run (382 cases,
    0 diverged) and then committed one by one.
- Labels:
  - type.c `SetTypeInfoAttribute` is labelled SW, but it splits an if/else-if chain.
  - constitute.c 654335738 is labelled E.
  - 0ad8d3037 also drops a `status` store that was never read.
  - annotate.c 03b12f5e4 and 526966ca1 are SD chains through an existing struct.
- Statements moved: annotate.c a05e5ec59 moves a struct fill earlier, past statements that write
  none of its members; 0c44f29bb copies `metrics` again after RenderType changes it.
- OpenMP-adjacent: annotate.c 95fd7ca64 extracts inside RasterizeGlyphRow, which runs per row of
  RenderFreetype's parallel loop. The helper writes only that iteration's `q` and `active`; read
  and confirmed by the main session.
- `#if` lines moved with code: memory.c `InitializeVirtualMemoryPolicy` took its compiled
  `#if defined(MAGICKCORE_HAVE_MMAP) && defined(MAP_ANONYMOUS)` line along, and profile.c
  589301054 (H) moves an `#ifndef TYPE_XYZ_8` block, which is compiled here.
- Weak reach:
  - Reached by no case: `FormatHexTextLine` (guard).
  - Single-digit reach: `PrintStringInfoHexLine` 2, `GetXMPContinuedFraction` 2,
    `SetColorInfoAttribute` 3 (its caller `LoadColorCache` 1,391 also ran), `HuffmanDecodeImage` 5,
    `UpdateClipPath` 5, `FormatLogFilename` 6.
  - `TranslateEvent` 21.
- Process lesson: the lanes shared the session scratchpad, and one lane's `verify.sh` overwrote
  another's, so one oracle run went to the wrong worktree. Lane c reran everything. The brief now
  keeps scratch files in `<worktree>/build-oracle/lane-tmp/`.
