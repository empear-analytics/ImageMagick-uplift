# M102 - ImageMagick's own CI build, and the split into two campaigns

Not a generated task file. Branch `campaign/M102`, from `main` at `b4628e4e4` (#58), also
the oracle's baseline. Measured with `cs-mcp` 1.5.13, on the Mac.

### Why

ImageMagick's own workflow, `Main` (`.github/workflows/main.yml`), was green on `main`
through #30 and has failed on every merge since #31 (2026-10-25). Two causes, both from
the campaign:

- **Build MacOS arm64** compiles with `-Wdeclaration-after-statement -Werror`. Recipe B
  step 4 placed each struct "directly before the one call", after statements: 40
  reported sites in 20 files, more behind them, since clang reports only the first such
  declaration in a block.
- **Build Windows x64, x86 and arm64** (MSVC, `/WX`): C4701, `distance` and `point`
  "potentially uninitialized" in `attribute-hull.c`. Two earlier Recipe E extractions
  (`GetMinimumBoxAngle`, `MoveBoxToOrigin`) kept values local to the helpers that the
  caller held before the edge loop.

The oracle builds use neither `-Werror` nor these warnings, so every step passed.

### What changed

- `tools/ci_warnings.sh <file>`: compiles a file with the macOS job's flags, plus
  `-Wconditional-uninitialized`, clang's counterpart of C4701 (it flags `distance` at
  the line MSVC reported). One known upstream diagnostic is exempt (`fx.c`, `iaStr`).
  It joins the verification loop (AGENTS.md, playbook).
- Recipe B step 5 amended (two playbook commits): the struct goes among the declarations
  at the top of the block holding its calls, `const` with its initialiser where the
  operands are final there, otherwise declared plain and filled member by member where
  it takes its values.
- `attribute-hull.c`: `MoveBoxToOrigin` returns the corner (Recipe E step 3);
  `GetMinimumBoxAngle` passes out `angle` and `distance` (Recipe O). Both restore the
  caller's values from before the extractions.
- One Recipe B commit per function for the declarations, in `annotate.c`, `composite.c`,
  `configure.c`, `delegate.c`, `distort.c`, `feature.c`, `fx.c`, `identify.c`,
  `locale.c`, `morphology.c`, `pixel-export.c`, `pixel-import.c`, `policy.c`,
  `quantize.c`, `quantum-export.c`, `type.c`. All are clean under `ci_warnings.sh`; no
  file's Code Health changed.
- `annotate.c` 2.77 → 2.92: filling `RenderType`'s struct added lines to a *Large Method*
  and cost 0.01, which the pre-commit gate refuses. Two Recipe E extractions came first
  (`GetTypeInfoByFamilyList`, `GetDefaultTypeInfo`), and the fix then left the score as
  it was.

### Left for the two campaigns

| File | Campaign | Reported now |
| --- | --- | --- |
| `quantum-import.c` | B (Mac) | 1 site |
| `shear.c` | A (Windows), task W02 | 2 sites |
| `transform.c` | A (Windows), task W02 | 1 site |
| `vision.c` | A (Windows), task W02 | 9 sites, `ConnectedComponentsImage` |

`Main` stays red until all four are clean (and possibly on more MSVC warnings that only
the Windows jobs show). The split into two campaigns is in [CAMPAIGNS.md](../CAMPAIGNS.md).

### Notes

- The move-up form adds no line of code; filling adds one per member. In a function with
  *Large Method* that can cost a hundredth, so look for a real extraction first.
- `resize/6ee45e9cfa` (`-scale` under a write mask) diverged once on `ExportQuantumPixels`
  and passed twice alone; the known Mac flake (ORACLE.md).
