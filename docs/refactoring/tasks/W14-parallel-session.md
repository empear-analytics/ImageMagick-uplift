# W14 - campaign A, parallel lanes (2026-11-05)

Campaign A (Windows desktop, WSL2). After #87 the owner found the serial lane too slow and approved
running four agents in parallel; the main session runs on Opus, the hard whole-function chains go to
Opus subagents and the mechanical ones to Sonnet. Each lane is a git worktree `~/im-w14<x>` on branch
`campaign/W14-<x>` with its own candidate build and its own oracle baseline; the main session merges each
lane's finished file into `campaign/W14` and runs the whole catalogue on the merged tree.

| Lane | Model | File | Score | Commits |
| --- | --- | --- | --- | ---: |
| a | Opus | `fx.c` | 3.52 -> 3.62 | 8 + 2 playbook |
| b | Opus | `distort.c` | 4.25 -> 5.15 | 19 + 1 playbook |
| c | Sonnet | `visual-effects.c` | 4.23 -> 4.59 | 3 + 1 playbook |
| d | Sonnet | `enhance-tone.c` | 5.07 -> 5.80 | 7 |
| d | Sonnet | `effect-blur.c` | 5.95 -> 6.01 | 1 |

Whole catalogue on the merged `campaign/W14`: 15,900 cases, 0 diverged. (Lane a saw four `-list`
divergences in its own worktree; they came from its baseline being built under another checkout's path,
which the oracle does not normalise, and are absent on the merged tree.)

**New playbook text under the mandate, not yet ruled on.** Recipe BR (Extract Block Leaving Through a
Loop Break or a Return; helper returns whether to return and sets `broke` through a pointer; step 7 passes
an outer `broke` on), Recipe MB (Extract Block Through Copies of Macro-Bound Locals: copy each macro-named
struct local in, run the block with the macro text unchanged, copy it out; the `#define`s stay
byte-identical), and Recipe LB's parallel-loop-body form (a block inside a column loop of a parallel loop
body takes its `break;` as `return(MagickFalse);`, `status` by pointer).

**Owner question.** `distort.c` is at 110 functions: may a step raise *Number of Functions in a Single
Module* (about -0.15) when the chain it starts lifts the file further?

**Lessons.** Code Duplication is a cliff: a file sits just under its threshold, a helper for one of two twin
blocks is not flagged, the second is (-0.4 to -0.5). Lanes must keep private oracle baselines: two lanes
building the same baseline in one shared directory corrupted it.

**Noticed upstream, not changed.** `TranslateExpression`'s early returns inside the loop skip
`pfx->teDepth--`; `ImageStat` tests `channel >= 0` where `ExecuteRPN` tests `channel > 0`;
`DistortResizeImage` ends a statement with a comma operator; the macro name `InitalBounds` is misspelt;
`ImplodeImage` and `SwirlImage` compute `scale` in different forms; `SelectiveBlurImage` leaks `kernel`
when its first `CloneImage` fails.
