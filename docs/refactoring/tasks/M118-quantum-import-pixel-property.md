# M118 - quantum-import.c out of Red; second passes on pixel.c and property.c

Not a generated task file. Branch `campaign/M118`, stacked on `campaign/M117` (#83).
Campaign B on the Mac, morning of 2026-10-31. Measured with `cs-mcp` 1.5.16, the same
server process as M115-M117.

One subagent per file under the session brief. The session read both playbook changes
and spot-checked steps.

### Report

| File | Code Health | Commits |
| --- | --- | --- |
| `quantum-import.c` | 3.83 → **7.29** | 28 |
| `pixel.c` (second pass) | 3.73 → **4.40** | 17 |
| `property.c` (second pass) | 2.43 → 2.47 | 3 |

48 refactor commits and 2 playbook commits. Every refactor commit passed the build, the
literal and call guards, `ci_warnings.sh`, the oracle on the cases reaching its function
(0 diverged; `resize/6ee45e9cfa` diverged once under load in a `SetImageProperty` run
and passed alone with `--no-cache`) and the pre-commit gate. The whole catalogue ran after
`quantum-import.c` and at the branch tip: 15,906 cases, 1 diverged, the known
`gaps/c5651ffad8` (logcfg).

With `quantum-import.c`, every file on M115's list of sixteen campaign-B files in Red
has now been worked; `pixel.c` and `pixel-export.c`, `pixel-import.c`, `stream.c`,
`identify.c`, `delegate.c`, `utility.c`, `xml-tree.c` and `quantum-import.c` have left it.

**Recipe D on byte-identical bodies.** `ImportUnpacked10BitRGBAQuantum` and
`...RGBOQuantum` had bodies byte-identical to their BGRA/BGRO twins (the session compared
them); each now calls its twin (3c919a846, 5402a77f1), as #67 did for the 12-bit pair.

### Catalogue changes (each its own commit, before the steps that use it)

- **Recipe SD step 1** (5ba638f93): a chain of groups linked by `magick_fallthrough;`
  (`case 24:`, `case 64:`, `default:`) counts as one group when all of it moves, in
  order; its groups may hold a `break;` inside a selection. A kept group may not be part
  of a chain. `quantum-import.c`'s sixteen depth dispatchers, 3.83 to 5.93.
- **Recipe N step 2** (142ffc923): a variable the block reads on entry, then writes,
  and that nothing after the block reads may be a member; the new function binds it to
  a non-`const` local initialised from the member (E step 4's by-value input in another
  form). `pixel.c`'s interpolation average groups (`x_offset--` before the fetch).

### For the owner

1. **Recipe N step 8, read by its purpose, not its letter** (`pixel.c` 04b27d1b0): in
   `InterpolatePixelChannels`' Average case, `x_offset`/`y_offset`, which fill members of
   the input struct, are written between the fill and the call. The helper reads only
   `grid->source` and `grid->destination`, so behaviour is unchanged, but step 8 says no
   member's variable is written in between. Amend step 8 to "no member the function
   reads", or should the step be reverted?
2. **Recipe D on copies CodeScene does not report as duplicates** (`pixel.c` 9a27ac26c:
   the Blend and Mesh cases, differing in the per-channel function). The same question
   as M115's item 5.
3. **A store left with no reader** (`pixel.c` 3196e9f0e, dd19c10dc): `pixels` is now
   declared in the split-off functions, so the caller's `memset(&pixels,...)` is read by
   nothing. It stays, since no recipe deletes code; `ci_warnings` is clean.
4. **Long SD chains on public entry points**: `ImportQuantumPixels` now dispatches
   through five `Import*FamilyQuantum` helpers, each delegating the rest to the next
   (the same shape as M117's question 2).
5. **A recipe for returns hidden in a macro** (`property.c`'s `WarnNoImageReturn`, which
   hides `return((const char *) NULL)`): rewriting it into a status-returning form would
   open the dispatch switches of `GetMagickProperty` and `SetImageProperty`, but changes
   macro text.
6. **Recipe SV with two differing constants** (`property.c`'s `GetICCProperty`, four
   copies, inside an LCMS-version `#else`), with an input struct for the arguments.
7. **Known plateaus**: `property.c` at 2.47 (below), and the helper-shape duplication
   across `pixel.c`'s three interpolation APIs.

### Commit messages with slips (not amended)

- `quantum-import.c` 5402a77f1 carries the RGBA step's message. It is the RGBO step:
  `ImportUnpacked10BitRGBOQuantum` became a call to `...BGROQuantum`; oracle on
  `ImportRGBOQuantum`, 61 cases, 0 diverged; 3 calls in the line coverage; 6.72 → 6.90;
  the calls moved are `SetPixelOpacity`, not `SetPixelAlpha`. 422a12d12 says "Each
  passes by value" for "span passes by value".
- `pixel.c` 1b741252d: "cc 10 -> 6" is an estimate; measured is only that the function
  left *Complex Method*. 3196e9f0e says the caller's `pixels` and `memset` are "read by
  nothing"; Catrom and Spline still passed `pixels` until dd19c10dc.

### Where the files plateau

- `quantum-import.c` 7.29: budget-stopped. `Import1BitGrayAlphaQuantum` and
  `...IndexAlphaQuantum` (main and tail loops near-duplicates, `pixel == 0` against
  `pixel != 0`; extraction writes `p`, `q`, `bit`), `Import10BitGrayQuantum` (LSB/MSB
  mirror branches), the `switch (n % 3)` decode now in three identical copies (one O
  helper with five arguments, reused by D, is next), Excess Arguments on two SD helpers
  (N or Z next).
- `pixel.c` 4.40: the interpolation room left is blocked by *Code Duplication* across
  the three struct APIs (Channel/Channels/PixelInfo); `InterpolateMeshPixelInfo` (four
  triangle blocks differing in index triples), `LogPixelChannels`,
  `GetPixelIntensity`/`GetPixelInfoIntensity`, `ResetPixelChannelMap` untouched.
- `property.c` 2.47: probes show five functions hold the score together (stubbing all
  five: 4.68; any one: at most +0.21): `GetMagickProperty`, `SetImageProperty`,
  `InterpretImageProperties` (`Append*2Text` macros hide `goto`), `GetEXIFProperty`
  (depth 6; a 5-argument key helper) and `GetMagickPropertyLetter`. Their letter groups
  carry too much cc for SD/SR helpers to drop under 9 (five chained helpers probed:
  2.41-2.43), and exit through `WarnNoImageReturn` (question 5).
