# Agent instructions

Read this before changing anything. It applies to every agent and every harness.

## What this repository is

This is a private fork of **ImageMagick** (`empear-analytics/ImageMagick-uplift`), used for agentic
refactoring research. ImageMagick is a C library (MagickCore, MagickWand), a C++ binding
(Magick++), roughly 150 image format coders, and the `magick` command-line tool. It is
used by millions of programs through a published, stable API.

The practical consequence:

> **Behaviour fidelity outranks code quality, readability, and your own judgement.**
> A change that makes the code cleaner but shifts one pixel by one unit, changes one byte
> of an output file, or alters one message is a defect, even if it looks obviously
> correct.

ImageMagick's arithmetic has been tuned over decades, and much of what looks redundant is
there for rounding, for HDRI (floating-point pixels outside [0,1]), or for a format quirk.
**Report what looks wrong, do not fix it.**

## Build and verification tools

The campaign's tools live in `tools/` and `tools/oracle/`. Everything they build or write
goes under `build-oracle/`, which `.git/info/exclude` keeps out of git.

```bash
tools/oracle/build.sh cand      # incremental build of the working tree, seconds
```

The oracle ([`docs/refactoring/ORACLE.md`](docs/refactoring/ORACLE.md)) is the only
mechanism that proves behaviour was preserved. It builds `magick` from the base commit and
from your working tree, runs about 9,500 command lines on both, and fails unless exit
status, stdout, stderr and every output byte are **identical**. There is no tolerance.

`make check` is not a substitute: most of it checks only that a command succeeded.

## The refactoring campaign

If you were given a task ID, your instructions are in
[`docs/refactoring/tasks/`](docs/refactoring/tasks/). Read, in order:

1. Your task file, if you have one, and
   [`docs/refactoring/CAMPAIGNS.md`](docs/refactoring/CAMPAIGNS.md) - two campaigns run
   at once, on two machines; it says which files are yours
2. [`docs/refactoring/PLAYBOOK.md`](docs/refactoring/PLAYBOOK.md) - the catalogue
   of allowed changes
3. [`docs/refactoring/README.md`](docs/refactoring/README.md) - campaign rules and scope
4. [`docs/refactoring/BACKLOG.md`](docs/refactoring/BACKLOG.md) - which files are ready

You may apply **only** the recipes in the playbook. When findings recur that no recipe
reaches, you may add a recipe, following the playbook's *Growing the catalogue*; do not
stretch an existing one. The branch `night1-refactoring` is an earlier pilot and is not
part of this campaign: do not read it or reuse its changes. Work from this repository only:
do not look for recipes in other campaigns' playbooks; new recipes come from what you find
in ImageMagick.

## Hard prohibitions

Never, in any commit:

- Change a numeric literal, string literal, character constant, or enum value.
- Change arithmetic, bitwise operations, shifts, or the order of floating-point
  operations. `a*b*c` and `a*(b*c)` round differently.
- Change a comparison operator, including `<` to `<=`.
- Change a type, including signedness, width, or `float` to `double`.
- Reorder statements that have side effects.
- Delete code that looks dead - it may be reachable from another format, platform, or the
  API.
- **Change anything public**: the name, signature, or behaviour of a function that is not
  `static`, anything declared in an installed header, a struct layout, or an exported
  symbol. ImageMagick is a library; its ABI belongs to everyone who links it.
- **Edit an OpenMP pragma**, or move code into, out of, or across a `#pragma omp` region
  (see the playbook). The oracle runs single-threaded and cannot see a data race.
  Ruling of 2026-11-15 (owner: "you can touch openmp files. If the test cases still pass,
  it's fine!"): files with OpenMP regions may be edited, including pragma lines and moves into,
  out of or across a `#pragma omp` region, one commit per step, when (1) the whole catalogue, narrow
  and wide, stays at 0 diverged, and (2) `tools/omp_compare.sh` (an OpenMP-enabled build of base
  and candidate at one and at four threads) shows no difference on cases that reach the region.
  The agents' conditions on top of the owner's, to keep the oracle's single-threaded blind spot
  honest: every variable the moved or edited region writes keeps the protection it had
  (private, `reduction`, `atomic` or the same `critical`), the commit message quotes that by-hand
  reading, a changed `critical` name says which regions now share one lock, and a step that
  slows the region by more than measurement noise is not taken.
  `GetMagickNumberThreads` runs a region on one thread below about 262144 pixels (four times that
  for `CompositeCopyImage`), so an `omp_compare` run counts only with cases big enough to start
  several threads: `tools/omp_cases/large-images.txt` has such cases for `composite.c`,
  `compare-similarity.c` and `fourier.c`; add one for any other region before relying on it.
- Touch code in a preprocessor branch this build does not compile (X11, OpenCL, Windows,
  a delegate library that is not installed). Nothing can check it.
  The one exception (owner, 2026-11-01): a branch no configuration compiles, `#if 0`, may
  move verbatim with the code that holds it (the playbook's *Preprocessor branches*).
  A second (owner, 2026-11-11): a Recipe S split may move a function whole, with an
  `#if defined(...)`/`#if !defined(...)` branch inside it, verbatim, when a build that
  compiles the branch shows the moved functions' instructions unchanged
  (`tools/split_branch_check.py` prints `IDENTICAL-MODULO-LAYOUT`).
  Rulings of 2026-11-12 (owner, "all of these are ok", answering a list of seven questions):
  a build configured with a feature absent (for example `ac_cv_func_atanh=no`, the
  `ci-gcc-noatanh` configuration) counts as an available build for that check; Recipe S
  splits that relabel most of a file are allowed; Recipe FT (fold an identical twin
  function into its twin) is allowed; the `#if`/OpenMP sites the lanes had listed as owner
  questions (attribute-depth, fourier, layer, resample, feature-texture, `ExecuteRPN`,
  `AcquireKernelBuiltIn`, `paint-floodfill`'s Recipe H, the `?:` fold in magnify) may be
  taken one at a time, each with the sharpest check the situation allows - for an OpenMP
  region, the playbook's rule about which variables a block may write, read by hand and
  quoted in the commit, plus the cases that reach it run with `OMP_NUM_THREADS=1` and
  `=4` and compared; an NS step may accept a reached function whose object code differs;
  an error call moved into a helper may name the helper in `GetMagickModule()`.
  Rulings of 2026-11-13 (owner, "2 and 5 sounds good to me. As long as the tests pass"):
  a Recipe S split may move an `#if defined(__cplusplus)`/`extern "C" {` pair verbatim with the
  family it wraps (the C++ side holds no code), when the split's other checks pass; a
  `LogMagickEvent(...GetMagickModule()...)` call moved into a helper may name the helper, as an
  error call may; a Recipe E (or N, O, Y, Z) step may move a nested `#if defined(...)` block
  whole, directive lines included, into a helper, and may add an `#if defined(...)` pair around
  the helper's definition when the caller keeps its own, if the block is compiled by an available
  build (the wide build for the X11 and shared-memory blocks) and the object comparison there
  (`tools/obj_modulo.sh` or `obj_identical.sh`) is IDENTICAL or MODULO-IDENTICAL. In every case the
  whole catalogue, narrow and wide, must stay at 0 diverged and `ci_linux_warnings.sh` must pass.
  Rulings of 2026-11-13 (owner): a function's opening block, its `LogMagickEvent(TraceEvent,
  GetMagickModule(),...)` call included, may move into a helper, so that trace logs name the
  helper; and the `%` doc-comment banners above functions may be shortened or reworded by a
  recipe (guard level: `tools/obj_identical.sh` IDENTICAL, no code token changed), keeping
  what each says about the API, where CodeScene counts their near-identical text as Code
  Duplication.
  Ruling of 2026-11-15 (owner, "that sounds like ok side effects if the tests pass"): Recipe SZ
  (sink a run of zeroing writes to their first reader) is allowed: a `memset` of a plain local
  or of a heap block the function allocated, or a `static void` helper made of such calls, may
  move past statements that do not name it, as the recipe's conditions require, when the whole
  catalogue, narrow and wide, stays at 0 diverged.
- (Owner, 2026-11-16, answering "which rule may I relax to push the campaign A scores toward 10?") **A file that a Recipe S split produced may be carved again** (Number of Functions, Lines of Code), each new file under the same conditions as the first split (the playbook's Recipe S and its Texture-family rules, the gate that a split product scores at least what its original did, a build in every configuration that compiles the file, no public or MagickPrivate name changed, one function or family per commit). **Long flat chains** (about 30 to 55 one-helper steps whose score does not move until the last) may be committed in a dedicated lane, each intermediate commit that the CodeScene gate refuses taken with `--no-verify` and disclosed, **provided the chain's end score was first measured on a stub probe** and the lane resets the branch if it cannot finish. Public and MagickPrivate signatures stay as they were (the owner did not relax that rule).
- (Owner, 2026-11-16, "You can do all three") **A Recipe S split may share a small `static` helper between the two files** by one `static inline` definition, byte-identical to the original, in a new non-installed private header that both files include (listed under `MAGICKCORE_NOINST_HDRS` in `MagickCore/Makefile.am`); widening the helper to `MagickPrivate` is not taken (it would add a symbol to the library's internal ABI), and copying the helper into both files stays forbidden. The split's own conditions stay (the gate that both products score at least what the original did, a build in every configuration, no public or MagickPrivate name changed, no hot path slowed beyond noise). The lane writes the recipe or amendment in its own `playbook:` commit first. **A timed merge chain for near-twin per-pixel functions** (the `XPack*` LSB/MSB row packers of `xwindow-image.c`) is allowed with the playbook's Recipe MT or a new recipe, each step timed against the baseline binary (median of several runs, no slowdown beyond noise). **A longer lane** may take the 51 to 54 steps of `morphology-apply.c` under the long-flat-chain ruling above.
- (Owner, 2026-11-16, answers to the round-twenty-two questions) **A Recipe S split may share several `static` helpers, a typedef and macros** through the one non-installed private header of the ruling above, each `static inline` function and each typedef and macro byte-identical to the original and defined only there; the ruling's other conditions stand (no copying, no `MagickPrivate` widening, both products at least the original's score, a build in every configuration, no public or `MagickPrivate` name changed, no hot path slowed beyond noise). **A file may be split more than once** (a chain of splits), each split under these rules. **`xwindow-image.c` continues** in a long dedicated lane (the `XPack*` packer merges, the Magnify and annotate/draw twin merges, the carves), each per-pixel step timed against the baseline binary.
- (Owner, 2026-11-16, answers to the round-twenty-three questions) Under the split rulings above: **the private header may hold a shared core of any size** (for example about 30 or more `static inline` helpers, typedefs and macros, as in `quantize.c`'s colour tree); there is no fixed size limit, but **the header is scored like every split product** and must score at least the original's score before the split. **Split files may share a file-scope variable through one accessor**: the variable stays defined once, in one `.c` file, and keeps its storage there; one accessor function, defined in that file and declared in the private header (not installed, not `MagickExport`), is the only new external name per variable; every per-pixel use is timed against the baseline binary as for Recipe TH. **A macro the original defines twice, byte-identically, may become one definition in the private header.** **`visual-effects-pixel-methods.c` may take its three-split chain**, its intermediate drops disclosed. The owner confirmed the lane-written readings of round twenty-three (the self-defined tag in Recipe S, Recipe FB's `return` form, the Recipe PS and D form (b) amendments, Recipe TH and its timing rule) and accepts a disclosed `--no-verify` drop inside a chain of any length when the chain was stub-probed first and its last step raises the file.
- (Owner, 2026-11-16, answers to the round-twenty-four questions) Under the split rulings above: **a private-header helper may hold an OpenMP pragma or a function-level `static` local** (the `quantize.c` remap split), each such helper timed against the baseline binary and its parallel region checked as the OpenMP ruling asks (by-hand write-set reading, `omp_compare` at 1 and 4 threads on cases that really start threads); **a private header may carry a file-scope `static const` object and an `#if`/`#else` macro block verbatim** (`segment.c`); **identical helpers in several private headers may be shared** through one common private header that the others include; **flat carves that add split files without raising the original are accepted** (the two `xwindow-image.c` carves); the seven-split `visual-effects-pixel-methods.c` chain and `visual-effects-polaroid.c`'s grouping by public argument count are accepted; **small Recipe E extractions taken only to clear Overall Code Complexity are accepted** (as `424f6c6ac`); and the round-twenty-four mandate readings are confirmed (Recipe RL step 8, read-only accessors for `static const` objects, a non-inline header helper and a qsort comparator in a header, the disclosed `--no-verify` on a revert to a gate-accepted state).
- Modify a `static const` data table.
- Fix a bug you noticed. Report it.

## Verification sequence

### Before you start on a function

The oracle only protects code its cases reach and check. Find out which level of evidence
the function is at, then decide:

```bash
python3 tools/oracle/casemap.py <Function>     # which cases execute it
```

- **some case executes it**: oracle-verified; the whole playbook applies. Do not run
  mutation testing first (owner's decision, 2026-10-27): the harness is finished, and
  its cases are the protection.
- **no case executes it**: guard-verified; only recipes E, G and P,
  with `refactor_guard.py --calls` reporting `OK`, and every argument of every call you
  add checked against its parameter by reading; and Recipe D's *Unreached functions*
  form, whose proof is `tools/obj_identical.sh` printing `IDENTICAL` for the whole
  object file (owner, 2026-10-30), or `tools/reuse_check.py` with
  `tools/obj_modulo.sh` (owner, 2026-11-10); and Recipes B and N with the playbook's
  *Unreached functions: B and N* checks (`tools/struct_fill_check.py`,
  `tools/obj_modulo.sh`; owner, 2026-11-10); and Recipe I with its
  *Unreached functions: Recipe I* checks (`tools/targets_fill_check.py`,
  `tools/obj_modulo.sh`; owner, 2026-11-10); and Recipe SD with its *Unreached functions:
  Recipe SD* checks (`tools/switch_split_check.py`, `tools/obj_modulo.sh`,
  `refactor_guard.py --calls`; **the agent's addition of 2026-11-13 under the owner's OK of
  2026-11-10 to evolve the recipes, for the owner's confirmation**); and Recipe WP's step 8
  for a wrapper no code calls (`tools/obj_modulo.sh`, `refactor_guard.py --calls`; **the
  agents' addition of 2026-11-15 under the same OK, for the owner's confirmation**).

The rules are in the playbook, *Two levels of evidence*; the reasoning and the measured
results are in [`docs/refactoring/VERIFICATION.md`](docs/refactoring/VERIFICATION.md).
Say in the commit message which level verified the step. When a function is not ready
and a few cases would change that, report the gap: adding cases beats refactoring on the
guard alone.

### After every commit

Run all four, in this order:

```bash
# 1. it still builds, also under ImageMagick's own CI warnings (macOS -Werror, MSVC)
tools/oracle/build.sh cand
tools/ci_warnings.sh <the file you changed>
tools/cxx_warnings.sh <the file you changed>   # as C++: MSVC's bool/enum, shadowing

# 2. no constant was removed or altered (add --calls after an extract or split)
python3 tools/refactor_guard.py <the file you changed>

# 3. behaviour is byte-identical on every case that reaches the function - seconds
python3 tools/oracle/oracle.py run --function <Function>

# 4. the metric improved
python3 tools/ch.py --review <the file you changed>
```

For step 3, name the function you refactored, not a helper you just extracted from it:
the case map knows the original names, and the original still calls the helper.

`refactor_guard.py` reports one of three things:

| Outcome | Meaning |
| --- | --- |
| `OK` | No literal changed. |
| `FAIL` | A value vanished, or a count dropped while another rose: a substituted constant or deleted logic. |
| `WARN` | Counts only dropped with every value still present (deduplication), or only rose (a new guard clause with its own `return`). A request for a second look, not a pass. |

If the guard fails or the oracle diverges, revert immediately:

```bash
git checkout -- <file>
```

### Before pushing a branch

```bash
python3 tools/oracle/oracle.py run              # the whole catalogue, ~2 minutes warm
```

## CodeScene

Code Health is measured with CodeScene, through its MCP server (`cs-mcp`) where the
harness has one, or through `tools/ch.py`, which talks to the same server. Do not use the
`cs` command-line tool for measurement.

- After changing code, run `code_health_review` (or `tools/ch.py --review`) on each changed
  file. If Code Health drops, fix the issues, review again, and confirm the result with
  `code_health_score`.
- Before finishing, run `pre_commit_code_health_safeguard` (or
  `tools/codescene_precommit.py`), and stop only when it passes.
- This applies to the campaign's own tools in `tools/` as well as to ImageMagick's
  sources. For ImageMagick, the playbook's rules still decide which changes are allowed.

## Commits

- One recipe, one function, one commit.
- Message format: `refactor(<file>): simplify <function>`, e.g.
  `refactor(MagickCore/resize.c): simplify ScaleImage`, a body that says what moved, and
  two trailers: `Recipe: <code>` (a code in the playbook) and `Verified: oracle` or
  `Verified: guard`. `python3 tools/check_trailers.py` checks them.
- Never squash several functions into one commit. The campaign relies on being able to
  revert a single step.
- The CodeScene gate, `tools/codescene_precommit.py`, checks what is staged: no library
  file may lose Code Health, and a genuinely new one must score 10.0. Code that only
  moves is held to what it scored before: a file split off another (Recipe S) must
  score at least what its original scored, and in a merge a file identical to the
  merged branch's version is skipped (owner, 2026-10-30). `.githooks/pre-commit` runs
  it on every commit once the clone is set up with `git config core.hooksPath .githooks`.
  It needs `cs-mcp` on the `PATH` (or `CS_MCP_BINARY_PATH`) and `CS_ACCESS_TOKEN`. Do not
  bypass it with `--no-verify`; when it refuses, stop and report, as the playbook says.

## Git remotes and pull requests

This checkout is a **fork**, and the two remotes are easy to confuse:

| Remote | Repository | Use |
| --- | --- | --- |
| `origin` | `empear-analytics/ImageMagick-uplift` | **The target for everything.** |
| `upstream` | `ImageMagick/ImageMagick` | The public project. Fetch only. |

> [!WARNING]
> **Every pull request must target `empear-analytics/ImageMagick-uplift`.** Never open one against
> `ImageMagick/ImageMagick`.

GitHub's "Compare & pull request" button and `gh pr create` both default the base to the
**upstream parent** when the repository is a fork, so the wrong target is the default.
Always name it:

```bash
gh pr create --repo empear-analytics/ImageMagick-uplift --base main
```

`upstream` has its push URL set to `DISABLED`, so a push to it fails rather than
succeeds. Keep it that way. This work is private research, not a contribution.

## When to stop

Stop and report, rather than pressing on, whenever:

- No case executes the function you want to change.
- The build fails and the fix is not obvious.
- The guard fails, or the oracle diverges, and you do not understand why.
- A function's control flow is too tangled to transform confidently.
- The change would touch OpenMP, a public symbol, or code this build does not compile.
- You want to make a change the playbook does not cover and cannot add as a recipe.

**An unfinished task is an acceptable outcome. A silently changed image library is not.**
Reporting "I completed 2 of 5 steps and stopped because step 3 was unclear" is a good
result.
