# M122: campaign B, morning of 2026-11-13, five files

Branch `campaign/M122`, stacked on M121 (PR #99). Same method as M121: one subagent per file
(or group), run one after the other, under one brief; every commit has its `Recipe:`,
`Verified:` and `Claude-Session:` trailers and passed the CodeScene commit gate. Scores are
`tools/ch.py` (cs-mcp 1.5.16), before = M121 tip.

| File | Before | After | Commits | Left |
| --- | ---: | ---: | ---: | --- |
| `identify.c` | 4.70 | 6.97 | 16 | file length (~1,800 lines); `IdentifyImage` cc 13 (writes four locals); `PrintChannelLocations` scan |
| `utility.c` | 5.55 | 6.04 | 5 | `ExpandFilenames` cc 24 and `GetPathComponent` cc 22 (5+ arguments per block); file length |
| `splay-tree.c` | 5.94 | 7.49 | 4 | `Splay` (recursive, 6 arguments); duplication of small accessors |
| `exception.c` | 6.16 | 8.03 | 2 | `IsRepeatedException` (5 arguments), public 8-argument `ThrowMagickExceptionList` |
| `list.c` | 6.45 | 7.09 | 4 | `CloneImages` 2 bumps; `MarkScenesForDeletion` cc 10; accessor duplication |

Mean of the five: 5.76 to 7.12.

New recipes, each in its own `playbook:` commit before its first use: KS (`identify.c`'s
`GetIPTCTagName`, a sparse keyed switch assigning string literals, cc 55) and KR (the
returning twin, `exception.c`'s `ExceptionSeverityToTag`, checked against all 63 case labels
by script).

## Evidence

- Behaviour steps ran `oracle.py run --function ...`; label, ternary and similar steps ran
  `obj_identical.sh`. `tools/ci_warnings.sh` is OK on the five files at the tip.
- Whole catalogue at the tip: 15,961 cases, 3 diverged (`info/d333f9b818`, `gaps/2857e5295a`,
  `gaps/c5651ffad8`: `list configure` and `logcfg`), the same three as in M121, now measured
  against the current `main` baseline (`b772f03dc`).

## For the owner

- `utility.c`: `GetExecutionPath` is reached by no case; its three helper extractions
  (`GetNSExecutablePath`, `GetProcExecutionPath`, `GetExecNamePath`) are guard-level only, and
  the `getexecname` block is not compiled on this Mac.
- `identify.c`: `PrintImageLocations` and `PrintImageSummary` read `image->colorspace` at the
  top instead of taking the `colorspace` local (same value at the same point; keeps four
  arguments). The guarded-call helper steps are labelled E, since no recipe describes them.
- Helpers in `list.c`, `exception.c` and `splay-tree.c` sit between a function's doc comment
  and the function (cosmetic).
