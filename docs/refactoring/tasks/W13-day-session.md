# W13 - campaign A, evening session (2026-11-04)

Campaign A (Windows desktop, WSL2), branch `campaign/W13`, cut from `main` after #86 merged.
A fresh sweep of `MagickCore/` (cs-mcp 1.5.16) gave a mean of 6.65 and showed several plateau
rows were stale (`magnify.c` was 8.15, not 6.57). One subagent per file, sequentially; the whole
catalogue ran at the last commit of each file: 15,900 cases, 0 diverged.

| File | Score | Commits | Recipes |
| --- | --- | ---: | --- |
| `magnify.c` | 8.15 -> 8.2 | 1 | PC |
| `feature-texture.c` | 5.18 -> 5.25 | 2 | CN |
| `resize-image.c` | 6.83 -> 7.68 | 1 | PC |
| `layer.c` | 7.07 -> 7.08 | 2 | LB, V |
| `shear.c` | 7.62 -> 8.03 | 3 | W, O |
| `montage.c` | 7.36 -> 7.66 | 7 | E, W, O |
| `channel.c`, `resample.c`, `shear-rotate.c` | 6.93, 7.42, 7.63 | 0 | none legal |

**Lesson of the session.** In a file with few functions a new helper switches on *Overall Code
Complexity* (channel.c lost about 0.35 on every probe; feature-texture.c 0.76), and a second
look-alike PC head is a Code Duplication clone (resize-image.c `VerticalFilter` 7.41, shear.c 7.43).

**For the owner.** (1) `layer.c` has `#if DEBUG_OPT_FRAME`, a macro defined as 0 in the file, so no
configuration compiles it. The `#if 0` exception names only a literal `#if 0`. A probe that moved the
background-expansion block (debug text dropped) scored 7.08 -> 7.19. May an `#if M` branch move
verbatim when `M` is unconditionally defined as 0 in the same file? Not used until you rule.
(2) `layer.c` 312b1ba9c is flat as the first step of a two-step plan (rule 2). (3) `montage.c` commits
1, 2, 3 and 6 cover allocation- and thumbnail-failure paths no case reaches (checked by reading);
the oracle normalises the helper's `GetMagickModule()`.

**Noticed upstream, not changed.** `GetImageFeatures` green-channel guard and divisor use
`density_y[x].direction[i].red`, `density_xy` is not cleared between passes, a failed co-occurrence
pass leaks; `ChannelFxImage` returns NULL without destroying the list when `SetImageStorageClass`
fails; `YShearImage` reports progress against `image->rows` over a width loop and
`IntegralRotateImage`'s 180-degree `progress++` races; `MontageImageList` leaks on allocation
failure; `ResamplePixelColor` leaves `status` unused.
