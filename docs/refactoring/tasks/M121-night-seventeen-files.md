# M121: campaign B, night of 2026-11-12/13, seventeen files

Branch `campaign/M121`, stacked on M120 (merged as #97). One general-purpose subagent per
file, one after the other (they share `build-oracle/cand`), each under the same brief; every
commit carries its `Recipe:`, `Verified:` and `Claude-Session:` trailers and passed the
CodeScene commit gate. Scores are `tools/ch.py` (cs-mcp 1.5.16), before = M120 tip.

| File | Before | After | Commits | Left |
| --- | ---: | ---: | ---: | --- |
| `type.c` | 4.76 | 6.23 | 16 | `LoadTypeCache` cc 27 (throw in the include loop), 5-argument functions |
| `log.c` | 4.29 | 5.89 | 12 | `LoadLogCache`, `TranslateEvent` cc 22 (needs a context struct), 6-argument public API |
| `delegate.c` | 4.16 | 6.27 | 21 | `InterpretDelegateProperties` (macros that `return`), throws, public 5-argument signatures |
| `locale.c` | 5.09 | 6.28 | 12 | `LoadLocaleCache` cc 24 (shared state), hot `LocaleNCompare` left alone |
| `configure.c` | 6.47 | 7.91 | 7 | `LoadConfigureCache` cc 19 (throws), `GetConfigureOptions` log call |
| `mime.c` | 6.85 | 9.61 | 10 | `LoadMimeCache` cc 9 (throws) |
| `string.c` | 4.57 | 6.64 | 9 | `CopyMagickString` (hand-unrolled hot path), hex-line bumps, public `FormatMagickSize` |
| `constitute.c` | 4.76 | 5.60 | 7 | `ReadImage` cc 49 (throw-bearing decoder branches), `WriteImage` cc 23 |
| `resource.c` | 5.02 | 7.04 | 11 | big per-resource switches, a 5-argument log helper |
| `option.c` | 5.54 | 7.78 | 5 | 2,286 lines of tables, a log prologue shared by seven functions |
| `geometry-parse.c` | 4.49 | 7.04 | 11 | `GetGeometry` cc 35, `ParseGeometry` cc 39 (the meta-character switch) |
| `quantum-export.c` | 5.02 | 8.28 | 23 | function count, 5- and 6-argument signatures |
| `annotate.c` | 3.93 | 4.20 | 14 | `RenderFreetype` cc 44, `AnnotateImage` cc 28 (need 5+ arguments) |
| `xml-tree.c` | 4.31 | 4.56 | 7 | module function-count threshold: one more function costs 0.12 |
| `compress.c` | 5.31 | 6.58 | 16 | Huffman decode/encode (macros with `break`, five written variables) |
| `image.c` | 4.20 | 5.53 | 19 | `SmushImages`, `AppendImages`, two functions no case reaches |
| `pixel.c` | 4.40 | 7.27 | 14 | public 7- and 8-argument interpolators, `SortImagePixels` atomic block |

Mean of the seventeen: 4.89 to 6.63.

## Evidence

- Every behaviour step ran `oracle.py run --function ...`; every whitespace, label, ternary or
  macro-hoist step ran `obj_identical.sh` or `pp_identical.sh`. `tools/ci_warnings.sh` is OK on
  all seventeen files at the tip.
- Whole catalogue at the tip, on the Mac, case map rebuilt after the merge from main:
  15,961 cases, 3 diverged (`info/d333f9b818`, `gaps/2857e5295a`, `gaps/c5651ffad8`: `list
  configure` and `logcfg`), the same three that diverge with the unmodified source.
- Single-function runs sometimes showed `resize/6ee45e9cfa`, `resize/6efddf0c0a` or
  `gaps/a355c0c1c8` (write-mask cases that diverge under load on the Mac, ORACLE.md); each
  passed alone with `--filter ID --no-cache`.

## New recipes (each in its own `playbook:` commit before its first use)

LL (`type.c`), PA and HE (`geometry-parse.c`), FO (`image.c`). The loaders reused SF, QS, NG,
DL and DT from M120.

## For the owner

- **Labels.** `option.c` commits `7a9f0f2ec` and `c4b01b09c` (a letter-to-flag table and the
  67-label `GetOptionInfo` switch as a keyed table, checked against the old switch label by
  label) carry `Recipe: KT`, whose text covers only assign-form switches and sparse integer
  keys; a table-lookup recipe would fit better. `log.c` commit 2 (`QS`) also holds E-style
  helpers; `geometry-parse.c` commit 2 bundles E with a guard return.
- **Reach.** Little or nothing covers: `RenderPostscript` (annotate.c, guard-verified by
  reading and `refactor_guard.py`), `InterpolateAverageChannels` and `LogPixelChannels`
  (pixel.c), `NewMagickImage`/`SetImageColor` (image.c), the HEX branch of `StringToStrings`,
  `ListConfigureInfo` (compared by hand against the base build), `InvokeDelegate` (12 cases).
- **Argued by reading.** `delegate.c`'s `GetMagickPropertyLetter` raises its NoImage and
  NoImageInfo warnings before dispatch (a script checked that the letter sets equal the old
  case sets); `mime.c`'s `GetMimeEscape` table equals the old switch; `compress.c`'s
  `CountHuffmanRun` tests `n > 0` before `*q` in the black loop (both pure, the read at
  `n == 0` is gone); `constitute.c`'s `ApplyExtractGeometry` keeps the `ParseRegionGeometry`
  call with a `(void)` cast.
- **Message slips.** A few commit bodies have sloppy score lines (`compress.c` MB commits,
  `type.c` `SkipCommentElement`); the numbers in the table above are right.
- **Plateaus.** Most of what is left needs a context struct for the loaders' shared state
  (`q`, `token`, `keyword`, `status`), which the catalogue does not allow; the `Throw*` calls
  stay in their functions because `GetMagickModule()` names the function.
