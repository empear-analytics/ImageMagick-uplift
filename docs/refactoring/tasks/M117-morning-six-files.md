# M117 - six campaign-B files, four out of Red

Not a generated task file. Branch `campaign/M117`, from `campaign/M116` (#82); after #81,
#82 and #80 (W10) merged, `origin/main` was merged in (e7a1eba10; one playbook conflict
in Recipe O step 2, both texts kept: W10's discarded out-argument rule, then this
branch's exception for a store left unused).
Campaign B on the Mac, early morning of 2026-10-31. Measured with `cs-mcp` 1.5.16, the
same server process as M115 and M116.

One subagent per file under the session brief, as in M115/M116. The session read every
playbook change and every step inside an OpenMP region, and spot-checked steps of each
file.

### Report

| File | Code Health | Commits |
| --- | --- | --- |
| `pixel-export.c` | 3.65 → **6.89** | 30 |
| `pixel-import.c` | 3.66 → **6.69** | 28 |
| `stream.c` | 3.14 → **5.84** | 41 |
| `identify.c` | 3.48 → **4.70** | 35 |
| `profile.c` | 2.92 → 3.58 | 25 |
| `draw.c` | 2.00 → 2.51 | 20 |

179 refactor commits, 5 playbook commits, and one report fix (3196d54f8: the owner does
not report upstream bugs, so the M115 and M116 lists are gone). Every refactor commit
passed the build, the literal and call guards, `ci_warnings.sh`, the oracle on the cases
reaching its function (0 diverged) and the pre-commit gate. The whole catalogue ran after
`draw.c`, `profile.c`, `identify.c` and `pixel-export.c`, and at the branch tip: 15,906 cases, 1 diverged, the known `gaps/c5651ffad8`
(logcfg).

**Reach for the M109 helpers.** `casemap.py`'s map (10-27) predates the
`Export*Quantum`/`Import*Quantum` helpers and shows no case for them. The
pixel-export/import agents checked with the 10-29 line coverage (`build-oracle/oracle.lcov`)
that every `switch` label they moved runs, and ran the oracle on the covering callers
(`Export<T>Pixel`, `ExportImagePixels` and the import mirrors). Each message says so.
The casemap needs a refresh.

**What paid.** Probing first (stubbing function bodies in a scratch copy and scoring it)
showed where each file's score is held: in `draw.c` only exits from *Deep, Nested
Complexity*; in `stream.c` and the pixel files one function, taken apart innermost-first
(map blocks, channel `switch`es through SD, row loops, generic tails, then the cases).
Taking outer blocks first lost (`stream.c`: 2.05). The first near-duplicate helpers cost
about 0.2 through *Code Duplication* before later ones gained.

### Catalogue changes (each its own commit, before the steps that use it)

- **Recipe SD step 1** (fec98c68b): the last group may end at the closing brace without
  `break;` when it moves and stays last. `stream.c`'s four channel `switch`es.
- **Recipe O** (f40153d07, the owner's two `identify.c` questions, left to the session):
  *Use when* covers a loop block as well as a selection; step 2 passes a variable by
  pointer when declaring it in the helper would leave the caller's store before the block
  with no use (`IdentifyImage`'s `x=0;`, which `-Wunused-but-set-variable` would flag on
  macOS). No recipe deletes such a store.
- **New Recipe SR** - Split a Selection at its Default, Returning Success (bd1cecf7c, step 3
  amended 856e0788e): SD for a `switch` whose moved groups end in `return(R)`; the helper
  returns `MagickBooleanType` and the caller's `default:` becomes
  `if (H(...) == MagickFalse) return(R); break;`. `ExportImagePixels`' and
  `ImportImagePixels`' map `switch`es.
- **Recipe N step 2 (d)** (f2b9bc4b8): a struct may be a member when the only writes
  through its pointer members are a callee's, through the `&v` (b) allows, into another
  object (`ExportPixelBuffer`'s `pixels`; the callees take `const ExportPixelBuffer *`).

### For the owner

1. **Confirm Recipe N step 2 (d)** (f2b9bc4b8): you set N's conditions; e98ca99e5 and
   624858197 rest on the amendment.
2. **The SR chain** in `ExportImagePixels`/`ImportImagePixels`: the map parser now runs
   C → K → M → Y → AB → GI → OPR through seven helpers. Correct and mechanical, but it
   reads oddly. Leave, or tidy later?
3. **Mechanical one-block steps as rule-1 sets** (`stream.c`: about 30 steps of +0.01 to
   +0.08 each, one commit each).
4. **Recipe LB step 7 with `S break;`** (`draw.c`'s `ThrowPointExpectedException`, which
   expands to `status=MagickFalse; break;`): holds `RenderMVGContent` and `TracePath` at
   depth 6/5.
5. **Recipe V step 7 with `S` and `return(<expr>)` together** (`draw.c`'s
   `ConvertPathToPolygon`: `points=Relinquish(points); return(DestroyPolygonInfo(...));`).
6. **A Q8 and a non-HDRI build** for the oracle, so `profile.c`'s `ProfileImage` colour
   switches (`#if MAGICKCORE_QUANTUM_DEPTH == 8` in every case; +1.38 if the function is
   stubbed) become movable.
7. **Recipe D: `'8'` and `0x38` as one value**, so `Update8BIMClipPath` can reuse
   `Read8BimSignature`.

### Commit messages with slips (not amended)

- `draw.c`: the CN commit d2f5fac0d says `Verified: guard`; the oracle (474 cases) also
  ran. The L commit 521cf58e2 omits the case count (360).
- `profile.c` 27ee27a51 names LB as the next step; G was used (3ad9426bd).
- `stream.c` 60e4cb2d6: the subject should name `StreamCharQuantum` and the trailer
  `Recipe: SD`; its metrics line ends empty. The storage-case commits (d86b06b34..) call
  `stream_info` and `image` "assigned before the switch"; they are parameters.
- `pixel-export.c` 483a2ff43 says the last group is "`default: default:` … without
  `break;`"; it ends in `break;`.

### Where the files plateau

- `draw.c` 2.51: *Deep, Nested Complexity* in `RenderMVGContent`, `TracePath` (question
  4), `TraceStrokePolygon` (join selections over `box_p`/`box_q` arrays, which N refuses;
  `CheckPathExtent`) and `ConvertPathToPolygon` (question 5). Single exits from *Complex
  Method* or *Large Method* did not move the score in probes.
- `profile.c` 3.58: `ProfileImage` (question 6); `SyncExifProfile` cc 57 (0x0128 near
  Orientation); `Update8BIMClipPath` (question 7).
- `stream.c` 5.84: budget-stopped. `StreamImagePixels`' map-parse loop (Recipe W with
  Y's input struct is next; stubbing it gives 6.22); per-type duplication; callback
  signatures fixed by the cache-method table.
- `identify.c` 4.70: budget-stopped. `IdentifyImage` cc 18, 158 lines (more plain E
  steps remain); `GetIPTCTagName`'s constant `switch` (every split probed draws
  duplication); `PrintChannelDepths`, `PrintChannelMomentsTable`.
- `pixel-export.c` 6.89, `pixel-import.c` 6.69: file size, overall complexity, the
  per-type duplicate families (differ in two or more values, so D refuses), the public
  9-argument signatures; `ImportCharPixel` cc 10 (nine maps). Recipe S would cut size
  but touches the build lists.
