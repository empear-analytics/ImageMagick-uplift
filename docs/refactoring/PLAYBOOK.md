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
   used only by the block (`#define` ... `#undef`) move with it.

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

**Evidence level:** oracle.

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
   `#pragma omp single`, `critical` or `atomic`. Inside one, use Recipe E or skip it.
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
4. In each caller, declare a `const Name` local initialised from the exact argument
   expressions, member by member in declaration order. Each expression must be an
   identifier or a literal, not a call or anything with a side effect.
5. Nothing may write an identifier in the struct's initialiser between the declaration
   and the last call that uses the struct; declare it where that holds (inside the loop
   iteration, or just before a single call).
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

Type is `NEW`, `AMEND` or `WORDING`. *Decided by* is `owner`, `mandate (<date>)` or
`after the fact`.
