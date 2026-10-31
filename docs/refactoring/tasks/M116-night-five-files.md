# M116 - five more campaign-B files in the night (xml-tree.c leaves Red)

Not a generated task file. Branch `campaign/M116`, stacked on `campaign/M115` (#81).
Campaign B on the Mac, night of 2026-10-30/31. Measured with `cs-mcp` 1.5.16, the same
server process as M115 (the version of the committed baseline and of W09).

As in M115, each file was worked by a subagent under one brief, one file at a time. The
brief now requires every commit to be the working-tree state that `verify_step.sh` built
and ran (no batched oracle runs on saved copies, no commits through the index). The
session read every playbook change, every step inside an OpenMP region, and spot-checked
steps of each file.

### Report

| File | Code Health | Commits |
| --- | --- | --- |
| `xml-tree.c` (second pass) | 3.09 → **4.31** | 25 |
| `pixel.c` | 2.38 → 3.73 | 17 |
| `image.c` | 2.77 → 2.98 | 18 |
| `blob.c` | 2.30 → 2.71 | 19 |
| `cache.c` | 2.16 → 2.67 | 8 |

87 refactor commits and 4 playbook commits. Every refactor commit passed the build, the
literal and call guards, `ci_warnings.sh`, the oracle on the cases reaching its function
(0 diverged) and the pre-commit gate. The whole catalogue ran after each file and at the
branch tip: 15,906 cases, 1 diverged, the known `gaps/c5651ffad8` (logcfg). The
write-mask case `resize/6ee45e9cfa` diverged in bursts while the machine was loaded
(load average 10-12) and passed 5 of 5 alone a few minutes later, as before.

`NewXMLTree` went from cc 75, nesting 7, 15 bumps and 310 lines to cc 16, no deep
nesting, 2 bumps and 127 lines, through the two amendments the owner left to the session
in M115 (V step 7's `return(&root->root)`, O step 2's `xml` cursor). `pixel.c` lost its
*Brain Method*. The xml-tree pass stopped at its commit budget, not at a plateau.

### Catalogue changes (each its own commit, before the steps that use it)

- **Recipe N, sibling blocks** (8fe327213): one input struct, filled once, may serve
  several `case` helpers of the same `switch`; leans on Recipe B's condition.
  `pixel.c`'s interpolation switches.
- **Recipe T step 2** (78d953daf): the status flag may also be read after the block in
  the same iteration (the caller's write lands as the call returns, before the read).
  `image.c`'s `IsHighDynamicRangeImage`, inside its parallel row loop; *Deep, Nested
  Complexity* gone.
- **Recipe OC step 1** (7720a5432): a block at the tail of the loop body counts as
  followed by `continue;` (C11 6.8.6.2). `image.c`'s `AppendImages`.
- **New Recipe VC** - Extract Loop-Tail Block Returning Success for a Cleanup Exit
  (4b7079ad0): a loop-tail block whose exits are `continue;` and a V-form cleanup return;
  `continue;` becomes `return(MagickTrue)`, the cleanup exit `return(MagickFalse)`, the
  caller keeps the cleanup. `xml-tree.c`'s `<!ATTLIST` branch, +0.16.

### For the owner

1. **Recipe D with a function as the differing value** (`pixel.c`: the Catrom and spline
   interpolation copies pass `CatromWeights`/`SplineWeights`). Behaviour unchanged; new
   ground for D's wording.
2. **`LogMagickEvent` traces in helpers** (`blob.c`, `cache.c`'s `OpenPixelCache`): the
   trace location is not normalised by the oracle, so blocks holding one stay. Allow a
   recipe that keeps the location (a macro argument), or leave them?
3. **A Recipe S split of `blob.c`** into the typed `ReadBlob*`/`WriteBlob*` wrappers.
4. **The function-count threshold** (about 117 functions, *Number of Functions in a
   Single Module*): the first helper over it costs a one-time ~0.04. Record it in
   *Known plateaus*? Similarly, in `blob.c` a function going from cc 9 to 8 lowered the
   file by about 0.08.
5. **A correction to our own M105 code:** `cache.c`'s `IsCacheSourcePending` and
   `IsCacheClonePending` (1244c57ad) take `const CacheInfo` by value, a copy of several
   kB, up to nine calls in `OpenPixelCache`. Change to a `const` pointer? No recipe
   covers it, so it is not done.
6. **A `static` forward declaration** so helpers can sit below a file's first function
   (CodeScene counts the `#include` block into it): unlocks `image.c`'s `AcquireImage`
   (also asked in M106) and `xml-tree.c`'s recursive `XMLTreeTagToXML` tail.
7. **Record `image.c` (2.98) in *Known plateaus*?** Probes show the score moves only when
   several worst functions change together (`AcquireImage` + `SetImageInfo` 3.24; the
   mask functions + `SmushImages` 3.31); each alone stays at 2.97.
8. **V step 7 / I step 8 with `return(v)`** of a variable the block writes through a
   pointer (every write has landed when the helper returns): opens `XMLTreeInfoToXML`
   and the `<!ENTITY` tail. And may Recipe Y admit V step 7's cleanup forms, as I step 8
   does for I?
9. **5-argument helpers**: probes in `image.c`'s row shells and mask switches need five;
   the playbook's limit is four.

### Kept although flat

- `blob.c` 505209b69 (D): flat, kept because `WriteBlobLong` left *Code Duplication*;
  the follow-up it names was dropped. It also lacks the `Claude-Session` trailer.

### Commit messages with slips (not amended)

- `image.c` 7fd15d9b2 (`DestroyImageInfo`) and 7336d0635 (`CloneImageInfo`): the cc
  and line counts were counted by hand, not measured (CodeScene no longer reports either
  function).
- `xml-tree.c` b8a8153cd: the subject names `NewXMLTree`; the step refactored its helper
  `ParseTagAttributes` (the body says so). 3f2739a62 says `&xml` is taken only at that
  call; `ParseEntities` also passes it to two other helpers, none of which keeps a copy.
- 47 of the 91 M116 commits carry `Co-Authored-By` but not the `Claude-Session`
  trailer; the brief named only the first until the xml-tree pass.

### Where the files plateau

- `blob.c` 2.71: the typed read/write wrappers duplicate each other by type; logging
  blocks; the compression `switch`es under `#if` delegates.
- `cache.c` 2.67: `OpenPixelCache`'s debug traces and its by-value predicates; the
  OpenMP copy loops.
- `pixel.c` 3.73: budget-stopped; the three interpolation functions have more room.
- `image.c` 2.98: see question 7; the OpenMP row loops (status/continue checks, the
  atomic progress block, `&range_exception` written inside the loop); the assert plus
  `LogMagickEvent` preambles of the small public functions; Excess Arguments on public
  signatures.
- `xml-tree.c` 4.31: budget-stopped. `XMLTreeInfoToXML` (two processing-instruction
  loops differing in two literals; exit returns a written variable), `ParseInternalDoctype`'s
  `<!ENTITY` branch (`break`, `continue` and cleanup return mixed), `NewXMLTree`'s tag
  branch (five written variables, I takes four), `FileToXML`; Recipe B on the
  existing helpers with Excess Arguments is next.
