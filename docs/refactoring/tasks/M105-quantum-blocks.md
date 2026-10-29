# M105 - the quantum helpers' guarded blocks, the Index pair, cache.c

Not a generated task file. Branch `campaign/M105`, stacked on `campaign/M104` (#64). Campaign
B on the Mac, night of 2026-10-28/29. Measured with `cs-mcp` 1.5.16.

### Report (2026-10-29, Mac, night)

**quantum-import.c 2.59 → 3.21, quantum-export.c 2.59 → 3.78** (both 1.88 and 2.04 at the start
of M104). cache.c flat at 2.16 (`OpenPixelCache` cc 67 → 56).

| Step | Recipe | Functions | Evidence | Code Health |
|---|---|---|---|---|
| `IsCacheSourcePending`, 11 sites | P | cache.c `OpenPixelCache` | oracle, 15,025 cases | flat 2.16 |
| Depth cases returning `range_exception` (`extract_case.py --returns`) | E | `ImportIndexQuantum`, `ImportIndexAlphaQuantum` | oracle, 2,725 and 52 cases | import 2.59 → 2.84 |
| Guarded blocks that end a helper (`tools/extract_block.py`) | E | 75 import and export depth helpers, one commit each | oracle on the reaching entry function | import 2.84 → 3.21, export 2.59 → 3.78 |

**`tools/extract_block.py`** moves the N-th `if (C) { ... return; }` (or its `else`) of a
static void function into a helper named for its test (`ImportFloat16BitGrayQuantum`,
`ImportUnpacked10BitRGBAQuantum`, `ExportPacked12BitRGBQuantum`, ...). Locals assigned
before the block (`range=...`, `pixel=0`) are passed in (Recipe E step 3); blocks before it
must end in `return;` under a test free of assignments and calls; the block's own writes are
not read after it (steps 4 as amended in M104, and 7).

**Coverage.** A fresh `tools/oracle/linecov.py` at the tip: all 305 helpers created in M104 and
M105 (164 in quantum-import.c, 141 in quantum-export.c) are executed by some case. The case map
knows only the original names, so each step ran the oracle on the entry function that reaches
it (`ImportRGBAQuantum` for `Import10BitRGBAQuantum`).

Whole catalogue at the tip: 15,906 cases, 1 diverged (`gaps/c5651ffad8`, the known Mac logcfg
artifact).

**Incidents.**
- `Import12BitGrayQuantum`: the batch run reported 1 of 1,151 cases diverged (case not
  recorded) while the Mac was loaded; reverted, redone by the same script, 0 diverged in two
  runs. The moved code is verbatim (its trailing `p++` was dead already).
- `Import10BitGrayQuantum`: refused, 2.97 → 2.88 (its unpacked helper takes five arguments).
- Two driver bugs, caught by the build or the tools, never committed: labels already
  extracted were extracted again (duplicate definitions); zsh passed `--returns X` as one word.

**Left.** `Import/ExportPixelChannel` (pointer offset before the switch, six arguments),
`Import/ExportCbYCrYQuantum`, `Import10BitGrayQuantum`, the public entry points (cc 37, six
arguments), and *Number of Functions in a Single Module*, which a split (Recipe S) of the
depth helpers into their own file would answer. In cache.c, `OpenPixelCache`'s debug-log
blocks have no case (`-debug cache` logs timestamps and temporary file names before `-log`
applies; a case needs a log.xml in the case directory), and its memory-cache block either
returns or falls through, which no recipe moves.
