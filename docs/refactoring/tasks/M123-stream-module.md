# M123: campaign B, morning of 2026-11-13, stream.c and module.c

Branch `campaign/M123`, stacked on M122 (PR #100). Same method as M121 and M122. Scores are
`tools/ch.py` (cs-mcp 1.5.16), before = M122 tip.

| File | Before | After | Commits | Left |
| --- | ---: | ---: | ---: | --- |
| `stream.c` | 5.84 | 6.15 | 4 (+3 playbook) | `StreamImagePixels` cc 16 (any new function crosses the function-count threshold: 111 is fine, 112 flagged); ~80 `Stream*Pixels`/`Stream*Quantum` clones; 10 Excess Arguments |
| `module.c` | 6.46 | 6.48 | 1 | whole file is `#if MAGICKCORE_MODULES_SUPPORT` and not compiled in the oracle build; helpers with two string parameters cost 0.10 (String Heavy threshold) |
| `image-view.c` | 6.24 | 6.24 | 0 | five iterators, each held by a failed-fetch `continue` and an `omp atomic` tail; no case reaches them |

New recipes, each in its own `playbook:` commit before its first use: DD (drop a case that
repeats the default), KE (letter-keyed parser switch as a table of records) and DV (drop a
parameter every caller fills from a member of another argument).

## Evidence

- `stream.c`: behaviour steps ran `oracle.py run --function ...` (261 cases for the map
  parser); the DD and DT steps are `obj_identical.sh` IDENTICAL. `tools/ci_warnings.sh` OK on
  both files.
- `module.c` is reached by no case and its body is not compiled by the oracle build, so its
  one step (`IsModuleMatch` out of `GetModuleList`) is guard-level: `refactor_guard.py` OK,
  `--calls` +2, and a syntax compile with `-DMAGICKCORE_MODULES_SUPPORT` shows no new
  diagnostics.
- Whole catalogue at the tip: 15,961 cases, 4 diverged: the three known ones (`info/d333f9b818`,
  `gaps/2857e5295a`, `gaps/c5651ffad8`) and `resize/6ee45e9cfa` (a write-mask `-scale` case
  that diverges under load on the Mac, ORACLE.md), which passed three times alone with
  `--filter resize/6ee45e9cfa --no-cache`.

## For the owner

- KE's table has 22 entries (both cases of each map letter) and the lookup runs once per map
  letter per stream write.
- DV's callees now read `stream_info->quantum_info` instead of a parameter; every caller passed
  that member.
- `image-view.c` and `module.c` are close to a plateau for unreached code under the recipes
  that admit it.
