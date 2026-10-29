# M107 - CbYCrY and the 1-bit Index helpers

Not a generated task file. Branch `campaign/M107`, stacked on `campaign/M106` (#66). Campaign
B on the Mac, morning of 2026-10-29. Measured with `cs-mcp` 1.5.16.

### Report (2026-10-29, Mac, morning)

**quantum-export.c 3.99 → 4.09 (out of the red band), quantum-import.c 3.29 → 3.48.**

| Step | Recipe | Evidence | Code Health |
|---|---|---|---|
| `ImportCbYCrYQuantum`: the unpacked 10-bit block to `ImportUnpacked10BitCbYCrYQuantum`, by hand (the case sets `pixel=0` before its test, which the tools refuse); `pixel` passed | E | oracle, 20 cases; the moved lines run in 3 | import 3.29 → 3.46 |
| `ExportCbYCrYQuantum`: case 10 to `ExportUnpacked10BitCbYCrYQuantum` (`tools/extract_switch_case.py`); `n` and `quantum`, set before the switch, passed (five arguments) | E | oracle, 28 cases; the moved lines run | export 3.99 → 4.09 |
| `GetColormapBit`, the min-is-white bit test, two copies in `Import1BitIndexQuantum` | D | oracle, 2,725 cases | import 3.46 → 3.48 |
| the same helper, two copies in `Import1BitIndexAlphaQuantum` | D | oracle, 52 cases | flat 3.48 |

Whole catalogue at the tip: 15,906 cases, 1 diverged (`gaps/c5651ffad8`, the known logcfg
artifact).

**Possibly bugs, not changed.**
- `ExportCbYCrYQuantum` writes nothing for packed 10-bit samples: case 10 is
  `if (pack == MagickFalse) {...; break;} break;`, so a packed 10-bit image never reaches
  `default`, whereas `ImportCbYCrYQuantum` falls through to it.
- `Import1BitIndexAlphaQuantum`'s main loop never advances `p` (no `p++` after each byte), so
  every group of four pixels reads the same input byte.

**Left in the quantum files.** The 12-bit unpacked helpers' three `x % 3` switches (one value
apart, but a shared helper would take five arguments and returns `q`), `Import10BitGrayQuantum`
(its extraction lowers the file), the 1-bit GrayAlpha helpers, and the public entry points.
