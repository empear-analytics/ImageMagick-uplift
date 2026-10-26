# M98 - two open catalogue questions, decided

On 2026-10-26 the owner delegated two open questions to Claude Code ("You can make what
you feel is right"). This file records the decisions and the evidence behind them.
Branch `campaign/M98`, on `campaign/M97` (PR #51).

### 1. Guard-level reuse of a predicate (the question from PR #16): allowed, under conditions

Recipe P is amended (`playbook: amend Recipe P - several sites`). One predicate may
replace the same expression at several sites, at guard level, under three conditions:
- a script confirms that each site's condition equals the predicate's body with the
  arguments substituted, character for character, whitespace aside;
- every argument is an identifier, a member access or a literal;
- `refactor_guard.py --calls` shows only the expected counts: +2 for a new predicate, +1
  per further site, and one fewer of each callee inside the expression for each copy
  beyond the first.

The reasoning:
- This is still expression motion, the safest kind of step in the catalogue.
- The one thing reuse adds over a single site is the claim that the copies are the same.
  A script check settles that better than reading.

First use: delegate.c `GetDelegateList`. Its decode and encode tests became
`IsListedDelegateMatch`, script-checked; the counts were exactly +3 and −1
`GlobExpression`. delegate.c went 2.38 → 2.39.

### 2. Recipe B for new helpers ("extract into a helper with a context struct"): not added

The hypothesis was this. Several extractions refused today needed five or more
parameters, and the Excess Number of Function Arguments finding made them lower the
score; a context struct (Recipe B's machinery, applied at extraction time) would remove
that penalty. It was tested on the two refused cases:

| Block | 5 parameters (as reverted) | with a context struct | Why |
|---|---|---|---|
| feature.c, GetImageFeatures' co-occurrence loop | 1.81 → 1.78 | flat at 1.81 | the helper keeps Bumpy Road (3), Deep Nesting (4), Complex (cc 15) and Large (73 lines) |
| type.c, GetTypeInfoByFamily's scoring pass | 4.21 → 4.20 | 4.21 → 4.20 | the helper's Bumpy Road (2) and Complex Method (cc 11) |

The struct removes the argument penalty as intended, but in neither case was the penalty
what blocked the step. The extracted blocks are themselves complex, and their findings
cancel the gain in the caller. Under *Growing the catalogue*, a recipe earns its place by
unblocking work that pays, so this one is not added. Both trials were reverted.

Recipe B remains as it is: bundling the arguments of existing static functions. Where an
existing helper carries the Excess Arguments finding (the CLAHE helpers, pixel.c's
Import/Export, the shears), B applies today with oracle-level evidence. Those are the
targets for B, not new extractions.

**Revisit when:** a block refused only for its parameter count turns out to be simple
(cc ≤ 8, no nesting) once moved. Then the context-struct extraction would pay, and a
recipe can be added with that block as its provenance.
