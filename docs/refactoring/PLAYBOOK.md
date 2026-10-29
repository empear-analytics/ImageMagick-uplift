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
   One exception (owner's decision, 2026-10-28): when the same recipe applied to each of
   several functions on its own lowers the score (a file-size threshold, say) and the
   pre-commit safeguard refuses it, but the set together raises it, the set may go in
   one commit. Each function's change must still be the recipe's mechanical step, and
   the commit message lists them.
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
- Recipe T is the one exception to that proviso: a block whose only outer write is
  `status=MagickFalse;` may move into a function that returns the status, which the
  caller then writes.
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

**Oracle-verified.** Some case executes the function:

```bash
python3 tools/oracle/casemap.py <Function>              # some case executes it
```

That allows every recipe in this catalogue. **No mutation run is needed before a step**
(owner's decision, 2026-10-27): the harness was built under mutation guidance, and its
cases are the protection now. The mutation reports already in `build-oracle/work/` may
still be read, for instance to see which lines of a function no case checks, but do not
start new runs (`mutate.py`, Mull builds, `night3.sh`) to gate a step.

**Guard-verified.** No case executes the function. It may still
be refactored, as an earlier campaign refactored a program with no tests at all, but only with the
recipes whose guard fingerprint is exact: **E, G and P**. The proof is the build
and `refactor_guard.py` reporting `OK` with `--calls`; a `WARN` stops the commit.
For E and P, which add a function, `OK` means the exact fingerprint the guard names as
expected: the literal check `OK`, and `--calls` moving nothing but `+2` for each new
helper (its definition and its call) before its `PASS` line. Any other count that
moves is the `WARN` that stops the commit.
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
   When that value is an array of the caller's that the block updates element by
   element (an accumulator, `mean[i]+=...`), pass the array as a pointer parameter
   whose uses do not change, as Recipe K does; the OpenMP rule below still applies.
4. If the block writes to more than one outer variable, **skip it** and move to the next
   bump. Do not invent an out-parameter struct.
5. If the block is inside an OpenMP loop, check the rule above before you cut.
6. If the block can throw - `ThrowBinaryException`, `ThrowImageException` and friends
   expand to a `return` - the new function must return a status the caller checks, and
   the caller must `return` exactly what the macro returned. If you cannot see what the
   macro expands to, skip the block.
7. A block that runs to the end of a `void` function may contain plain `return;`
   statements: they become returns of the new `void` function, and its call is the
   caller's last statement, so both end the caller's work at the same point. Macros
   used only by the block (`#define` ... `#undef`) move with it. Likewise, a block of a
   value-returning function whose every path ends in `return(<expr>);` moves to a
   function of the caller's return type, and the block becomes `return(<call>);`.
   Likewise, a block that is a loop's whole body, declarations included, may contain
   `continue;` statements of that loop: they become `return;` of a new `void`
   function, and the body becomes the call alone, so both end the iteration at the
   same point. The block contains no `break` or `return` of its own, and no `continue`
   of a loop nested inside it changes; the loop's counter goes by value.
   Likewise, inside a `switch` case, a block after which the case runs nothing but its
   closing `break;` on every path (the case's whole body, or the last statement of a
   branch that ends the case) may contain `break;` statements of that `switch`, when
   its single output is one variable `v`: the new function takes `v` by value and
   returns it, each such `break;` becomes `return(v);`, the function ends with
   `return(v);`, and the block becomes `v=Helper(...);`. A path that left `v` alone
   returns the caller's value. The block contains no `continue`, `return` or `goto`,
   and a `break` of a loop or `switch` nested inside it does not change.

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
- **Except for floating point.** When either operand is a `double`, `float` or
  `MagickRealType`, `a < b` and `a >= b` are both false for a NaN, so `a >= b` is not
  the inverse: write `!(a < b)`. ImageMagick compares doubles against user-supplied
  limits that can be NaN.
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

**Several sites.** One predicate may replace the same expression at more than one site.
That covers a new predicate written for several copies, and an existing predicate taking
a further site. Three conditions apply:
- At every site the expression must equal the predicate's body with the arguments
  substituted for the parameters, character for character. Check this by script, not by
  eye.
- Every argument must be an identifier, a member access or a literal: nothing with a side
  effect, and nothing evaluated a different number of times than before.
- `refactor_guard.py --calls` must show only the expected counts: +2 for a new predicate
  plus +1 for each further site, and, for each copy replaced beyond the first, the callees
  inside the expression one fewer.

The step stays at guard level: it moves expressions, and the script check stands in for
reading each copy.

**Evidence level:** guard.

**Provenance:** founding recipe (2026-09-29). *Several sites:* 2026-10-26. It arose from
the question in PR #16 (segment.c, a guard-level reuse showing +1 in `--calls`) and from
delegate.c's `GetDelegateList`, whose decode and encode tests are the same expression on
two fields.

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

Variables count as the helper's parameters, not as differences: two blocks that are the
same text once the variables each reads or writes are renamed consistently (block A's
`channel_norm` wherever block B has `channel_variance`) are identical, and the helper
takes those variables as arguments. Check it by script: rename, then compare the text.
Literals and operators are not renamed; one that differs is a differing value.

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
4. List the new file wherever the original is listed: `tools/register_source.py` does it
   for `MagickCore/Makefile.am`, `Makefile.in` (by hand, mirroring what automake
   generates), `MagickCore/Make.com` and `scripts/format_c_api_docs`.
   `tools/split_source.py` does steps 1 and 2 and refuses a family that would need a
   linkage change or a file-level macro on both sides; `--out` scores a split first.

**Careful:**

- The pre-commit gate requires a new file to score 10.0, which a split-off family never
  does: it carries the original's findings with it. Commit after the gate has reported
  only that failure (its check that the original does not lose health has then passed),
  with `--no-verify`, run `tools/check_trailers.py` on the message by hand, and say so in
  the message (the owner approved this, 2026-10-28). The Windows build configuration outside this repository still needs the
  file; say that too.

**Evidence level:** oracle, with `refactor_guard.py --combined` and `--combined --calls`
over the pair.

**Provenance:** founding recipe (2026-09-29).

---

## Recipe O - Extract Function with Out-Parameters

**Use when:** CodeScene reports *Complex Method*, *Large Method* or *Bumpy Road Ahead*,
and the block that would fix it is a self-contained selection (a `switch` or an `if`
chain) that sets several of the caller's local variables together, such as a method
pointer with its magnification and window width.

**Not covered by:** Recipe E, whose step 4 refuses a block that writes more than one
outer variable; Recipe P, which moves an expression, not statements; Recipe D, which
needs two copies of the block.

**How:**

1. The block must lie outside every OpenMP region: not in a parallel loop, not under
   `#pragma omp single`, `critical` or `atomic`. Inside a parallel loop body it may
   move only when every variable it writes is declared inside that body (so each
   thread has its own), it writes nothing declared outside it, `status` included, and
   it contains no `#pragma omp` line: the condition Recipe K rests on. Otherwise use
   Recipe E or skip it.
2. List the caller's variables the block writes. Each must be a plain local of the
   caller (not a parameter, not a global, not a `static`) whose address is not taken
   elsewhere, so no two out-parameters can name the same object.
3. The block must contain no `return`, `goto`, `continue`, or `break` that leaves the
   block; a `break` that ends a `switch` or loop inside the block is fine.
4. Cut the block into a new `static void` function directly above the caller. Pass each
   variable it only reads by value, and each variable it writes as a pointer.
5. In the body, write `*v` for every use of a written variable `v`, reads included, so
   a value the block does not overwrite is the caller's value. Change nothing else.
6. Call it where the block stood, passing `&v` for each written variable, in the order
   of the parameters. Check every argument against its parameter by reading.

**Before:**

```c
  scaling_method=Scale2X;
  magnification=2;
  width=3;
  switch (*option)
  {
    case 'e':
    {
      if (LocaleCompare(option,"eagle2x") == 0)
        {
          scaling_method=Eagle2X;
          magnification=2;
          width=3;
          break;
        }
      ...
  }
  assert((magnification*magnification) <= MaxMagnification);
```

**After:**

```c
static void SelectMagnifyMethod(const char *option,
  void (**scaling_method)(const Image *,const Quantum *,Quantum *,size_t),
  size_t *magnification,size_t *width)
{
  *scaling_method=Scale2X;
  *magnification=2;
  *width=3;
  switch (*option)
  {
    ...
  }
}
...
  SelectMagnifyMethod(option,&scaling_method,&magnification,&width);
  assert((magnification*magnification) <= MaxMagnification);
```

**Careful:**

- Every write in the original stays a write in the callee and nothing more: a variable
  written on only some paths keeps the caller's value on the others, which is why reads
  go through the pointer too.
- Keep out-parameters to the few variables the block decides; a block that writes more
  than four is not a selection, and is skipped.
- The function must be oracle-verified, with no unmatched or unreached survivor on the
  block's lines: the guard sees the moved literals but not a write that went missing.

**Evidence level:** oracle, with `refactor_guard.py --calls`.

**Provenance:** 2026-10-24, from `MagnifyImage` (its method `switch`), with the same
shape in `AcquireResizeFilter` (filter and window parsing, the B and C overrides) and
`ScaleImage` (row bookkeeping), all blocked by Recipe E's step 4. Added by Claude Code
(Claude Opus 5.5) during task M01.

---


## Recipe T - Extract Function Returning the Loop Status

**Use when:** CodeScene reports *Bumpy Road Ahead*, *Deep, Nested Complexity*, *Large
Method* or *Complex Method*, and the block that would fix it sits inside the body of a
`#pragma omp parallel for` loop and writes the loop's shared `status`, as most pixel
loops do (`status=MagickFalse;` when a cache read fails).

**Not covered by:** Recipe E, whose OpenMP rule refuses a block that writes a variable
declared outside the loop body, and `status` is one; Recipe O, which refuses every block
inside an OpenMP region; Recipe G, which removes one level of nesting, not the block.

**How:**

1. The block lies inside one iteration of the parallel loop body and contains no
   `#pragma omp` line, so no `critical` or `atomic` block moves.
2. The only variable declared outside the loop body that the block writes is `status`,
   and only as `status=MagickFalse;`. The block does not read `status`.
3. Every variable of the loop body that the block writes is declared inside the block,
   or is not read after it (a row pointer `q` advanced along the row, say).
4. The block contains no `return`, `goto`, or a `continue` or `break` of the parallel
   loop itself; a `continue` or `break` of a loop inside the block is fine.
5. Cut the block into a new `static MagickBooleanType` function directly above the
   caller. It declares a local `status`, set to `MagickTrue` first; every
   `status=MagickFalse;` in the block stays as it is; the function ends with
   `return(status);`.
6. Pass every other variable the block reads by value, pointers included (`q`, the cache
   views, `exception`), the same pointers the block already used.
7. Where the block stood, write
   `if (Helper(...) == MagickFalse) status=MagickFalse;`, and check every argument
   against its parameter by reading.

**Before:**

```c
  for (y=0; y < (ssize_t) source_image->rows; y++)
  {
    ...
    for (x=0; x < (ssize_t) source_image->columns; x++)
    {
      ...
      p=GetCacheViewVirtualPixels(image_view,x-width/2,y-width/2,width,width,
        exception);
      if (p == (Quantum *) NULL)
        {
          status=MagickFalse;
          continue;
        }
      ...
      q+=(ptrdiff_t) magnification*GetPixelChannels(magnify_image);
    }
    if (SyncCacheViewAuthenticPixels(magnify_view,exception) == MagickFalse)
      status=MagickFalse;
```

**After:**

```c
static MagickBooleanType MagnifyImageRow(...,Quantum *q,
  ExceptionInfo *exception)
{
  MagickBooleanType
    status;

  ssize_t
    x;

  status=MagickTrue;
  for (x=0; x < (ssize_t) source_image->columns; x++)
  {
    ...
  }
  return(status);
}
...
    if (MagnifyImageRow(...,q,exception) == MagickFalse)
      status=MagickFalse;
    if (SyncCacheViewAuthenticPixels(magnify_view,exception) == MagickFalse)
      status=MagickFalse;
```

**Careful:**

- The one thing that changes is *when* the iteration's write to the shared `status`
  happens: at the end of the block instead of in its middle. Step 2 makes that
  invisible to this thread, which does not read `status` in between, and OpenMP gives
  other threads no ordering for this unsynchronised write in the first place: the same
  value is written, by the same iteration, before the same later reads.
- A block that *tests* `status` (`if (status == MagickFalse) break;`), or assigns it
  anything but `MagickFalse` (`status=InterpolatePixelChannels(...)`), is not this
  recipe: the callee's local `status` would decide it, not the shared one. Skip it. A
  `break` of an inner loop straight after `status=MagickFalse;` is fine: it does not
  depend on the shared value.
- Variables of the loop body passed by value are copies: step 3 is what makes a write
  to the copy, such as advancing `q`, harmless.

**Evidence level:** oracle, with `refactor_guard.py --calls`, and steps 1 to 4 checked by
reading: the oracle runs single-threaded and cannot see them.

**Provenance:** 2026-10-24, from `MagnifyImage` (its per-row column loop), with the same
shape in `resize.c`'s `SampleImage`, both refused by the OpenMP rule of Recipe E. (Not
`InterpolativeResizeImage`, whose block assigns and tests `status`, nor the two filters,
whose failures `continue` the parallel loop.) Owner's go-ahead the same day. Added by Claude
Code (Claude Opus 5.5) during task M01, on `magnify.c`.

---

## Recipe M - Merge Duplicate Parameter

**Use when:** CodeScene reports *Excess Number of Function Arguments* on a `static`
function and every call passes the same argument to two of its parameters, such as a
comparison of two pixels that are always taken from one buffer.

**Not covered by:** Recipe E, P, O and T, which move code into a new function and leave
existing signatures alone; Recipe D, which merges two copies of a block, not two
parameters; Recipe B, which bundles parameters that carry different values.

**How:**

1. The function is `static` (or `static inline`), declared in this file only, and no
   pointer to it is taken: a direct call is the only way in.
2. Two of its parameters, `p1` and `p2`, have the same type, and no write to either
   appears in the body.
3. At **every** call, the arguments for `p1` and `p2` are the same identifier, a
   parameter or local the call does not modify, so both evaluate to one value. Not a
   function call, not `*p++`, not an expression with side effects.
4. Remove `p2` from the parameter list and write `p1` for every use of `p2` in the body.
   Change nothing else.
5. Remove the matching argument from every call, macros that expand to a call included.
6. Build: a call left with the old number of arguments does not compile.

**Before:**

```c
static inline int PixelsEqual(const Quantum *source1,ssize_t offset1,
  const Quantum *source2,ssize_t offset2,const size_t channels)
{
  ...
    if (source1[offset1+i] != source2[offset2+i])
  ...
}
...
  if (PixelsEqual(pixels,1,pixels,7,channels) ||
      PixelsEqual(pixels,3,pixels,5,channels))
```

**After:**

```c
static inline int PixelsEqual(const Quantum *source,ssize_t offset1,
  ssize_t offset2,const size_t channels)
{
  ...
    if (source[offset1+i] != source[offset2+i])
  ...
}
...
  if (PixelsEqual(pixels,1,7,channels) ||
      PixelsEqual(pixels,3,5,channels))
```

**Careful:**

- List every call before you start (`grep -n` the name across the source tree); a single
  call that passes two different buffers ends the recipe.
- Two arguments that look alike can differ: `p` and `p` across a loop that advances `p`
  are the same identifier at the same call, which is what matters, but `p` and `q` that
  happen to point to the same place today are not.

**Evidence level:** oracle, with `refactor_guard.py --calls`, and step 3 checked by
reading every call.

**Provenance:** 2026-10-24, from `magnify.c`'s `PixelsEqual`, whose 84 calls all pass one
buffer twice; part of the owner's go-ahead the same day for removing *Excess Number of
Function Arguments* from the magnify kernels. Added by Claude Code (Claude Opus 5.5)
during task M01, on `magnify.c`.

---

## Recipe B - Bundle Arguments into a Struct

**Use when:** CodeScene reports *Excess Number of Function Arguments* on a `static`
function, and some of its parameters travel together: every call in a caller passes the
same unchanged values for them (a source buffer, a result buffer and a channel count),
or together they name one thing (the six neighbour indices of a pixel rule).

**Not covered by:** Recipe M, which needs two parameters that receive one value; Recipes
E, O and T, which create functions and leave existing signatures alone; Recipe D, which
merges code, not parameters.

**How:**

1. The function is `static`, declared in this file only, and never used through a
   function pointer: its signature is the file's own business.
2. Declare a `typedef struct _Name { ... } Name;` under *Typedef declarations* near the
   top of the file, one member per parameter in the group, with the parameter's type and
   name.
3. Replace the group with one parameter `const Name *name`, and in the body write
   `name->member` for each use, or declare a `const` local of the old name initialised
   from the member at the top of the body. Change nothing else.
4. In each caller, a `Name` local holds the exact argument expressions, member by
   member in declaration order. Each expression must be an identifier, a literal, or
   arithmetic on these (`1.0/scale`), not a call, a dereference or anything with a side
   effect. Member accesses (`shear.x`, `image->rows`) and casts of these forms are
   allowed too, but only in a struct filled directly before the one call that uses it,
   with no statement between.
5. Declare the local among the declarations at the top of the block that holds its
   calls (the loop body, for a struct per iteration), never after a statement: the
   macOS CI job compiles with `-Wdeclaration-after-statement -Werror`. Nothing may
   write an identifier of the initialiser between the point where the struct takes its
   values and its last call. Where that point can be the declaration, and the
   initialiser has no member access or cast, declare it `const` with the initialiser.
   Otherwise declare it without `const` or initialiser, and fill it at that point,
   `name.member=expr;` for every member in declaration order, with nothing between.
6. A struct declared outside a `#pragma omp` region and read inside it is `const`, and
   none of its members point to data the region writes through the struct.
7. At each call, replace the group with `&name`, and check by reading that every member
   is initialised from the argument it replaces.
8. Build: a call left with the old arguments does not compile.

**Before:**

```c
static inline void CopyPixels(const Quantum *source,const ssize_t source_offset,
  Quantum *destination,const ssize_t destination_offset,const size_t channels)
{
  ...
    destination[(ssize_t) channels*destination_offset+i]=
      source[source_offset*(ssize_t) channels+i];
}
...
  for (i=0; i < 4; i++)
    CopyPixels(pixels,4,result,i,channels);
```

**After:**

```c
typedef struct _MagnifyPixels
{
  const Quantum
    *source;

  Quantum
    *destination;

  size_t
    channels;
} MagnifyPixels;
...
static inline void CopyPixels(const MagnifyPixels *magnify,
  const ssize_t source_offset,const ssize_t destination_offset)
{
  ...
    magnify->destination[(ssize_t) magnify->channels*destination_offset+i]=
      magnify->source[source_offset*(ssize_t) magnify->channels+i];
}
...
  const MagnifyPixels
    magnify = { pixels, result, channels };
  ...
  for (i=0; i < 4; i++)
    CopyPixels(&magnify,4,i);
```

**Careful:**

- A member keeps the parameter's exact type: `const` included, no `size_t` for a
  `ssize_t`. The casts in the body stay where they were, on the member now.
- The struct holds the values the arguments had at its declaration. Step 5 is what makes
  them the values each call would have passed; a pointer advanced along a row between
  calls (`q+=...`) cannot be a member.
- Pointer members point to the same buffers the arguments did, so writes through them
  land where they always did.
- A member access or cast is read at the declaration instead of in the call. Declared
  directly before its one call, nothing can run between the two, not even a call that
  writes the object through another pointer, so the value is the one the call would
  have read. The expression has no call and no side effect, so evaluating it first
  changes nothing, whatever order the call's arguments were evaluated in.
- A struct filled member by member holds what the `const` initialiser would have: the
  same expressions, evaluated in the same order, directly before the call. Nothing
  checks that every member is assigned (an unassigned one is read uninitialised), so
  count them against the typedef. Not `const`, it may not be read inside an OpenMP
  region (step 6).
- An arithmetic member (`1.0/scale`) is computed once, at the declaration, instead of at
  every call. Step 5 keeps its operands unchanged until the last call, so each call
  receives the value it computed before; the member has the parameter's type, so the
  value is rounded to it exactly as the argument was.

**Evidence level:** oracle, with `refactor_guard.py --calls` (no call may appear or
vanish), and steps 4 to 7 checked by reading.

**Provenance:** 2026-10-24, from `magnify.c`'s pixel kernels (`CopyPixels`, `MixPixels`,
`Mix2Pixels`: 5 or 6 arguments, three of them the same at every call in a kernel),
`Hq2XHelper` (11 arguments) and `CopyMagnifiedPixels`, with the owner's go-ahead the
same day. Added by Claude Code (Claude Opus 5.5) during task M01, on `magnify.c`.

---

## Recipe K - Extract Accumulating Block with Out-Parameters

**Use when:** CodeScene reports *Deep, Nested Complexity*, *Bumpy Road Ahead*, *Complex
Method* or *Large Method*, and the block that would fix it is a per-pixel accumulation
inside a parallel loop body that adds into two to four of the iteration's own
accumulators (`channel_similarity` and `channel_area`, or three sums).

**Not covered by:** Recipe E, whose step 4 refuses a block that writes more than one
variable; Recipe O, which takes a selection (a `switch` or `if` chain) and only outside
every OpenMP region; Recipe T, whose block writes the shared `status`, not private
accumulators.

**How:**

1. The block contains no `#pragma omp` line, no `return`, `goto`, and no `continue` or
   `break` that leaves it.
2. Every variable the block writes is declared inside the parallel loop body (so each
   thread has its own), and there are at most four of them. Writes through pointers the
   block already uses for pixel data stay as they are. The block writes nothing
   declared outside the loop body, `status` included.
3. Variables the block declares and uses alone move with it; one initialised in its
   declaration moves with its initialiser only if it is declared in the innermost loop
   that runs the block, so it is reset as often as before.
4. Cut the block into a new `static void` function directly above the caller. Pass each
   variable it only reads by value; each array it writes as a pointer (its uses do not
   change); each scalar it writes as a pointer, written `*v` (or `(*v)++`) at every use,
   reads included.
5. Call it where the block stood, passing `&v` for each written scalar and the array
   for each written array, in parameter order; check every argument by reading.

**Before:**

```c
    for (x=0; x < (ssize_t) columns; x++)
    {
      ...
      Sa=QuantumScale*(double) GetPixelAlpha(image,p);
      Da=QuantumScale*(double) GetPixelAlpha(reconstruct_image,q);
      for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
      {
        ...
        if (MagickSafeSignificantError(error*error,fuzz) != MagickFalse)
          {
            channel_similarity[i]+=QuantumScale*error*QuantumScale*error;
            ...
            channel_area++;
          }
      }
```

**After:**

```c
static void AccumulateFUZZPixel(...,double *channel_similarity,
  double *channel_area)
{
  ...
        if (MagickSafeSignificantError(error*error,fuzz) != MagickFalse)
          {
            channel_similarity[i]+=QuantumScale*error*QuantumScale*error;
            ...
            (*channel_area)++;
          }
}
...
      AccumulateFUZZPixel(...,channel_similarity,&channel_area);
```

**Careful:**

- The variables written are each thread's own, so the parallel loop shares nothing new;
  the OpenMP section's rule, which forbids passing the address of a variable declared
  *outside* the loop body, is untouched.
- `(*v)++` and `*v+=x` are the same operations on the same object as `v++` and `v+=x`:
  the order of floating-point additions does not change.
- Keep the written variables few; a block that writes more than four is not one
  accumulation, and is skipped.

**Evidence level:** oracle, with `refactor_guard.py --calls`, and steps 1 to 3 checked by
reading.

**Provenance:** 2026-10-24, from `compare.c`'s `GetFUZZSimilarity` (`channel_similarity`
and `channel_area`), `GetMEPPSimilarity` (mean and maximum error), `GetDPCSimilarity`
and `GetNCCSimilarity` (three sums each), whose per-pixel blocks Recipe E's step 4
refused after the single-accumulator siblings (`GetAESimilarity`, `GetMAESimilarity`,
`GetMSESimilarity`, `GetPASimilarity`, `GetPDCSimilarity`) had taken Recipe E. Added by
Claude Code (Claude Opus 5.5) during task M02, under the mandate of 2026-10-24.

---

## Recipe J - Extract Accumulating Block Returning the Loop Status

**Use when:** CodeScene reports *Bumpy Road Ahead*, *Deep, Nested Complexity*, *Complex
Method*, *Large Method* or *Code Duplication*, and the block that would fix it sits in a
parallel loop body, adds into two to four of the iteration's own accumulators, and also
writes the loop's shared `status` (`status=MagickFalse;` when a cache read fails).

**Not covered by:** Recipe T, which passes the block's other variables by value and so
cannot return accumulators; Recipe K, whose step 2 refuses a block that writes `status`;
Recipe E, whose OpenMP rule refuses both.

**How:**

1. The block contains no `#pragma omp` line, no `return` or `goto`, and no `continue` or
   `break` that leaves it.
2. The only variable declared outside the loop body that the block writes is `status`,
   and only as `status=MagickFalse;`. The block does not read `status`.
3. The block's accumulators are variables declared in the loop body, at most four, each
   read after the block. Every other loop-body variable it writes is declared in the
   block or is not read after it before being written again (a pixel pointer `p`, the
   counters `x` and `y`): those are declared in the new function instead.
4. Cut the block into a new `static MagickBooleanType` function directly above the
   caller. It declares a local `status`, set to `MagickTrue` first; every
   `status=MagickFalse;` stays as it is; the function ends with `return(status);`.
5. Pass every variable the block only reads by value, pointers included. Pass each
   scalar accumulator as a pointer, written `*v` (or `(*v)++`) at every use in the
   block, and each array accumulator as a pointer whose uses do not change, as in
   Recipe K.
6. Where the block stood, write `if (Helper(...,&v,...) == MagickFalse)
   status=MagickFalse;`, and check every argument against its parameter by reading.

**Before:**

```c
    bounding_box=object[i].bounding_box;
    for (y=0; y < (ssize_t) bounding_box.height; y++)
    {
      p=GetCacheViewVirtualPixels(component_view,bounding_box.x,
        bounding_box.y+y,bounding_box.width,1,exception);
      if (p == (const Quantum *) NULL)
        {
          status=MagickFalse;
          break;
        }
      for (x=0; x < (ssize_t) bounding_box.width; x++)
      {
        if ((ssize_t) GetPixelIndex(component_image,p) == i)
          {
            M00++;
            M10+=x;
            M01+=y;
          }
        p+=(ptrdiff_t) GetPixelChannels(component_image);
      }
    }
    centroid.x=M10*MagickSafeReciprocal(M00);
```

**After:**

```c
static MagickBooleanType GetObjectCentroidSums(const Image *component_image,
  CacheView *component_view,const RectangleInfo bounding_box,const ssize_t i,
  double *M00,double *M10,double *M01,ExceptionInfo *exception)
{
  ...
  status=MagickTrue;
  for (y=0; y < (ssize_t) bounding_box.height; y++)
  {
    ...
        {
          status=MagickFalse;
          break;
        }
    ...
          (*M00)++;
          *M10+=x;
          *M01+=y;
    ...
  }
  return(status);
}
...
    if (GetObjectCentroidSums(component_image,component_view,bounding_box,i,
          &M00,&M10,&M01,exception) == MagickFalse)
      status=MagickFalse;
```

**Careful:**

- The write to the shared `status` moves to the end of the block, as in Recipe T, whose
  argument holds unchanged: the block does not read `status`, and OpenMP orders this
  unsynchronised write against no other thread.
- The accumulators are each iteration's own, as in Recipe K; `(*v)++` and `*v+=x` are the
  same operations on the same objects, in the same order.
- A `break` of a loop inside the block after `status=MagickFalse;` stays inside the new
  function and ends the same loop. A block that tests `status`, such as a second pass
  starting `if (status == MagickFalse) continue;`, is not this recipe.
- Keep it to accumulations that are worth it: on `MotionBlurImage`'s kernel loops, where
  the helper needs nine to eleven arguments, the plain loop gained 0.02 and the blended
  one lost 0.02, and the Brain Method stayed; none was committed.

**Evidence level:** oracle, with `refactor_guard.py --calls`, and steps 1 to 3 checked by
reading: the oracle runs single-threaded.

**Provenance:** 2026-10-28, from `vision.c`'s `MajorAxisThreshold`, whose first moment
pass (`M00`, `M10`, `M01`) recurs in `MinorAxisThreshold`, `EccentricityThreshold` and
`AngleThreshold`; tried first on `MotionBlurImage`. The owner's "you can try 3" of the same
day, on blocks that write `status` and accumulators. Added by Claude Code (Claude Opus 5.5)
during task M101.

---

## Recipe N - Extract Function Taking an Input Struct

**Use when:** CodeScene reports *Deep, Nested Complexity*, *Bumpy Road Ahead*, *Complex
Method* or *Large Method*, and the block that would fix it meets Recipe E in every respect
but one: it reads five or more of the caller's variables, so the new function would carry
*Excess Number of Function Arguments* (and the pre-commit gate refuses a new function with
a finding).

**Not covered by:** Recipe E, whose new function would take every input as a parameter;
Recipe B, which bundles the parameters of a function that already exists, so E then B is
two commits of which the first adds a finding and lowers the score; Recipes O, K, T and J,
which pass the block's *written* variables by pointer and leave its inputs as parameters.

**How:**

1. The block meets Recipe E steps 1 and 3 to 7, and the OpenMP rule above. It contains
   no `#pragma omp` line.
2. List the variables the block only reads. Choose a group of them that are scalars or
   pointers, none written anywhere in the block, so the new function keeps at most four
   parameters: the struct and what the block writes through (a pixel buffer, say).
3. Declare `typedef struct _Name { ... } Name;` directly above the new function, one
   member per variable in the group, with the variable's exact type and name: no `size_t`
   for a `ssize_t`, no `const` added or dropped on what a pointer points to. Only a
   top-level `const` of a parameter (`const double softness`) is left off the member,
   which step 6 assigns.
4. Cut the block into a new `static` function taking `const Name *name` in place of the
   group. At the top of its body declare one `const` local per member, of the member's
   type and name, initialised from the member; the block's text below is unchanged.
5. In the caller, declare a `Name` local among the declarations at the top of the block
   that holds the call (the loop body, for a call inside a loop), never after a
   statement.
6. Directly before the call, assign every member from the variable of the same name,
   `name.member=member;`, in declaration order, with nothing between the assignments and
   the call. Then call the function with `&name` where the block stood.
7. Inside an OpenMP parallel loop, the `Name` local is declared inside the loop body, so
   each thread fills its own.
8. Check by reading that every member is assigned, from the variable of its name, and
   that every other argument fills the parameter it names.

**Before:**

```c
      magnitude=threshold*(double) noise_levels[level];
      for (i=0; i < (ssize_t) number_pixels; ++i)
      {
        pixels[(ssize_t) high_pass+i]-=pixels[(ssize_t) low_pass+i];
        if ((double) pixels[(ssize_t) high_pass+i] < -magnitude)
          pixels[(ssize_t) high_pass+i]+=(float) (magnitude-softness*magnitude);
        else
          ...
        if (high_pass != 0)
          pixels[i]+=pixels[(ssize_t) high_pass+i];
      }
      high_pass=low_pass;
```

**After:**

```c
typedef struct _WaveletThreshold
{
  size_t
    high_pass,
    low_pass;

  MagickSizeType
    number_pixels;

  double
    magnitude,
    softness;
} WaveletThreshold;

static void ThresholdWaveletLevel(float *pixels,
  const WaveletThreshold *wavelet)
{
  const size_t
    high_pass = wavelet->high_pass,
    low_pass = wavelet->low_pass;
  ...
  for (i=0; i < (ssize_t) number_pixels; ++i)
  {
    pixels[(ssize_t) high_pass+i]-=pixels[(ssize_t) low_pass+i];
    ...
  }
}
...
      magnitude=threshold*(double) noise_levels[level];
      wavelet.high_pass=high_pass;
      wavelet.low_pass=low_pass;
      wavelet.number_pixels=number_pixels;
      wavelet.magnitude=magnitude;
      wavelet.softness=softness;
      ThresholdWaveletLevel(pixels,&wavelet);
      high_pass=low_pass;
```

**Careful:**

- The members are read in the caller, directly before the call, from variables the block
  does not write: each holds the value the block would have read at its first use, and
  the `const` locals hold it for the whole block, as the block's own reads of an
  unwritten variable did. A variable the block writes is never a member.
- A member keeps the variable's exact type, so the assignment and the local's
  initialisation convert nothing; under HDRI a `MagickRealType` member stays one.
- Every member is assigned before the call: an unassigned member is read uninitialised,
  and nothing checks it but reading (step 8). Count them against the typedef.
- Recipe E's limit on what the block writes stands: one value returned, or writes
  through a pointer the block already used for pixel data. A block writing more is
  skipped, as in Recipe E.

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for the new function,
nothing else moved), and steps 1, 2 and 8 checked by reading.

**Provenance:** 2026-10-29, from `visual-effects.c`'s `WaveletDenoiseImage`, whose last
depth-4 site is the coefficient threshold loop (six inputs: the buffer, both offsets, the
pixel count, the magnitude and the softness). The Mac campaign tried it with Recipe E
(2.41 to 2.36 on the new function's *Excess Arguments*, M05) and refused `SteganoImage`'s
bit switch for the same reason; `MotionBlurImage`'s kernel loops (9 to 11 inputs, M101)
are the same shape. A probe of the struct form: the depth-4 finding leaves, 2.84 to 2.90.
Added by Claude Code (Claude Opus 5.5) during task W07, under the mandate of 2026-10-24.

---

## Recipe Q - Extract Accumulating Block Taking an Input Struct

**Use when:** CodeScene reports *Deep, Nested Complexity*, *Bumpy Road Ahead*, *Brain
Method*, *Complex Method* or *Large Method*, and the block that would fix it is a
per-pixel accumulation inside a parallel loop body that meets Recipe K in every respect
but one: with its inputs as parameters, the new function would take five or more
arguments (*Excess Number of Function Arguments*, which the pre-commit gate refuses on a
new function). The typical case is a filter's per-channel weighted sum (`pixel`, `gamma`
and a cursor), reading the window pointer, the window size, the weights and the channel.

**Not covered by:** Recipe K, which passes every input as a parameter; Recipe N, whose
block may write only one value or through a pixel pointer, and which keeps written
variables out of its struct; Recipe J, which also writes `status` and passes its inputs
as parameters. K then B is two commits, of which the first adds a finding.

**How:**

1. The block meets Recipe K steps 1 to 3: no `#pragma omp` line, no `return`, `goto`, or
   `continue`/`break` that leaves it; it writes at most four of the parallel loop body's
   own variables, and nothing declared outside that body.
2. Split what the block touches in two: the variables it writes (the accumulators and
   any cursor it advances, such as `n`), and the variables it only reads. Variables it
   declares and uses alone move with it, as in Recipe K step 3.
3. Put the read-only variables, scalars and pointers, in a struct as Recipe N steps 2
   and 3 do: one member per variable, the variable's exact type and name, a top-level
   `const` or `magick_restrict` left off. A written variable is never a member.
4. Cut the block into a new `static void` function taking `const Name *name` first, then
   one pointer per written scalar, in the order the block first writes them. At the
   top of its body declare one `const` local per member, as Recipe N step 4 does.
5. In the block's text, write each written scalar `v` as `*v` (`(*v)++` for `v++`) at
   every use, reads included, as Recipe K step 4 does; nothing else in the text changes.
6. In the caller, declare the `Name` local at the top of the block holding the call (in
   the parallel loop body, so each thread fills its own), and directly before the call
   assign every member from the variable of its name, in declaration order, with nothing
   between the assignments and the call.
7. Call it with `&name` and `&v` for each written scalar, at most four parameters in
   all; check every member and every argument by reading.

**Before:**

```c
        pixel=0.0;
        gamma=0.0;
        n=0;
        if ((blur_traits & BlendPixelTrait) == 0)
          {
            for (v=0; v < (ssize_t) MagickMax(height,1); v++)
            {
              for (u=0; u < (ssize_t) MagickMax(width,1); u++)
              {
                 r=p-(ssize_t) (GetPixelChannels(image)*MagickMax(width,1)*
                  (size_t) (mid.y-v)+GetPixelChannels(image)*(size_t)
                  (mid.x-u));
                pixel+=weights[id][n]*(double) r[i];
                gamma+=weights[id][n];
                n++;
              }
            }
            SetPixelChannel(blur_image,channel,ClampToQuantum(
              MagickSafeReciprocal(gamma)*pixel),q);
            continue;
          }
```

**After:**

```c
static void SumBilateralPixels(const BilateralSum *sum,double *pixel,
  double *gamma,ssize_t *n)
{
  const Image
    *const image = sum->image;
  ...
  for (v=0; v < (ssize_t) MagickMax(height,1); v++)
  {
    for (u=0; u < (ssize_t) MagickMax(width,1); u++)
    {
       r=p-(ssize_t) (GetPixelChannels(image)*MagickMax(width,1)*
        (size_t) (mid.y-v)+GetPixelChannels(image)*(size_t)
        (mid.x-u));
      *pixel+=weights[id][*n]*(double) r[i];
      *gamma+=weights[id][*n];
      (*n)++;
    }
  }
}
...
        if ((blur_traits & BlendPixelTrait) == 0)
          {
            sum.image=image;
            sum.p=p;
            ...
            sum.i=i;
            SumBilateralPixels(&sum,&pixel,&gamma,&n);
            SetPixelChannel(blur_image,channel,ClampToQuantum(
              MagickSafeReciprocal(gamma)*pixel),q);
            continue;
          }
```

**Careful:**

- The two halves keep the arguments of the recipes they come from. The written
  variables are each thread's own and change through `*v`, the same operations on the
  same objects in the same order (Recipe K); the read-only ones are copied directly
  before the call from variables the block does not write, so each holds the value the
  block would have read (Recipe N).
- A cursor the block reads before writing (`n`, initialised before the block) is a
  written variable, passed by pointer, never a member: its value at the call is not one
  the struct may freeze.
- More than four written variables, or a write to `status`, is not this recipe: skip the
  block (Recipe J covers `status` when the inputs fit in parameters).
- Two blocks that differ only by a line or two (a plain and an alpha-blended sum) become
  two helpers that CodeScene may report as *Code Duplication*; measure, and keep the step
  only when no duplication appears.

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for each new
function, nothing else moved), and steps 1, 3 and 7 checked by reading: the oracle runs
single-threaded.

**Provenance:** 2026-10-29, from `effect.c`'s `BilateralBlurImage`, whose per-channel
body holds a plain and an alpha-blended weighted sum (`pixel`, `gamma` and the cursor `n`,
reading eight variables): Recipe K gives each helper eleven parameters, Recipe N refuses
the accumulators, and the per-channel body cannot move (Recipe E step 7) while the two
sums keep it a Bumpy Road. A probe of the three steps (sums, then the body): the
function leaves Brain Method and Deep, Nested Complexity, 1.72 to 1.89. Added by Claude
Code (Claude Opus 5.5) during task W07, under the mandate of 2026-10-24.

---

## Recipe U - Extract Function Returning the Loop Status Taking an Input Struct

**Use when:** CodeScene reports *Deep, Nested Complexity*, *Bumpy Road Ahead*, *Complex
Method* or *Large Method*, and the block that would fix it meets Recipe T in every
respect but one: with the variables it reads as parameters, the new function would take
five or more arguments (*Excess Number of Function Arguments*, which the pre-commit gate
refuses on a new function). The typical case is a per-pixel channel loop inside a
parallel row loop, reading both images, the context, the thread id and the coordinates.

**Not covered by:** Recipe T, which passes every input as a parameter; Recipe N, whose
block must meet Recipe E's OpenMP rule and so may not write `status`; Recipe J, which
takes accumulators by pointer and its inputs as parameters; Recipe Q, whose block writes
no `status`. T then B is two commits, of which the first adds a finding.

**How:**

1. The block meets Recipe T steps 1 to 4: it lies in one iteration of the parallel loop
   body with no `#pragma omp` line; its only write to a variable declared outside the
   loop body is `status=MagickFalse;`, and it does not read `status`; every loop-body
   variable it writes is declared in the block or not read after it before being
   written again (those are declared in the new function instead, with any initialiser);
   no `return`, `goto`, or `continue`/`break` of the parallel loop.
2. Put the variables it only reads, scalars and pointers none of which the block writes,
   in a struct as Recipe N steps 2 and 3 do: one member per variable, the variable's
   exact type and name, a top-level `const` or `magick_restrict` left off. A variable
   the block writes (a row pointer it advances) is never a member: it goes by value.
3. Cut the block into a new `static MagickBooleanType` function taking `const Name *name`
   first and then the written-but-not-read-after variables by value, at most four
   parameters. At the top of its body declare one `const` local per member, as Recipe N
   step 4 does; then a local `status` set to `MagickTrue` first, as Recipe T step 5
   does. The block's text is unchanged, and the function ends with `return(status);`.
4. In the caller, declare the `Name` local among the declarations at the top of the
   block holding the call, inside the parallel loop body, so each thread fills its own.
5. Directly before the call, assign every member from the variable of its name, in
   declaration order, with nothing between the assignments and the call.
6. Where the block stood, write `if (Helper(&name,...) == MagickFalse)
   status=MagickFalse;`, and check every member and argument by reading.

**Before:**

```c
    for (x=0; x < (ssize_t) fx_image->columns; x++) {
      ssize_t i;

      pfx->fxrts[id].thisPixel = (Quantum *)p;

      for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
      {
        ...
        if (!ExecuteRPN (pfx, &pfx->fxrts[id], &result, &pos)) {
          status=MagickFalse;
          break;
        }

        q[i] = ClampToQuantum ((MagickRealType) (QuantumRange*result));
      }
      p+=(ptrdiff_t) GetPixelChannels (image);
```

**After:**

```c
static MagickBooleanType ApplyFxToPixel (const FxPixelInputs * in,
  const Quantum *magick_restrict p, Quantum *magick_restrict q)
{
  const Image
    *const image = in->image;
  ...
  status=MagickTrue;
  for (i=0; i < (ssize_t) GetPixelChannels(image); i++)
  {
    ...
  }
  return(status);
}
...
      pixel_in.image=image;
      ...
      pixel_in.y=y;
      if (ApplyFxToPixel (&pixel_in, p, q) == MagickFalse)
        status=MagickFalse;
      p+=(ptrdiff_t) GetPixelChannels (image);
```

**Careful:**

- The two halves keep the arguments of the recipes they come from. The write to the
  shared `status` moves to the end of the block, as in Recipe T: the block does not read
  `status`, and OpenMP orders this unsynchronised write against no other thread. The
  members are copied directly before the call from variables the block does not write,
  as in Recipe N, into a local each thread owns.
- A temporary the caller declared with an initialiser (`result = 0.0`) and the block
  writes before every read moves into the new function with its initialiser; it is
  then initialised once per call instead of once per outer iteration, which no read
  can see. A temporary read before it is written in the block is not one: skip the block.
- Writes through pointers the block already used (the pixels of `q`, the per-thread slot
  `pfx->fxrts[id]`) stay as they are: the same thread writes the same objects.

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for the new function,
nothing else moved), and steps 1, 2 and 6 checked by reading: the oracle runs
single-threaded.

**Provenance:** 2026-10-29, from `fx.c`'s `FxImage`, whose last depth-4 site is the
per-channel loop of its parallel row loop (seven inputs: both images, `pfx`, `id`, `x`,
`y`, `p` and `q`, plus `status`). Recipe T gives the helper seven parameters, and moving
the whole column loop leaves it a Complex Method (cc 9). A probe of the struct form:
the function leaves Deep, Nested Complexity, fx.c 2.74 to 2.80. Added by Claude Code
(Claude Opus 5.5) during task W07, under the mandate of 2026-10-24.

---

## Recipe H - Hoist Function-Local Macros

**Use when:** a function `#define`s constants or helper macros at the top of its body, and
the blocks Recipe E or P would move use them. A helper outside the function cannot sit
above those lines, so every such block is pinned: `HuffmanDecodeImage`, `LZWEncodeImage`,
`TransformSignature`, `GenerateDifferentialNoise`, `SyncExifProfile`, `GetEXIFProperty`.

**Not covered by:** Recipe E, whose step 7 moves a macro only together with a block that is
its sole user; here the macros serve the whole function. Recipe S, which moves whole
functions between files. No other recipe changes where a preprocessor line sits.

**How:**

1. Take the run of `#define` lines (with their `\` continuations and the blank lines
   between them) that starts on the first line after the function's opening `{` and ends
   before its first declaration. Nothing else may sit in that run.
2. Move the run, unchanged, to directly above the function's signature: below its
   comment banner and below any helper already placed there.
3. Leave every `#undef` and any `#define` further down the body where it is.
4. Run `sh tools/pp_identical.sh <file>`. It must print `IDENTICAL`: the preprocessed
   translation unit, with the build's own flags, is the same before and after. If it
   differs, a token between the old and new position named one of the macros (usually a
   parameter in the signature); undo the step.
5. Helpers that later use the macros go between the hoisted run and the function, never
   above the run.

**Before:**

```c
MagickExport MagickBooleanType HuffmanDecodeImage(Image *image,
  ExceptionInfo *exception)
{
#define HashSize  1021L
#define MBHashA  293L
  ...
  CacheView
    *image_view;
```

**After:**

```c
#define HashSize  1021L
#define MBHashA  293L
  ...

MagickExport MagickBooleanType HuffmanDecodeImage(Image *image,
  ExceptionInfo *exception)
{
  CacheView
    *image_view;
```

**Careful:**

- A `#define` inside a function is already in force from its line to the end of the file
  (or to its `#undef`), so the step widens the macro's reach only over the function's
  signature. `pp_identical.sh` proves that no token there changed meaning.
- Do not hoist a macro that is `#define`d differently in an earlier function of the same
  file: the run would then redefine it above code that used the other definition. The
  compiler warns about a redefinition, and the build must stay free of warnings.
- The step changes no code Code Health measures and leaves the score flat. It is kept
  under rule 2 only when the same task follows it with the extractions it unblocks, and
  its commit names them.

**Evidence level:** guard, with `tools/pp_identical.sh` printing `IDENTICAL`. That is a
proof of identity for the build, stronger than the oracle, so no readiness verdict is
needed for this step. The extractions it unblocks keep their own evidence level.

**Provenance:** 2026-10-26, from the local macros that pinned `HuffmanDecodeImage`,
`HuffmanEncodeImage` and `LZWEncodeImage` (M68), `TransformSignature` (M61),
`GenerateDifferentialNoise` (M74), `SyncExifProfile` (M69) and `GetEXIFProperty` (M41), and
the local typedef in `GetImageFeatures` (M35), which is a separate shape (Recipe L).
Added by Claude Code (Claude Opus 5.5) during task M84, under the mandate of 2026-10-24.

---

## Recipe L - Hoist Function-Local Types

**Use when:** a function declares a `typedef` (a `struct` or an `enum`) at the top of its
body, and the blocks Recipe E, O or K would move declare, read or write variables of that
type. A helper outside the function cannot name the type, so every such block is pinned:
`GetImageFeatures` (cc 180, all of its working variables are of its local
`ChannelStatistics`), `ColorDecisionListImage`, `SyncExifProfile`, `GetEXIFProperty`.

**Not covered by:** Recipe H, which moves preprocessor lines and is proven by an identical
preprocessed text; a type that moves changes that text, and its scope ends at the
function's closing brace, not at the end of the file. Recipe E, which moves statements and
never declarations of types. No other recipe moves or renames a type.

**How:**

1. Take one `typedef` declaration that sits in the function's opening run of declarations
   (before its first statement), with no initialiser and no storage class, and whose
   members use only types declared at file scope or in headers.
2. Move it, unchanged, to directly above the function's signature: below its comment
   banner and below any helper or hoisted macro already placed there. Several such
   typedefs move in their original order.
3. Build. If the compiler reports that the tag, the typedef name or an enumerator is
   already declared (in a header or elsewhere in the file), rename that name: prefix it
   with a word from the function's name (`ChannelStatistics` in `GetImageFeatures` becomes
   `FeatureStatistics`, `_ChannelStatistics` becomes `_FeatureStatistics`), and rename
   every use inside the function. Choose a name that `grep` finds nowhere else in
   `MagickCore/` and in the headers the file includes.
4. Run `sh tools/obj_identical.sh <file>`. It must print `IDENTICAL`: the object file,
   compiled with the build's own flags at `-g0`, is the same byte for byte. A use left
   unrenamed in step 3 would name the other type and change the code; undo the step.
5. Helpers that later use the type go between the hoisted type and the function.

**Before:**

```c
MagickExport ChannelFeatures *GetImageFeatures(const Image *image,
  const size_t distance,ExceptionInfo *exception)
{
  typedef struct _ChannelStatistics
  {
    PixelInfo
      direction[4];  /* horizontal, vertical, left and right diagonals */
  } ChannelStatistics;

  CacheView
    *image_view;
  ...
  ChannelStatistics
    **cooccurrence,
```

**After:**

```c
typedef struct _FeatureStatistics
{
  PixelInfo
    direction[4];  /* horizontal, vertical, left and right diagonals */
} FeatureStatistics;

MagickExport ChannelFeatures *GetImageFeatures(const Image *image,
  const size_t distance,ExceptionInfo *exception)
{
  CacheView
    *image_view;
  ...
  FeatureStatistics
    **cooccurrence,
```

**Careful:**

- The type's layout, its members' names and types, and every variable's type stay as they
  were; only the place of the declaration and, in step 3, the type's own name change. The
  local name in `GetImageFeatures` shadows the public `ChannelStatistics` of
  `statistic.h`, which is why step 3 exists.
- Do not hoist a typedef whose members use a local macro defined further down the body,
  or another local type that stays behind: hoist those first (Recipe H, or this recipe).
- A `static` table declared with the type (`property.c`'s `TagInfo` tables) stays where it
  is; only the `typedef` moves.
- The step changes little that Code Health measures (the function loses the typedef's
  lines). It is kept under rule 2 only when the same task follows it with the extractions
  it unblocks, and its commit names them.

**Evidence level:** guard, with `tools/obj_identical.sh` printing `IDENTICAL`. That is a
proof of identity for the build, stronger than the oracle, so no readiness verdict is
needed for this step. The extractions it unblocks keep their own evidence level.

**Provenance:** 2026-10-26, from `GetImageFeatures` (M35, recorded there as blocked by its
local typedef), with the same shape in `ColorDecisionListImage`, `SyncExifProfile`,
`GetEXIFProperty` and `HaldClutImage`. A trial rename of the tag compiled to the same object file, and a planted
`<` to `<=` was reported as differing. Added by Claude Code (Claude Opus 5.5) during task
M90, under the mandate of 2026-10-24.

---

## Recipe F - Hoist Function-Local Static Data

**Use when:** CodeScene reports *Large Method* (or the function's length tips *Complex
Method* or *Bumpy Road Ahead* over a threshold), and much of the body is one `static`
declaration whose initialiser is data, not logic: a lookup table, a string assembled from
literals under `#if` lines (`GetMagickDelegates`: 124 of 130 lines), a set of
coefficients.

**Not covered by:** Recipe L, which moves a type and refuses anything with a storage class
or an initialiser; Recipe H, which moves preprocessor lines only; Recipe E, which moves
statements, never declarations. No other recipe moves a variable.

**How:**

1. Take one declaration in the function's opening run of declarations (before its first
   statement) with storage class `static` and an initialiser made only of literals,
   constant expressions, the `#if` lines inside it, and the addresses of other file-scope
   data.
2. The variable is never written: its type is `const`, or it is a pointer to `const` that
   no statement assigns. A static the function updates (a counter, a cache, a flag set
   once) does not move.
3. Move the declaration with its initialiser and the `#if` lines inside it, unchanged
   except that lines indented under the declaration lose two spaces, to directly above the function's
   signature, below its comment banner and below any hoisted macro or type it uses.
4. Keep the name unless `grep` finds it at file scope in this file or in a header the file
   includes; then prefix it with a word from the function's name and rename the uses in
   the function.
5. Build without warnings: a later function with a local of the same name now shadows
   the file-scope variable, and the build's warnings say so; rename as in step 4.
6. Run `sh tools/obj_same_code.sh <file>`. It must print `SAME CODE`: every function's
   machine code is unchanged, and the strings and data are the same, though the compiler
   may lay them out in another order.

**Before:**

```c
MagickExport const char *GetMagickDelegates(void)
{
  static const char
    *delegates=""
#if defined(MAGICKCORE_AUTOTRACE_DELEGATE)
  " autotrace"
#endif
  ...
  ;
  if (*delegates == '\0')
    return(delegates);
  return(delegates+1);
}
```

**After:**

```c
static const char
  *delegates=""
#if defined(MAGICKCORE_AUTOTRACE_DELEGATE)
  " autotrace"
#endif
  ...
  ;

MagickExport const char *GetMagickDelegates(void)
{
  if (*delegates == '\0')
    return(delegates);
  return(delegates+1);
}
```

**Careful:**

- A `static` local already has static storage duration and is initialised once, before
  the program runs; at file scope it keeps both, so only its visibility widens, to the
  rest of the file below it. Step 2 makes that harmless: nothing else writes it.
- `tools/obj_same_code.sh` compares the machine code before the linker fills in data
  addresses, so it cannot tell which of two hoisted tables an instruction reads. Two
  tables from one function, or a rename in step 4, are checked by reading that each use
  names the table it named before; the oracle covers the rest.
- The step changes nothing but the function's length. Rule 2 keeps it only if the score
  rises or a finding leaves the function.

**Evidence level:** oracle, with `tools/obj_same_code.sh` printing `SAME CODE`.

**Provenance:** 2026-10-27, from `version.c`'s `GetMagickDelegates`, whose only finding,
*Large Method* (130 lines), is a 124-line `#if`-assembled string; the same shape holds
`GetEXIFProperty`'s tag table (304 lines), `TransformsRGBImage`'s colour tables (236),
`TransformSignature`'s constants and `AcquireResizeFilter`'s mapping table. Added by
Claude Code (Claude Opus 5.5) during task M101, under the mandate of 2026-10-24.

---

## Known plateaus

None recorded yet. When a file stops improving and no recipe in this catalogue applies,
and you do not add one, record it here: the file, its score, and the smells left.

| File | Score | Smells left, and why no recipe reaches them | Recorded |
| --- | ---: | --- | --- |
| `MagickCore/resize.c` | 2.89 | Brain Method and Deep Nesting (`InterpolativeResizeImage`'s channel loop assigns and tests the shared `status` inside `omp parallel for`, which even Recipe T refuses; `ScaleImage`'s bookkeeping writes six variables on survivor lines); Bumpy Road, Complex and Large Method (OpenMP-bound: the filters `continue` the parallel loop on failure; survivor lines; LQR not compiled); Excess Arguments (public signatures; the filter kernels' context is computed per iteration, and bundling it costs the filters more than the kernels gain). 2.45 before Recipe S, 2.87 before Recipe T on `SampleImage`. See [M01's report](tasks/M01-resize.md) | 2026-10-24, Mac |
| `MagickCore/magnify.c` | 6.57 | Complex and Large Method: `Hq2XHelper` is a rule table written as a `switch` (splitting its cases yields clones with 7-8 arguments, 5.16 in a probe); `Fish2XBottomRightPixel`'s ladder shares six flags that only a struct declared after statements could carry; the 3x3 kernels and `IsEpbx2XBlendable` split into clones of each other (5.93 in a probe). Bumpy Road and Large Method in `MagnifyImage`: its last bumps are the parallel loop's `continue` and the atomic progress block. Overall Code Complexity: the mean of the above. From 6.21, with Recipes E, T, M and B. See [M01's report](tasks/M01-resize.md) | 2026-10-24, Mac |
| `MagickCore/compare.c` | 1.77 | Code Duplication, Deep Nesting, Bumpy Road, Excess Arguments and a Complex Conditional sit in the FFTW-only region (lines 2889-4569, not compiled here), so no category can leave; `GetSSIMSimularity` and `GetPHASESimilarity` write five accumulators per pixel (over Recipe K's four); `CompareImages`' pixel loop needs ten arguments; the `Accumulate*Pixel` helpers differ in more than one value; `IsImagesEqual` has no case. See [M02's report](tasks/M02-compare.md) | 2026-10-24, Mac |
| `MagickCore/colorspace.c` | 2.80 | Bumpy Road, Deep Nesting, Complex and Large Method in `sRGBTransformImage` and `TransformsRGBImage`: each is one `switch (colorspace)` whose every case holds its own `#pragma omp parallel for` (and the map tables are filled under pragmas), so no case can move without its pragma; a probe extracting all seven pixel loops of `sRGBTransformImage` left the score, nesting and bumps unchanged. Code Duplication and Excess Arguments in the public `ConvertRGBToGeneric`/`ConvertGenericToRGB` (not ready, 0%). See [M06's report](tasks/M06-colorspace.md) | 2026-10-25, Mac |
| `MagickCore/fourier.c` | 3.27 | On the Mac, which has no FFTW: most findings are in the FFTW-only functions (`ForwardFourier`, `InverseFourier`, the transforms, `RollFourier`, the quadrant swaps), not compiled here; `ComplexImages`' operator `switch` sets two locals inside its parallel loop (Recipe O is outside OpenMP only; a K-style helper takes eight arguments). See [M12's report](tasks/M12-fourier.md) | 2026-10-25, Mac |
| `MagickCore/colormap.c` | 9.36 | Complex Method in `SortColormapByIntensity` (cc 13) and `CycleColormapImage` (cc 10): guard tests, the parallel loops' status checks and the sync; Recipe T on the row costs more (Excess Arguments, and cc 9 is still flagged) than it saves (8.55 in a probe). See [M13's report](tasks/M13-colormap.md) | 2026-10-25, Mac |
| `MagickCore/semaphore.c` | 9.38 | Code Duplication between `LockSemaphoreInfo`, `UnlockSemaphoreInfo` and `RelinquishSemaphoreInfo`: shared assertions and preprocessor structure, a different primitive in each, most of the text in branches not compiled here. See [M19's report](tasks/M19-semaphore.md) | 2026-10-25, Mac |

### Tool notes

- **CodeScene's start line** for a function often includes the comment block above it.
  Use the compiler's view of where a function starts; `tools/make_task.py` does.
- `refactor_guard.py --calls` fails on a renamed function until the rename is declared
  (`--renamed OLD=NEW`); that is legal only for a file-local static, and in practice
  only for a helper this campaign created.

---

## The verification loop

After every commit, as [`AGENTS.md`](../../AGENTS.md) sets out:

1. `tools/oracle/build.sh cand`, then `tools/ci_warnings.sh <file>`: the oracle builds
   accept what ImageMagick's own CI rejects (a declaration after a statement, a variable
   MSVC cannot prove initialised)
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
| 2026-10-24 | NEW | O | Extract Function with Out-Parameters: a selection outside OpenMP that sets several locals together moves to a `static void` function taking the written locals by pointer | `MagnifyImage`'s method `switch` (cc 33 to 18, 212 to 123 lines in a probe) and the same shape in `AcquireResizeFilter` and `ScaleImage`, all refused by Recipe E's step 4 | mandate (2026-10-24) |
| 2026-10-24 | NEW | T | Extract Function Returning the Loop Status: a block inside a parallel loop body whose only outer write is `status=MagickFalse;` moves to a `static MagickBooleanType` function; the caller writes `status` from its result. The OpenMP section names it as the one exception to Recipe E's proviso | `MagnifyImage`'s column loop (probe: bumps 3 to 2, 109 to 94 lines) and the same shape in `resize.c`'s `SampleImage` | owner |
| 2026-10-24 | NEW | M | Merge Duplicate Parameter: a `static` function whose every call passes one identifier to two parameters of the same type loses the second | `magnify.c`'s `PixelsEqual` (5 arguments, 84 calls, all passing one buffer twice) | owner |
| 2026-10-24 | NEW | B | Bundle Arguments into a Struct: parameters of a `static` function that travel together become one `const` struct pointer; each caller fills a `const` struct from the same argument expressions | `magnify.c`'s pixel kernels, `Hq2XHelper` (11 arguments) and `CopyMagnifiedPixels`; a probe removing the finding raised the file 6.26 to 6.46 | owner |
| 2026-10-24 | AMEND | E | Step 7: a block that runs to the end of a `void` function may contain `return;`; the new function is `void` and its call is the caller's last statement; macros used only by the block move with it | `Fish2X`'s ladder of early returns (probe: 6.52 to 6.56), refused by the founding wording, which covers only the throw macros' returns | mandate (2026-10-24) |
| 2026-10-24 | WORDING | T | Provenance corrected (the shape is in `SampleImage` only, not `InterpolativeResizeImage` or the filters); the Careful note now refuses blocks that test or assign `status`, not a `break` of an inner loop after `status=MagickFalse;` | Reading the three loops after the recipe was added | after the fact |
| 2026-10-24 | NEW | K | Extract Accumulating Block with Out-Parameters: a per-pixel accumulation inside a parallel loop body that writes two to four of the iteration's own variables moves to a `static void` function taking them by pointer | `compare.c`'s FUZZ, MEPP, DPC and NCC similarity loops, refused by Recipe E's step 4; their single-accumulator siblings each gained about 0.02 with Recipe E | mandate (2026-10-24) |
| 2026-10-24 | AMEND | D | Blocks identical up to a consistent renaming of the variables they read or write count as identical; the variables become the helper's arguments; checked by script | M02's `GetNCCSimilarity`, deduplicated onto `GetDPCSimilarity`'s helper under this reading before it was written down; `ColorDecisionListImage`'s three parse blocks | after the fact |
| 2026-10-24 | AMEND | G | Careful: a comparison with a floating-point operand is inverted as `!(a < b)`, not `a >= b`, which differs for NaN | `ClipCLAHEHistogram`'s `(double) *p < clip_limit`, where the written-out inverse would have run the block for a NaN limit | mandate (2026-10-24) |
| 2026-10-24 | WORDING | rule | *Two levels of evidence*: at the guard-verified level, E and P pass on the exact extract fingerprint (`+2` per new helper, nothing else moved), which `--calls` prints as a `WARN`; any other movement stops the commit | The literal reading would forbid every extraction at the guard level, contradicting the same paragraph's list of E and P; found before the first guard-verified step (M04) | mandate (2026-10-24) |
| 2026-10-25 | AMEND | E | Step 7: a block of a value-returning function whose every path ends in `return(<expr>);` moves to a function of the caller's return type; the block becomes `return(<call>);` | `QueryColorCompliance`'s fallback to a colour name when `ParseCSSColor` finds no values (probe: 2.89 to 2.91, bumps 6 to 5), refused by step 7's `void`-only wording | mandate (2026-10-24) |

Type is `NEW`, `AMEND` or `WORDING`. *Decided by* is `owner`, `mandate (<date>)` or
`after the fact`.
| 2026-10-26 | NEW | H | Hoist Function-Local Macros: a run of `#define` lines at the top of a function body moves unchanged to directly above the function, proven by an identical preprocessed translation unit (`tools/pp_identical.sh`); the extractions it unblocks follow | Local macros pinned every block of `HuffmanDecodeImage`, `LZWEncodeImage`, `TransformSignature`, `GenerateDifferentialNoise` and `SyncExifProfile` (M61, M68, M69, M74); a trial hoist of `HuffmanDecodeImage`'s 31 lines preprocessed identically | mandate (2026-10-24) |
| 2026-10-26 | NEW | L | Hoist Function-Local Types: a `typedef` at the top of a function body moves to directly above the function, renamed when its name is already declared at file scope, proven by an identical object file (`tools/obj_identical.sh`); the extractions it unblocks follow | `GetImageFeatures`' local `ChannelStatistics` (M35) pins every block of a cc-180 function; it shadows the public type of the same name. A trial rename compiled to the same object file; a planted `<` to `<=` was caught | mandate (2026-10-24) |
| 2026-10-26 | AMEND | P | Several sites: one predicate may replace the same expression at several sites, at guard level, when a script confirms each site equals the body with arguments substituted and every argument is free of side effects; `--calls` shows +1 per extra site and one fewer of each inner callee per replaced copy. Decided by Claude Code under the owner's delegation of 2026-10-26 | PR #16's open question; `GetDelegateList` (decode/encode) | owner |
| 2026-10-26 | AMEND | B | Arithmetic members: an initialiser may be arithmetic on identifiers and literals (`1.0/scale`), computed once at the declaration; step 5 keeps its operands unchanged until the last call | `identify.c`'s `PrintChannelStatistics`: all 15 calls pass `file`, `1.0/scale` and `channel_statistics` | mandate (2026-10-24) |
| 2026-10-27 | AMEND | B | Member accesses and casts: an initialiser may read a member (`a.b`, `p->b`) or cast an allowed expression, when the struct is declared directly before the one call that uses it, with no statement between | `shear.c`'s XShearImage, YShearImage and CropToFitImage (`shear.x`, `bounds.width`, `(ssize_t) (image->rows-height)/2`), `transform.c`'s CopyImageRegion, `channel.c`'s FlattenPixelInfo: the last argument findings of two files, refused only by step 4 | mandate (2026-10-24) |
| 2026-10-27 | NEW | F | Hoist Function-Local Static Data: a `static` declaration whose initialiser is data (a table, an `#if`-assembled string) and which nothing writes moves to directly above the function, proven by unchanged machine code and the same data (`tools/obj_same_code.sh`) | `GetMagickDelegates` (version.c's only finding: 130 lines, 124 of them one string); the same shape in `GetEXIFProperty`, `TransformsRGBImage`, `TransformSignature`, `AcquireResizeFilter` | mandate (2026-10-24) |
| 2026-10-27 | AMEND | rule | *Two levels of evidence*: a function some case executes is oracle-verified for every recipe; no mutation run (readiness gate) before a step. Guard level stays for code no case reaches | The harness was built under mutation guidance and is finished; the 3SX and Jellyfin campaigns refactored without per-step mutation runs, and today's uncapped runs only confirmed it | owner |
| 2026-10-27 | AMEND | O | Inside a parallel loop body: allowed when every variable the block writes is declared in that body and it writes nothing declared outside it (`status` included) and has no `#pragma omp` line, Recipe K's condition. Each thread's call then writes only that thread's variables, as the block did | `KmeansImage`'s nearest-centroid search (`k`, `min_distance`), `CannyEdgeImage`'s neighbour selection (`alpha_pixel`, `beta_pixel`), `DrawGradientImage`'s repeat offset (`offset`, `repeat`, `antialias`): selections over iteration-private variables, refused only by step 1 | mandate (2026-10-24) |
| 2026-10-28 | AMEND | rule 1 | One commit may apply the same recipe to several functions when each step alone lowers the score and only the set raises it | pixel.c's five fixed-map Recipe E extractions: each alone 1.73 → 1.71 (the file passes a size threshold), all five 1.80; the pre-commit safeguard refuses the single steps (M100) | owner |
| 2026-10-28 | AMEND | S | Step 4 (register the file; the two tools); *Careful*: how a split meets the pre-commit gate's new-file rule; evidence names the combined guards. Wording of what `magnify.c` (2026-10-24) and pixel.c's and resize.c's splits did | `MagnifyImage`, `ExportImagePixels`, `ImportImagePixels`, `ResizeImage` | mandate (2026-10-24) |
| 2026-10-28 | NEW | J | Extract Accumulating Block Returning the Loop Status: a block in a parallel loop body that writes `status=MagickFalse` (without reading it) and two to four of the iteration's accumulators moves to a `static MagickBooleanType` function that returns its status and takes the accumulators by pointer (Recipes T and K together) | `vision.c`'s four moment-threshold functions share the first moment pass; one helper used by all four measures 2.35 → 2.59. On `MotionBlurImage`'s kernel loops it was flat or worse (9 to 11 arguments) | owner |
| 2026-10-28 | AMEND | J | Step 5: an array accumulator goes as a pointer with its uses unchanged, as Recipe K already allows | `PerimeterThreshold`'s row loop counts into `pattern[4]`, the object's own array, and writes `status=MagickFalse` | mandate (2026-10-24) |
| 2026-10-28 | AMEND | S | Scope: any MagickCore file whose family splits cleanly, not only the approved list; a split may lower MagickCore's per-file mean (the new file starts red) when it raises the original file; *Careful*'s new-file gate procedure confirmed | statistic.c's `StatisticImage` family (dry run 2.29 → 2.61); the per-file mean dip of the first three splits (4.97 → 4.96, line-weighted 3.149 → 3.200) | owner |
| 2026-10-28 | AMEND | B | Step 5: the struct is declared among the declarations at the top of the block holding its calls, `const` with its initialiser where its operands are final there, otherwise filled member by member where it takes its values; `tools/ci_warnings.sh` joins the verification loop | `Main`'s macOS arm64 job (`-Wdeclaration-after-statement -Werror`) has failed on every merge since #31: 40 structs declared after statements in 20 files, as step 4's "directly before the call" placed them | mandate (2026-10-24) |
| 2026-10-29 | NEW | N | Extract Function Taking an Input Struct: an E-legal block whose inputs would give the new function five or more parameters moves to a `static` function taking a `const` struct of the inputs it only reads, filled member by member directly before the call; `const` locals of the old names keep the block text unchanged | `WaveletDenoiseImage`'s threshold loop (six inputs; with Recipe E 2.41 to 2.36 in M05, with the struct 2.84 to 2.90); M05's `SteganoImage` bit switch and M101's `MotionBlurImage` kernel loops refused for the same reason | mandate (2026-10-24) |
| 2026-10-29 | AMEND | E | Step 3: a block whose single output is an array of the caller's, updated element by element, takes the array as a pointer with its uses unchanged, as Recipe K does for arrays | `BlendMeanImage`'s serial first pass sums each channel into `mean[MaxPixelChannels]`; outside OpenMP, so Recipe K (parallel loop bodies) does not name it, and E's "return the single value" cannot return an array. A probe: the function's last depth-4 site goes, 2.43 to 2.49 | mandate (2026-10-24) |
| 2026-10-29 | AMEND | E | Step 7: a loop's whole body may move even when it `continue`s that loop; each such `continue;` becomes `return;` of the new `void` function, whose call is the whole body, so both end the iteration at the same point | `statistic.c`'s GetImageStatistics entropy pass (W06: moving the outer loop gives the helper a Bumpy Road; moving the inner loop needs six parameters), and the per-channel bodies of `effect.c`'s blur family (W07), both refused for want of this | mandate (2026-10-24) |
| 2026-10-29 | NEW | Q | Extract Accumulating Block Taking an Input Struct: a Recipe K accumulation whose inputs would give the new function five or more parameters moves to a `static void` function taking a `const` struct of the inputs it only reads (Recipe N) and a pointer per written scalar (Recipe K), at most four parameters | `BilateralBlurImage`'s plain and alpha-blended per-channel sums (`pixel`, `gamma`, `n`; eight inputs): K gives eleven parameters, N refuses accumulators; a probe of the sums and then the per-channel body (Recipe N), 1.72 to 1.89 | mandate (2026-10-24) |
| 2026-10-29 | AMEND | E | Step 7: inside a `switch` case, a block after which the case runs only its closing `break;` may contain `break;` statements of that `switch` when its single output is one variable: the new function takes it by value and returns it, each such `break;` becomes `return(v);`, and the block becomes `v=Helper(...);`, so both reach the case's end with the same value | `fx.c`'s `ExecuteRPN`: its pixel-lookup cases (`fU`, `fU0`, `fUP`, `fS`/`fV`, `fP`/`fSP`/`fVP`) hold its nesting of 8 and each sets `regA` and leaves the `switch` by `break` from inside nested `if`s. A probe removing them: depth 8 to 4, fx.c 2.62 to 2.92 | mandate (2026-10-24) |
| 2026-10-29 | NEW | U | Extract Function Returning the Loop Status Taking an Input Struct: a Recipe T block whose inputs would give the new function five or more parameters moves to a `static MagickBooleanType` function taking a `const` struct of the inputs it only reads (Recipe N) and the written row pointers by value; the caller writes `status=MagickFalse` from its result (Recipe T) | `FxImage`'s per-channel loop inside its parallel row loop (seven inputs and `status`): T gives seven parameters, N refuses the `status` write, the whole column loop is a Complex Method (cc 9). A probe: FxImage leaves Deep, Nested Complexity, fx.c 2.74 to 2.80 | mandate (2026-10-24) |
