# M104 - Recipe E on the quantum depth switches

Not a generated task file. Branch `campaign/M104`, stacked on `campaign/M103` (#60, the
cs-mcp 1.5.16 note, open), which also carries ImportQuantumPixels' C89 fix (f53ffb2fa).
Campaign B on the Mac. Measured with `cs-mcp` 1.5.16.

### Report (2026-10-28, Mac, night)

**quantum-import.c 1.88 → 2.59, quantum-export.c 2.04 → 2.59.** Code Duplication and
*Lines of Code in a Single File* leave both files (CodeScene now reports *Number of
Functions in a Single Module* instead).

Every `Import*Quantum` and `Export*Quantum` function is its declarations, two asserts
(sometimes a guard that reports and returns), and one `switch (quantum_info->depth)`
whose cases are the bumps. `tools/extract_case.py` moves one case into a static helper
named for its depth (`Import10BitRGBQuantum`, `Import24BitFloatRGBQuantum` for the
floating-point block a falling-through case tests for, `ImportAnyDepthRGBQuantum` for
`default`). The helper takes only the parameters it uses and declares the locals it
uses, with the initialisers they had (`p = span->p`, ...). The script refuses:

- a statement before the switch other than an assert or a guard clause that assigns
  nothing;
- a case reached through a fallthrough path that does anything but test a condition
  free of assignments and calls;
- a `break` under a brace-less loop, a helper name already defined, a case already
  reduced to one call.

The cases write `p` and `q`, which nothing after the switch reads. Recipe E's step 4
refused two outer writes; the amendment in this batch (Recipes T and J already have it)
does not count a variable nothing reads afterwards.

| Step | Recipe | Functions | Evidence | Code Health |
|---|---|---|---|---|
| Depth cases 1-32 | E | import: BGR, RGB, Gray, GrayAlpha, RGBA, RGBO, BGRA, BGRO, CMYK, CMYKA, Multispectral | oracle, 35-7,214 cases each | 1.88 → 2.27 |
| Span | B | ImportCMYKOQuantum, ImportOpacityQuantum (6 → 4 arguments) | oracle, 20 and 64 cases | flat |
| Depth cases | E | ImportCMYKOQuantum, ImportOpacityQuantum | oracle | 2.27 → 2.34 |
| Fallthrough chains (8, 24, 64, default) | E | import, the same functions; CMYKA with CMYKO in one commit (rule 1: each alone 2.59 → 2.54) | oracle | 2.34 → 2.59 |
| Span | B | nine Export functions without it (CMYK, CMYKA, CMYKO, GrayAlpha, Index, IndexAlpha, Opacity, Alpha, Black) | oracle, 28-744 cases | 2.04 → 2.08 |
| All depth cases | E | fifteen Export functions | oracle, 28-6,806 cases | 2.08 → 2.59 |

Many single steps are flat (rule 2): a clone pair (RGBA and RGBO, BGRA and BGRO) moves
the file only once both members are done.

Whole catalogue at the branch tip: 15,906 cases, 1 diverged (`gaps/c5651ffad8`, the known
Mac logcfg artifact, unchanged code).

**Left in the two files.**
- `ImportIndexQuantum` and `ImportIndexAlphaQuantum`: `range_exception=MagickFalse` before
  the switch and its test after it. A variant that returns the flag would reach them;
  not done.
- `ImportPixelChannel` / `ExportPixelChannel`: `q` (or `p`) is offset before the switch,
  and there are six arguments; `ImportCbYCrYQuantum`/`ExportCbYCrYQuantum` keep a counter
  across the switch.
- The 10- and 12-bit helpers are still bumpy (packed or not, quantum 32 or not), and
  `ImportQuantumPixels` and `ExportQuantumPixels` are public (cc 37, six arguments).

**For the owner.**
- The Recipe E step 4 amendment was made under the 2026-10-24 mandate; the steps before
  it in this batch already relied on that reading.
- The CMYKA+CMYKO commit is flat as a pair, not rising as the rule-1 amendment's wording
  says; it finished the family's fallthrough chains.
- Possibly odd, not changed: `ImportBGRQuantum`'s unpacked 10-bit case stores the
  channels in R, G, B order like `ImportRGBQuantum`, while its other depths use B, G, R.
