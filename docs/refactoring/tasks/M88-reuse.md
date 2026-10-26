# M88 - reuse helpers once fresh mutation data exists

Not a generated task file: this task follows up four reuses (Recipe D) that earlier
tasks left because the target functions had no usable mutation data (M52, M53, M59).

### Report (2026-10-26, Mac, night)

Each target function was mutated on the Mac tonight, with one Mull build for the three
files (`build-oracle/mull-reuse`, reports `build-oracle/work/mutation-reuse-*.json`).

| Function | Fresh verdict | Reuse | Code Health |
|---|---|---|---|
| ResetSplayTree | ready (13 of 19, 6 unobservable) | `RelinquishSplayTreeNodes` | splay-tree.c 4.62 → 4.88 |
| RemoveNodeByValueFromSplayTree | careful (16 of 21); survivors outside the block | `GetSuccessorNode` | splay-tree.c 4.88 → 4.91 |
| RelinquishMagickResource | ready (1 of 4, 3 unobservable; thin) | `LockResourceSemaphore`, `UnlockResourceSemaphore` | resource.c 4.92 → 5.02 |
| RemoveElementFromLinkedList | careful (4 of 5) | `UnlinkNextElement` | flat at 8.95, no finding moved: reverted |

All commits are Recipe D at oracle level. A script checked each replaced block against
the helper's body. The two splay-tree commits and the resource commit are on branch
`campaign/M88-reuse`.

**Lesson.** The Mac can produce fresh mutation data for a handful of functions in
minutes when few cases reach them. Where missing or stale data is the only obstacle,
that is cheaper than waiting for ERDC.
