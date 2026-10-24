# Refactoring Playbook

This is a **catalogue that grows**. Apply only the recipes below. When findings recur
that no recipe reaches, do not stretch an existing recipe: add a new one (see *Growing the
catalogue*), or record a plateau.

Every recipe here is *behaviour-preserving*: it moves code around without changing what
the program computes. That property is what makes the campaign safe to run at scale, and
the oracle ([`ORACLE.md`](ORACLE.md)) is what checks it, byte for byte.

The catalogue starts with the five founding recipes of an earlier campaign (E, G, P, D
and S), each adapted here for what ImageMagick is: a published library, parallelised with
OpenMP, compiled differently on every platform. Every recipe added since is listed in
*Catalogue changes* with the evidence behind it.

---

## The three rules

1. **One recipe, one function, one commit.** Never combine two recipes in one commit.
2. **Re-measure after every commit.** If the Code Health score improved, keep the step. If
   it fell, revert. If it did not move, keep the step only when it is part of a plan: the
   review (`python3 tools/ch.py --review <file>`) shows the targeted function left a smell
   category or moved towards a finding's threshold (lower complexity, less nesting, fewer
   arguments), and the commit message names the finding and the next step towards it.
   Otherwise revert.
3. **When in doubt, stop and report.** An unfinished task is fine. A silently changed
   image library is not.

---

## Absolutely forbidden

These change behaviour, or change what other programs can rely on, and must never appear
in a campaign commit:

- Changing any numeric literal, string literal, character constant, or enum value.
- Changing arithmetic (`+ - * / %`), bit operations, shifts, or the grouping of
  floating-point operations. Floating-point addition and multiplication are not
  associative: `a*b*c` is `(a*b)*c`, and rewriting it as `a*(b*c)` changes the last bit,
  which the oracle compares.
- Changing a comparison operator, including `<` to `<=`.
- Reordering statements that have side effects (assignment, I/O, function calls,
  exceptions thrown).
- Changing types: signedness, width, `float` versus `double`, `MagickRealType` versus
  `double`, `Quantum` versus anything else. Under HDRI these are not the same.
- "Fixing" a bug you spot. **Report it, leave it.**
- Deleting code that looks dead. It may be reached by another format, another platform,
  another build configuration, or a program calling the API.
- Touching a `static const` data table.
- **Changing anything public.** Every function that is not `static` is part of the API or
  of the library's internal ABI: do not rename it, change its signature, change its
  linkage, or move it to another file. The same goes for struct layouts, macros and
  anything else in a header.

### OpenMP regions

About 380 loops in MagickCore run under `#pragma omp parallel for`. The oracle builds
with OpenMP disabled, so it cannot see a data race or a change in what is shared between
threads. Therefore:

- **Never add, remove, move, or edit a `#pragma omp` line or its clauses**, and never
  move code into, out of, or across the region it governs.
- A block **inside** a parallel loop body may be extracted (Recipe E) into a `static`
  function called from the same place, **provided the block writes no variable declared
  outside the loop body** - not directly, and not by passing its address. Reading outer
  variables, which become parameters passed by value, is fine; writing through pointers
  the block already uses for pixel data (`p`, `q`) is fine. Moving a statement into a
  callee does not change which thread runs it; changing which variables it writes would.
- Code under `#pragma omp critical` or `#pragma omp atomic` stays exactly where it is,
  including the progress-monitor blocks at the end of most loops.

### Preprocessor branches

Code inside `#if defined(...)` branches that this build does not compile - X11,
OpenCL, Windows, a delegate library that is not installed - cannot be compiled, run or
measured here. Do not change it, even when the change looks identical to one you made in
a compiled branch. When a recipe would need to, skip the function and report it.

This stands until `tools/oracle/build.sh` can build the X11, OpenCL and Windows code in
one command ([`VERIFICATION.md`](VERIFICATION.md), step 5). Then that code becomes
guard-verified, below, in the build that compiles it.

---

## Two levels of evidence

The oracle only proves what its cases reach and check. Before starting on a function,
find out which level it is at, and say in the commit message which one verified it
(`Verified: oracle` or `Verified: guard`).

**Oracle-verified.** The function passes the readiness gate:

```bash
python3 tools/oracle/casemap.py <Function>              # some case executes it
python3 tools/oracle/mutate.py --file <file> --function <Function> --max-cases 0
python3 tools/oracle/gate.py build-oracle/work/mutation-<name>.json --file <file>.c
```

A verdict of **ready** (90% or more of the killable mutants killed) allows every recipe
in this catalogue. **Careful** (75-90%) allows them too, but read the unmatched and
unreached survivors of the function first (`classify.py ... --kind unmatched`) and keep
clear of the lines they sit on, or add a case that kills them.

**Guard-verified.** The function is **not ready**, or no case executes it. It may still
be refactored, as 3SX refactored a program with no tests at all, but only with the
recipes whose guard fingerprint is exact: **E, G and P**. The proof is the build
and `refactor_guard.py` reporting `OK` with `--calls`; a `WARN` stops the commit.
Recipes D and S fold code together or move it, the guard cannot follow them, and
they need the oracle.

The guard has two blind spots: reordered statements, which the prohibitions already
forbid, and **transposed arguments**. For every call the step adds, check by reading
that each argument is passed in the position of the parameter it fills. Where the
machine-code comparison applies (`VERIFICATION.md`, step 3), identical optimised code is
a proof that covers both.

---

## Growing the catalogue

**New recipe or amendment?** A change to the catalogue is a **new recipe** if any of these
holds:

- (a) the code it applies to has a shape the existing recipe's *Before* does not show;
- (b) it produces a different kind of result than the existing recipe's *After*;
- (c) it targets a Code Health finding the existing recipe's *Use when* does not name;
- (d) it needs its own verification argument, or a different evidence level.

Otherwise it is an **amendment**: a relaxed precondition for the same transformation, a
compiler or platform workaround, or wording. **When in doubt, it is a new recipe.** A
recipe has at most eight numbered steps; a change that would need a ninth is a new recipe.

**Every recipe has these fields:** a code and a name, both chosen by whoever adds the
recipe (the name says what the transformation does, not where it was first needed); *Use when*; *Not covered by* (each nearby
recipe, and why it refuses this shape); *How* (at most eight steps); *Before* and *After*
from this repository; *Careful*; *Evidence level* (`guard` or `oracle`, plus any extra
check); *Provenance* (date, the functions that motivated it, and who added it, with harness
and model). The founding recipes have no
*Not covered by*.

**Adding a recipe.** Any agent may add a recipe when it meets a pattern of findings that
no recipe reaches; the project owner authorised additions on 2026-10-24. Add it in its own
commit (`playbook: add Recipe <code> - <name>`), before the refactor commits that use it,
with a *Catalogue changes* row, and run `python3 tools/playbook_lint.py`. Amendments to an
existing recipe go through the same test and get a row of their own. The same authorisation
covers the three rules: an agent may change one when the evidence shows it blocks correct
work, in its own commit, with a row (Recipe column: `rule 1`, `rule 2` or `rule 3`).

---

## Recipe E - Extract Function

**Use when:** CodeScene reports *Bumpy Road Ahead* (a function with several separate
blocks of nested logic). Each bump is a missing function.

**How:**

1. Find one nested block. It usually already has a comment or a blank line around it.
2. Cut it into a new `static` function directly above the current one.
3. Pass in every variable it reads as a parameter. Return the single value it produces.
4. If the block writes to more than one outer variable, **skip it** and move to the next
   bump. Do not invent an out-parameter struct.
5. If the block is inside an OpenMP loop, check the rule above before you cut.
6. If the block can throw - `ThrowBinaryException`, `ThrowImageException` and friends
   expand to a `return` - the new function must return a status the caller checks, and
   the caller must `return` exactly what the macro returned. If you cannot see what the
   macro expands to, skip the block.

**Before:**

```c
static MagickBooleanType ApplyLevel(Image *image,const double black,
  const double white,ExceptionInfo *exception)
{
  ...
  for (x=0; x < (ssize_t) image->columns; x++)
  {
    if (GetPixelWriteMask(image,q) > (QuantumRange/2))
      {
        for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
        {
          PixelChannel channel = GetPixelChannelChannel(image,i);
          PixelTrait traits = GetPixelChannelTraits(image,channel);
          if ((traits & UpdatePixelTrait) != 0)
            q[i]=ClampToQuantum(LevelPixel(black,white,(double) q[i]));
        }
      }
    q+=(ptrdiff_t) GetPixelChannels(image);
  }
  ...
}
```

**After:**

```c
static void LevelPixelChannels(const Image *image,const double black,
  const double white,Quantum *q)
{
  ssize_t
    i;

  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    PixelChannel channel = GetPixelChannelChannel(image,i);
    PixelTrait traits = GetPixelChannelTraits(image,channel);
    if ((traits & UpdatePixelTrait) != 0)
      q[i]=ClampToQuantum(LevelPixel(black,white,(double) q[i]));
  }
}
...
    if (GetPixelWriteMask(image,q) > (QuantumRange/2))
      LevelPixelChannels(image,black,white,q);
```

Name the new function for **what it does**, in ImageMagick's style: CamelCase, a verb
first, `static`, and declarations at the top of the block as the surrounding code does.

**Evidence level:** guard.

**Provenance:** founding recipe (2026-09-29).

---

## Recipe G - Guard Clauses

**Use when:** CodeScene reports *Deep, Nested Complexity* (nesting depth 4 or more).

**How:** invert the outermost condition and return early - or, inside a loop, `continue`.
Repeat until the main body sits at one level of indentation.

**Before:**

```c
if (image != (Image *) NULL)
  {
    if (image->alpha_trait != UndefinedPixelTrait)
      {
        if (GetImageArtifact(image,"verbose") != (const char *) NULL)
          ReportAlpha(image);
      }
  }
```

**After:**

```c
if (image == (Image *) NULL)
  return;
if (image->alpha_trait == UndefinedPixelTrait)
  return;
if (GetImageArtifact(image,"verbose") == (const char *) NULL)
  return;
ReportAlpha(image);
```

**Careful:**

- If the function returns a value, every guard must return the value the original would
  have produced by falling through. Read the end of the function first. In ImageMagick
  that is often `status`, or an image pointer that must also be destroyed on the way out;
  if the fall-through path frees anything, the guard must too, and then the recipe does
  not apply - skip the function.
- Inverting `a < b` gives `a >= b`, never `a > b`. Inverting `x != (T *) NULL` gives
  `x == (T *) NULL`. Write the inverse out; do not wrap the original in `!(...)` unless
  that is clearer, and never both.
- Inside an OpenMP loop the early exit is `continue`, and it must not skip a
  `#pragma omp critical` or `atomic` block that the original ran on that path.

**Evidence level:** guard.

**Provenance:** founding recipe (2026-09-29).

---

## Recipe P - Named Predicate

**Use when:** CodeScene reports *Complex Conditional*, or *Complex Method* driven by
compound boolean expressions.

**How:** move the expression into a `static` function whose name states the intent,
returning `MagickBooleanType`. This is the safest recipe in the catalogue.

**Before:**

```c
if ((image->alpha_trait != UndefinedPixelTrait) &&
    (image->colorspace != CMYKColorspace) && (image->columns > 1))
  ...
```

**After:**

```c
static inline MagickBooleanType IsBlendableImage(const Image *image)
{
  return(((image->alpha_trait != UndefinedPixelTrait) &&
    (image->colorspace != CMYKColorspace) && (image->columns > 1)) ?
    MagickTrue : MagickFalse);
}
...
if (IsBlendableImage(image) != MagickFalse)
  ...
```

Copy the expression **character for character**. Do not simplify the boolean algebra, and
do not reorder the operands: `&&` short-circuits, so reordering can change which calls run
and can dereference a null pointer the original never touched.

**Evidence level:** guard.

**Provenance:** founding recipe (2026-09-29).

---

## Recipe D - Deduplicate

**Use when:** CodeScene reports *Code Duplication*.

**How:** only when the blocks are **identical or differ by a single value**. Extract one
`static` helper and pass the differing value as a parameter. If they differ in two or
more places, leave them alone - forcing a shared abstraction over near-miss duplicates is
how subtle behaviour gets broken.

The per-colorspace and per-channel blocks in `colorspace.c`, `quantum-import.c` and
`quantum-export.c` often *look* identical and differ in one scale factor or one channel
index. That is exactly one value, and D applies; two, and it does not.

**Evidence level:** oracle.

**Provenance:** founding recipe (2026-09-29).

---

## Recipe S - Split File

**Use when:** CodeScene reports *Lines of Code in a Single File*, after the functions in
it have been simplified.

**This recipe needs the project owner's go-ahead per file.** ImageMagick lists its
sources explicitly in `MagickCore/Makefile.am` (and in the separate Windows build
configuration), so a new file means editing the build system and regenerating
`Makefile.in`. That is outside the other recipes' guarantee, and the owner decides when it
is worth it.

**How:** once the owner has approved the file,

1. Move a group of `static` functions that share a theme, together with the one public
   function that uses them, to a new `.c` file beside the original.
2. No function may change linkage: a `static` stays `static` and must travel with every
   caller. A split that would need one widened is refused.
3. The public function keeps its declaration in the header it already has.

**Evidence level:** oracle.

**Provenance:** founding recipe (2026-09-29).

---


## Known plateaus

None recorded yet. When a file stops improving and no recipe in this catalogue applies,
and you do not add one, record it here: the file, its score, and the smells left.

### Tool notes

- **CodeScene's start line** for a function often includes the comment block above it.
  Use the compiler's view of where a function starts; `tools/make_task.py` does.
- `refactor_guard.py --calls` fails on a renamed function until the rename is declared
  (`--renamed OLD=NEW`); that is legal only for a file-local static, and in practice
  only for a helper this campaign created.

---

## The verification loop

After every commit, as [`AGENTS.md`](../../AGENTS.md) sets out:

1. `tools/oracle/build.sh cand`
2. `python3 tools/refactor_guard.py <file>` (with `--calls` after Recipe E or S;
   always with `--calls` at the guard-verified level)
3. `python3 tools/oracle/oracle.py run --function <Function>` (at the guard-verified
   level too: it cannot prove the step, but it can still catch one)
4. `python3 tools/ch.py --review <file>`

Before pushing a branch: `python3 tools/oracle/oracle.py run`, the whole catalogue.

---

## Catalogue changes

| Date | Type | Recipe | Change | Evidence | Decided by |
| --- | --- | --- | --- | --- | --- |
| 2026-10-24 | WORDING | all | Reset to the founding catalogue before any refactoring on `main`: the five founding recipes, the founding wording of rules 1 and 2 and of Recipe E step 4; C, X, A and R, the flat-score section, and what the pilot on `night1-refactoring` taught (practical points, the parallel-region amendment) removed | The comparison with the earlier campaigns needs the same starting point | owner |
| 2026-10-24 | AMEND | rule 2 | A step that leaves the score flat is kept when the review shows progress on the targeted function and the commit names the finding and the next step | A file score moves only once a function drops below a finding's threshold, so the founding rule reverts correct steps towards it; the earlier campaign relaxed the same rule on its first morning | owner |

Type is `NEW`, `AMEND` or `WORDING`. *Decided by* is `owner`, `mandate (<date>)` or
`after the fact`.
