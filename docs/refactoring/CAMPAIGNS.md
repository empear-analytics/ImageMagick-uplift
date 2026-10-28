# Two campaigns, two machines

From 2026-10-28 the refactoring runs as two campaigns at once, on disjoint sets of
MagickCore files, decided by the project owner. Each campaign follows
[`AGENTS.md`](../../AGENTS.md) and the playbook unchanged; this file adds who owns what
and how the two stay out of each other's way.

| | Campaign A: pixel operations | Campaign B: infrastructure, I/O, text |
| --- | --- | --- |
| Machine | Windows desktop, in WSL2 (Ubuntu) | MacBook (macOS) |
| Task IDs and branches | `W02` upwards, `campaign/W<nn>` | `M102` upwards, `campaign/M<nnn>` |
| Builds | `build.sh cand`, and `build.sh wide` for the X11/OpenCL files | `build.sh cand` |

## Who owns which file

**Campaign A** (Windows), 58 files:

- the X11 and OpenCL files, measurable only in the wide build there (W01): `accelerate.c`,
  `animate.c`, `display.c`, `opencl.c`, `widget.c`, `xwindow.c`
- `attribute.c`, `attribute-bounds.c`, `attribute-depth.c`, `attribute-hull.c`
- `channel.c`, `channel-alpha.c`, `channel-combine.c`
- `colormap.c`, `colorspace.c`, `compare.c`, `composite.c`
- `decorate.c`, `decorate-raise.c`, `distort.c`, `effect.c`, `enhance.c`
- `feature.c`, `feature-texture.c`, `fourier.c`, `fx.c`, `gem.c`, `histogram.c`
- `layer.c`, `layer-merge.c`, `magnify.c`, `montage.c`, `morphology.c`, `paint.c`,
  `prepress.c`
- `quantize.c`, `quantize-posterize.c`, `resample.c`
- `resize.c`, `resize-image.c`, `resize-sample.c`, `resize-scale.c`, `segment.c`
- `shear.c`, `shear-deskew.c`, `shear-rotate.c`
- `statistic.c`, `statistic-function.c`, `statistic-image.c`, `statistic-moments.c`
- `threshold.c`, `threshold-adaptive.c`, `threshold-auto.c`
- `transform.c`, `transform-tiles.c`, `vision.c`, `visual-effects.c`

**Campaign B** (Mac), 62 files: every other MagickCore `.c` file, among them `annotate.c`,
`blob.c`, `cache.c`, `color.c`, `draw.c`, `geometry*.c`, `identify.c`, `image*.c`,
`pixel*.c`, `profile.c`, `property.c`, `quantum*.c`, `stream.c`, `string.c`, `token*.c`,
`utility.c`, `xml-tree.c` and the configuration loaders (`configure*.c`, `delegate.c`,
`locale.c`, `log.c`, `policy.c`, `type.c`).

**Neither:** `nt-base.c`, `nt-feature.c`, `vms.c` and `distribute-cache.c`, written off
(owner, 2026-10-04).

A file made by a split (Recipe S) belongs to the campaign that owns the original. Change
only your own files. If a step would need a change in the other campaign's file, leave it
and ask in your PR.

## Why the line runs there

- The Windows machine has the wide build with Xvfb and pocl (W01), the only place the six
  X11/OpenCL files can be verified. `accelerate.c` mirrors operations in A (blur, resize,
  convolve).
- Linux compiles FFTW, which brings `fourier.c` and `compare.c`'s FFT similarity functions
  into reach.
- Linux runs magick with `MALLOC_PERTURB_` (oracle.py, `env_for`), so the `-scale` under a
  write mask cases that diverge at random on the Mac (`resize/6ee45e9cfa`, ORACLE.md) are
  stable there, and they sit in A's resize code.
- The font cases (`Narrow.ttf`, `Quirk.ttf`) for `annotate.c` and `type.c` were made and
  checked on the Mac.

The two halves are about even: 23 red files in A (6 of them X11/OpenCL), 18 in B (sweep
of 2026-10-28, MagickCore mean 5.69).

## Shared files

- **The playbook.** Either campaign may add or amend a recipe, under *Growing the
  catalogue*. Pull `main` first, and append your row to *Catalogue changes*; a conflict
  there is two appended rows, keep both in date order. Name the campaign in the row's
  evidence column.
- **Split registration.** `tools/register_source.py` inserts the new file right after its
  original in `Makefile.am`, `Makefile.in`, `MagickCore/Make.com` and
  `scripts/format_c_api_docs`. Two splits of neighbouring originals can conflict there;
  keep both insertions, each after its own original.
- **Tools and AGENTS.md.** A change to a tool both campaigns use goes in its own commit and
  is named in the PR body, so the other side picks it up after the merge.
- **Measurements.** Code Health scores do not depend on the machine, given the same
  `cs-mcp` and `cs` versions (1.5.13 at the split); record the versions as the baseline
  does. Oracle results do: each machine verifies against its own baseline, built there.

## Pull requests

Each campaign opens batch PRs against `main` of `empear-analytics/ImageMagick-uplift`, as
AGENTS.md says, and stacks its branches on its own previous batch. The two campaigns'
PRs touch different files, so the owner can merge them in any order.

## Starting campaign A

1. Clone `main` into WSL's own filesystem (`~/...`, not `/mnt/c`), with `origin` at
   `empear-analytics/ImageMagick-uplift`, and set `git config core.hooksPath .githooks`.
2. Install `cs-mcp` 1.5.13 and set `CS_ACCESS_TOKEN`; check that
   `python3 tools/ch.py MagickCore/fx.c` prints a score.
3. `python3 tools/oracle/oracle.py selfcheck` until it reports 0 nondeterministic
   ([PORTING.md](PORTING.md)), then `python3 tools/oracle/casemap.py` and
   `python3 tools/oracle/linecov.py`, and the same with `WIDE=1` for the wide build.
4. **First task, W02:** ImageMagick's own CI (`Main`) still fails on A's files.
   `tools/ci_warnings.sh` reports declarations after statements in `shear.c`,
   `transform.c` and `vision.c` (nine in `ConnectedComponentsImage`), each a Recipe B
   struct to move up or fill as step 5 says. Clang reports only the first such
   declaration in a block, so run it again after each fix until the file is clean.
5. Then the campaign proper: the red files first, by the playbook.
