# M115 - five campaign-B files in one night (delegate.c and utility.c leave Red)

Not a generated task file. Branch `campaign/M115`, from `main` at 3f4076188 (after #79).
Campaign B on the Mac, evening and night of 2026-10-30/31. Measured with `cs-mcp` 1.5.16,
the version of the committed baseline and of W09.

Each file was worked by a subagent under one brief, one file at a time, as W09 did. The
session read every playbook change, every step inside an OpenMP region, and spot-checked
steps of each file before pushing.

### Report

| File | Code Health | Commits |
| --- | --- | --- |
| `utility.c` | 3.65 → **5.55** | 32 |
| `delegate.c` | 2.61 → **4.16** | 25 |
| `annotate.c` | 2.93 → 3.35 | 25 |
| `xml-tree.c` | 2.44 → 3.09 | 24 |
| `property.c` | 2.07 → 2.43 | 23 |

129 refactor commits and 5 playbook commits. Every refactor commit passed the build, the
literal and call guards, `ci_warnings.sh`, the oracle on the cases reaching its function
(0 diverged) and the pre-commit gate. The whole catalogue ran after each file: 15,906
cases, 1 diverged, the known `gaps/c5651ffad8` (logcfg).

The `utility.c` agent tested saved versions of the file and committed through the index.
To be sure each commit is a tested state, the session rebuilt every one of its 32 commits
and ran the oracle on the function it names: all built, all 0 diverged apart from logcfg
and two runs of the known write-mask flake `resize/6ee45e9cfa` (at 5fa6b1140 and
25f05575a; the neighbouring commits, with the same resize code, pass it).

### Catalogue changes (each its own commit, before the steps that use it)

- **New Recipe CN** - Combine Nested Conditions (00a03da47): `if (A) if (B) T`, no `else`,
  becomes `if ((A) && (B)) T`; proven by `tools/obj_identical.sh` printing `IDENTICAL`.
  `delegate.c`'s `GetDelegateInfo`, `IsExecutableToken`; `annotate.c`'s
  `ComplexTextLayout` (Deep Nesting gone).
- **New Recipe OC** - Extract Block Ending the Iteration with Out-Parameters (1b041c1d4):
  a block followed by its loop's `continue;` moves to a `void` helper, each inner
  `continue;` a `return;`. `InterpretDelegateProperties`' backslash escapes, +0.22.
- **New Recipe NS** - Name a Constant Selection (f63a8f6c7): a recurring
  `(c ? k1 : k2)` of two literals becomes a `static inline` call; proven by
  `obj_identical.sh`. `AnnotateImage`'s eighteen gravity signs, cc 56 → 38.
- **Recipe E step 4** (330576098): a temporary written before read is declared in the
  helper even when the caller wrote it earlier.
- **Recipe V step 7 / I step 8, and O step 2** (fd00d972a), the owner's two questions from
  the `xml-tree.c` pass, which the owner left to the session: the cleanup exit may end
  in `return(e);` of a side-effect-free expression (`&root->root`); a pointer parameter
  whose own value the block advances (`xml++`) counts as a plain local, passed as `T **`.

### For the owner

1. **CN and NS at guard level.** Both say they apply to functions no case reaches, on the
   proof of byte-identical object code. AGENTS.md lists only E, G, P and D's unreached
   form there. So far every CN and NS step was on reached functions (oracle-verified as
   well), so nothing rests on it yet. A ruling, and if yes, a line in AGENTS.md.
2. **Recipe SD on a `switch` without `default`.** Would open `GetMagickPropertyLetter`
   (cc 61) and `GetPathComponent` (cc 23). `delegate.c` also chained four SD helpers
   through their defaults (`LoadDelegateCache`): accept?
3. **`ThrowFreetypeErrorException`**, a function-local macro defined after declarations
   (not Recipe H's shape), blocks `RenderFreetype`'s error exits. Hoist it?
4. **Recipe E over compiled `#if` lines** (`FT_LOAD_TARGET_LIGHT` in `RenderFreetype`).
5. **D where CodeScene reports no duplication** (`utility.c` a6f848ab3, 1402f5269): the
   copies are identical; D's *Use when* names the finding. Legal, or amend the wording?
6. **Arrays filled through the pointers they decay to** (`filename`, `magick` in
   `ExpandFilenames`, 03b1f20b6, a24a89bac) were read as writes through a pointer, not
   as writes of a variable.
7. **E with a value passed in and returned** so the path that leaves it alone keeps the
   caller's value (`magick_length`, 156c87834, 6b2185c4e): E step 7's switch form,
   outside a switch.
8. **Recipe E for a struct written member-wise through its address** (`PixelInfo
   *pixel`, `property.c` 48ae67a, which leaned on D's wording).

### Commit messages with slips (not amended)

- `property.c` bb0ad16: a garbled oracle clause; the truth is that `resize/6ee45e9cfa`
  diverged under load and passed alone with `--no-cache`. 59a0d68's subject names
  `ReadPropertyMSBLong` only; it also changes `ReadPropertyMSBShort`.
- `delegate.c` fa26310c5: "more groups lowered the score" held only before the chained
  SD splits.
- `annotate.c` e4c545f04 (NS) says `Verified: guard`; the oracle (177 cases) also ran.
- `utility.c` a6f848ab3: "literals WARN WARN"; 03b1f20b6: "3 of 4 runs" means four
  reruns, the first of which diverged.

### Where the files plateau

- `property.c` 2.43: the `Append*2Text` macros in `InterpretImageProperties` hide
  `goto`; five dispatch switches (cc 77-124) whose groups return from inside `if`s;
  logging prologues (`LogMagickEvent` location is not normalised); file size.
- `xml-tree.c` 3.09: `NewXMLTree` (cc 75, nesting 7), now partly opened by the V step 7
  amendment; ParseEntities/ParseInternalDoctype by the O step 2 amendment; Get/Destroy
  pairs that differ by member names.
- `delegate.c` 4.16: `LoadDelegateCache`'s DOCTYPE scan and a Windows branch around the
  `command` case; `ExternalDelegateCommand`'s waitpid block under POSIX `#if`s.
- `annotate.c` 3.35: `RenderFreetype`'s parallel loop sits at depth 4; the encoding
  chain has `#if defined(FT_ENCODING_*)` lines; RAQM and Postscript paths not compiled
  or reached.
- `utility.c` 5.55: `GetPathComponent`'s switch (no default, a fall-through);
  `ExpandFilenames`' blocks read arrays N/Q do not take as members; VMS, Windows and
  POSIX-only branches.
