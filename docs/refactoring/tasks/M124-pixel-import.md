# M124: campaign B, morning of 2026-11-13, pixel-import.c

Branch `campaign/M124`, stacked on M123 (PR #101). `tools/ch.py` (cs-mcp 1.5.16), before = M123 tip.

| File | Before | After | Commits | Left |
| --- | ---: | ---: | ---: | --- |
| `pixel-import.c` | 6.69 | 6.89 | 1 (+1 playbook) | 88 near-identical `Import<Type><Map>Pixels` row loops (Code Duplication), ~2,850 lines, public 9-argument `ImportImagePixels` |

New recipe, in its own `playbook:` commit before its use: KD (a run of `if (LocaleCompare(v,"K")
== 0) return(F(...));` calls of one signature as a `static const` table of key and
function-pointer records plus a lookup loop), used on `ImportCharPixel` (cc 10, left Complex
Method, no function added).

Evidence: 30 cases reach `ImportCharPixel`, 0 diverged; `ci_warnings.sh` OK; whole catalogue at
the tip 15,961 cases, 3 diverged (`info/d333f9b818`, `gaps/2857e5295a`, `gaps/c5651ffad8`), the
same three as in M121 to M123.
