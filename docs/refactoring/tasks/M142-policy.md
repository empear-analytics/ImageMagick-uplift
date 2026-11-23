# M142 - 2026-11-23, campaign B, policy.c

Branch `campaign/M142`, PR #132, on `main` after #131. Scores are from cs-mcp 1.5.16 through
`tools/ch.py`. The oracle baseline is `8ea72475e3`. Whole narrow catalogue at `1c5aa4c29`, with a
clean `build.sh base` of the tip as the candidate: 15,979 cases, 0 diverged.

## Scores

| File | Before | After | Steps |
| --- | --- | --- | --- |
| policy.c | 8.55 | 9.68 | W; S; Q, Q; DT, E, P, DT, P, DT |
| policy-value.c (new) | - | 10.0 | S, split off SetMagickSecurityPolicyValue and its four statics |

policy.c keeps only *String Heavy Function Arguments*, which comes from the public signatures.

## How the chain was found
- **Line budget first.** policy.c was at the *Lines of Code in a Single File* limit. The first Q
  step alone crossed it (8.65 -> 8.22). Recipe OH's Form A could not help: the run must stop
  before `policy_cache`, a non-const file-scope object, and types and tables free no budget.
  The self-contained `SetMagickSecurityPolicyValue` family left by Recipe S instead.
- **Two Q steps in the step 8 form.** The evaluation loop of `IsRightsAuthorizedByName`
  (four accumulators, the member form `*targets->v`) went first. Then the path-domain block of
  `MatchPolicyRights` (six targets, step 8's limit).
- **Overall Code Complexity** was still reported at a lizard mean cc of 3.865 and cleared at
  3.849. Recipe P's ternary form adds a branch, so each P step is followed by DT
  (`obj_identical` IDENTICAL every time).

## Traps seen (probed, not committed)
- **A stub probe cut by text, not by function.** The first ceiling probe cut from the first
  `LockSemaphoreInfo`/`ResetLinkedListIterator` pair, which is in `GetPolicyInfo`. It deleted 15
  functions and reported a false 9.68. Cut stubs by function name.
- **memory.c (8.42):** `AcquireMagickMemory`'s complexity is in the
  `MAGICKCORE_ANONYMOUS_MEMORY_SUPPORT` branch. `AcquireVirtualMemory`'s deepest block holds the
  `MAGICKCORE_HAVE_POSIX_FALLOCATE` `#if`. Neither can move without a build that compiles the
  other side, and there is none on the Mac.
- **utility.c (7.84):** `GetPathTemplate`, `ListFiles` and `GetExecutionPath` are made of
  Windows, VMS and OS/2 branches.
- **token-glob.c (8.42):** extracting `GlobExpression_`'s `'{'` group, which returns on every
  path, scores 6.99, because the new function brings its own nesting, conditionals and size.
  A stub of that group still scores 8.04: findings weigh by the share of code they cover, so
  `GlobExpression_` (cc 32 without the group) and `MatchCharacterClass` must both be cleared
  before the file rises. The group's extraction also needs an amendment to Recipe RC, a
  returning counterpart of its step 6 void form ("wherever the switch stands").
