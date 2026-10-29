# M109 - the generic map loops of pixel-export.c and pixel-import.c

Not a generated task file. Branch `campaign/M109`, stacked on `campaign/M108` (#70). Campaign B
on the Mac, day of 2026-10-29. Measured with `cs-mcp` 1.5.16.

### Report (2026-10-29, Mac, day)

**pixel-export.c 3.11 → 3.65, pixel-import.c 3.12 → 3.66.**

Each of the fourteen `Export<Type>Pixel` and `Import<Type>Pixel` functions ends in a generic
loop over the caller's channel map, whose innermost statement is a `switch (quantum_map[i])`
storing one channel. That switch reads `image`, `p` and `q` and writes only through the
destination pointer; it assigns no local and holds no return. Recipe E moves it to
`Export<Type>Quantum` / `Import<Type>Quantum(image,quantum,p,q)`, called with `quantum_map[i]`
(script: the session's `mapswitch.py`, which refuses a switch that writes `p` or `q`). Each
function loses Deep, Nested Complexity (depth 5) and its Complex Method falls from cc 25 to
13 or 14.

| Step | Recipe | Evidence | Code Health |
|---|---|---|---|
| `ExportCharPixel` | E | oracle, 29 cases | export 3.11 → 3.18 |
| `ExportDoublePixel` | E | oracle, 15 cases | 3.18 → 3.26 |
| `ExportFloatPixel` | E | oracle, 21 cases | 3.26 → 3.33 |
| `ExportLongPixel` | E | oracle, 15 cases | 3.33 → 3.41 |
| `ExportLongLongPixel` | E | oracle, 15 cases | 3.41 → 3.48 |
| `ExportQuantumPixel` | E | oracle, 15 cases | flat 3.48 (kept by rule 2: depth 5 gone, cc 25 → 13) |
| `ExportShortPixel` | E | oracle, 23 cases | 3.48 → 3.65 |
| `ImportCharPixel` | E | oracle, 30 cases | import 3.12 → 3.19 |
| `ImportDoublePixel` | E | oracle, 14 cases | 3.19 → 3.26 |
| `ImportFloatPixel` | E | oracle, 20 cases | 3.26 → 3.34 |
| `ImportLongPixel` | E | oracle, 14 cases | 3.34 → 3.41 |
| `ImportLongLongPixel` | E | oracle, 14 cases | 3.41 → 3.49 |
| `ImportQuantumPixel` + `ImportShortPixel`, one commit (rule-1 amendment: each alone 3.49 → 3.48) | E | oracle, 14 + 22 cases | 3.49 → 3.66 |

Whole catalogue at the tip: 15,906 cases, 1 diverged (`gaps/c5651ffad8`, the known logcfg
artifact). Line coverage at the tip: all fourteen new helpers run (336 to 6,480 calls each).

**Left.** The generic loop itself (both bumps) could move only to a helper of six arguments
(`image`, `roi`, `map`, `quantum_map`, the destination and `exception`); the seven
fixed-map helpers of each type differ from each other in two or more places, so Recipe D
does not apply to them. `ExportQuantumQuantum` and `ImportQuantumQuantum` keep the family's
naming, awkward as it reads.

**Also tried in this session, not committed.**
- `cache.c`: `GetVirtualPixelCacheNexus`'s virtual-pixel colour switch to a helper,
  2.16 → 2.12 (the helper becomes a Complex Method of its own).
- `property.c`: most of `GetMagickProperty`'s branches use `WarnNoImageReturn`, which returns
  and reports through `GetMagickModule()`; moving them would change the reported function.
- `xml-tree.c` (three sites) and `profile.c` (one): the Complex Conditionals assign locals
  inside the test (`i+=2`, `xml=strchr(...)`, `p+=2`, `TryAccumulate(...,&new_num)`), which
  a Recipe P predicate cannot do without pointer parameters.
