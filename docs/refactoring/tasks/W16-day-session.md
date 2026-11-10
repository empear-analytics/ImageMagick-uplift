# W16 - campaign A, parallel lanes (2026-11-10)

Campaign A (Windows desktop, WSL2), branch `campaign/W16` cut from `main` 72f10d28d after #91. Four lanes in git
worktrees, as in W14. The owner's rulings of 2026-11-07 apply (a file may pass 110 functions when the chain lifts it;
label-run helpers may be Complex Method; Recipe S splits the tool accepts when the new file scores at least the
original; no new OpenMP exceptions). Start: campaign A mean 8.19, 27 of 74 files at 10.0.

## Lane d, first batch
- `enhance-level-colors.c` 9.36 -> 9.68 (e226d3e2d, cherry-picked).
- **Parked for the owner, not in the PR:** two `shear.c` commits that take it 9.17 -> 10.0 (local branch
  `w16-shear-enum`). The first (c74c70fd5) folds Recipes PC, N and a D reuse into one commit and replaces
  `YShearImage`'s local `UP`/`DOWN` enum and switch with the file's existing `ShearDirection` and an `if`
  (LEFT = UP, RIGHT = DOWN); behaviour is the same (43 cases, 0 diverged) but removing the enum is a rewrite no
  recipe covers. Merge it if you accept that.
- No step on `vision.c`, `paint.c`, `statistic.c`, `effect.c`, `feature.c`, `resize.c`, `visual-effects.c`: their
  remaining findings are public argument counts and file length (or need B on an unreached function).
