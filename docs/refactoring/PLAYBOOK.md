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
   the commit message lists them. The same holds for several blocks of one function:
   when the recipe applied to each block on its own lowers the score and the blocks
   together raise it, they may share one commit, each block the recipe's mechanical
   step, and the message names each block with the score it gave alone. The blocks of
   one function may take different recipes when what lowers each alone is a finding of
   the whole file that the first new function switches on (*Overall Code Complexity*,
   which CodeScene reports only from a file's seventh function on): each block is its
   own recipe's mechanical step, the message names each block with its recipe and the
   score it gave alone, and the `Recipe:` trailer names the recipe of the block that
   leaves a finding category (the trailer takes one code). The same holds for a set
   whose blocks lie in several functions of the file (blocks of a function and a block
   of a helper an earlier step made), each its own recipe's mechanical step, when
   *Overall Code Complexity* is what lowers each alone and no smaller set this rule
   already admits (the blocks of one function, or one recipe across functions) raises
   the score: the message names each block with its function, its recipe and the
   score it gave alone, and the score of each such smaller set. When such a
   set (of functions or of blocks) leaves the score flat instead, rule 2 decides it as
   it decides a single flat step: the set is kept when a targeted function leaves a
   finding category, and the message names that finding and the next step.
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
  the block already uses for pixel data (`p`, `q`) is fine. So is writing the thread's
  own slot of a per-thread array through the pointer the block already uses
  (`current_depth[id]`, where `id` is the loop body's `GetOpenMPThreadId()` and the block
  does not write it): the pointer passes by value, as Recipe U's *Careful* allows for
  `pfx->fxrts[id]`, and the same thread writes the same element. Moving a statement into a
  callee does not change which thread runs it; changing which variables it writes would.
- The same holds inside the structured block of `#pragma omp single`: a block in it may
  be extracted (Recipe E) into a `static` function called from the same place inside
  that structured block, provided it writes no variable declared outside the structured
  block (not directly, not by passing its address) and contains no `#pragma omp` line.
  The pragma and the braces it governs stay, now holding the call. The one thread that
  runs the structured block runs the call, and with it the moved statements, at the same
  point, and the implied barrier at its end does not move.
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
they need the oracle. The one exception is Recipe D's *Unreached functions* form,
whose proof is an identical object file, not the guard. Recipe CN rests on the
same proof, so it applies at either level.

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
   When that value is a member the block assigns in a struct local of the caller
   whose other members are not all set (`MontageImageList`'s `bounds.height`, a
   `RectangleInfo` whose `x` and `y` nothing sets), pass the struct as a pointer and
   write `(*v).m` at each use in the block (Recipe Q step 5's form), in a `void`
   function: copying the struct by value, in or out, would read members never set.
   The struct is a plain local whose address is not taken elsewhere (Recipe O step 2).
   A `const` struct local of the caller, initialised at its declaration, which the
   block neither assigns nor writes a member of, and whose address it takes only as
   `&s` passed to a pointer-to-`const` parameter of a callee that reads the members
   (and may write through its pointer members), is an input like any other: it goes
   by value as a parameter of its exact type, the block's `&s` unchanged (Recipe TW
   step 2's form; `CompareImagesBounds`'s `DifferingPixelScan scan`, passed to
   `FindDifferingPixel`). The copy holds the same member values, so the callee reads
   what it read, and a write through a pointer member lands in the object the
   caller's member names, as before; nothing writes either struct.
4. If the block writes to more than one outer variable, **skip it** and move to the next
   bump. Do not invent an out-parameter struct. A variable that nothing after the block
   reads does not count (a block that runs to the end of the function, step 7, advancing
   the row pointers `p` and `q`): the new function declares it, with the initialiser it
   had, when no statement between that declaration and the block writes it.
   One declared without an initialiser, which the block writes before it reads it on
   every path, is declared in the new function without one even when the caller writes
   it before the block (a loop counter `y` that an earlier loop of the caller also
   ran): the block never reads the value it held on entry, as Recipe TW step 3 and
   Recipe J step 3 already say. Its declaration leaves the caller only when nothing
   else there uses it.
   One the block reads on entry and then writes, and that nothing after the block
   reads (`AdaptiveBlurImage`'s `w`, counted down to release the kernels allocated so
   far, just before the caller throws), is an input like any other: it goes by value,
   its text unchanged, and its writes land in the new function's copy, which nothing
   reads once the function returns. Check by reading every path out of the block.
   One the block writes before it reads it on every path through the block, and that
   nothing after the block reads, is declared in the new function with the declaration
   it had, initialiser included, even when the caller wrote it before the block: the
   block never reads the caller's value (Recipe Y step 2's temporary). A struct filled
   by a call (`fstat(fd,&attributes)`) counts as written by it only where the block
   reads it after the call succeeded (`(fstat(...) == 0) && (attributes.st_size > 0)`).
   One the block writes before it reads it on every path, but only through a test a
   compiler cannot follow (`(i == 0) || (d < distance)`, whose second operand the first
   pass skips), so that declared in the new function without an initialiser it draws a
   may-be-uninitialised warning (`-Wconditional-uninitialized`, MSVC C4701), goes by
   value in the same way when the caller sets it before the block and nothing after
   the block reads it (`GetImageMinimumBoundingBox`'s `distance`): the value it carries
   in is never read, as before, and its writes land in the copy.
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
   returns the caller's value. When no path leaves `v` alone (the block writes it
   before it reads it on every path), the new function declares `v` without an
   initialiser instead of taking it, as step 4 does for such a variable: the value it
   held on entry is never read, and the caller may not have set it. The block contains no `continue`, `return` or `goto`,
   and a `break` of a loop or `switch` nested inside it does not change.
   A block of that kind with no `v`, which writes no variable of the caller that the
   caller reads after it (its writes go through pointers it already uses, `*pixel`,
   `resample_filter->average_pixel`, or into temporaries step 4 declares in the new
   function), moves to a `void` function instead: each such `break;` becomes
   `return;`, and the block becomes the call alone, before the case's closing
   `break;`. Both end the case at the same point, after the same writes through the
   same pointers.
   Likewise, a block that ends a loop's body (its last statements, after statements
   that stay) may contain `continue;` statements of that loop, under the whole-body
   form's conditions: each becomes `return;` of a new `void` function, and the call is
   the body's last statement, so both end the iteration at the same point, as the
   first form does for a block that ends a `void` function. The body's variables that
   only the block writes and reads move into it with their declarations (step 4); one
   the staying statements write and the block reads is an input.

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

**Leading operands of a chain.** The expression may be the leading operands of a
longer chain of one operator (`a && b` of `a && b && c`) when the whole chain as one
predicate would take five or more parameters (*Excess Number of Function Arguments*,
which the pre-commit gate refuses on a new function). C groups `&&` and `||` from the
left (C11 6.5.13, 6.5.14): `a && b && c` is `(a && b) && c`, so `a && b` is a whole
operand of the last `&&`. The predicate returns those leading operands, copied
character for character, in the form above, and the site becomes
`(Pred(...) != MagickFalse) && c`. The operands still evaluate left to right, and `c`
only when `a && b` is true, as before. Only a prefix of the chain moves: never an
operand from its middle or its end, and never across a mix of `&&` and `||`.

**Evidence level:** guard.

**Provenance:** founding recipe (2026-09-29). *Several sites:* 2026-10-26. It arose from
the question in PR #16 (segment.c, a guard-level reuse showing +1 in `--calls`) and from
delegate.c's `GetDelegateList`, whose decode and encode tests are the same expression on
two fields. *Leading operands of a chain:* 2026-10-30, from
`attribute-bounds.c`'s `GetImageBoundingBox`, whose lower-left test reads `x`, `y`,
`bounding_box`, `pixel` and `target` (task W10).

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

**Casts to the variable's own type.** A cast of a variable to the exact type it is
declared with (`(Image *) image`, where `image` is an `Image *` parameter) converts
nothing: C converts a value to its own type unchanged, so the cast expression and the
variable alone give the same value of the same type. Such a cast is not a differing
value: check each declared type by reading, have the script remove these casts before
it compares, and write the helper without them. A cast to any other type (one that adds
or drops a qualifier, or changes signedness, width or the pointed-to type), or a cast of
anything but a plain variable, is a differing value. A block whose every path ends in
`return(<expr>);` becomes `return(<call>);` in each copy, the helper having the
callers' return type, as Recipe E step 7 says.

A variable the copies write and their callers read afterwards goes to the helper by
address, exactly as in Recipe O (one pointer per such variable, the helper writing through
it where each copy wrote the variable); the helper may instead return one of them, as in
Recipe E step 7's value form. The copies must still be identical under the renaming. Owner
decision, 2026-10-29: "as you see fit" on the question raised in #70 and #73
(`ExportUnpacked12BitSample`, `GetStrokeSlope`).

**One value at several positions; conditions.** The single differing value may stand
at more than one position of the block, when each copy has the same value at every one
of them (`0.0` at both comparisons of one copy, `-32.0` at both of the other; a negated
constant is one value): the helper takes it as one parameter of the constant's type
(`const double` for a `double` constant), written at each of those positions. The copies
may also be operands of a condition rather than statements: a parenthesised operand of
`||` or `&&` (a whole operand, not part of a longer chain of the same operator). The
helper is then a predicate in Recipe P's form, `return((<expr>) ? MagickTrue :
MagickFalse);` with the expression copied character for character, and each copy becomes
its call, which `||` and `&&` test for nonzero as they tested the comparison's `0` or
`1`. Check by script that each copy equals the helper's expression with its arguments
substituted, as Recipe P's *Several sites* does; the literal guard reports the removed
copies of the value as deduplication (`WARN`, every value still present).

**Unreached functions.** When no oracle case executes the function, D may still replace
a block with a call to an existing `static` helper of the same file, under three
conditions, all checked before the commit:

- `casemap.py` shows 0 cases for the function, and for each of its callers in the
  catalogue (a caller some case executes makes the function oracle-verified instead);
- the block is identical to the helper's body as above (the same text once the
  variables are renamed consistently, the helper's parameters for the block's
  variables; checked by script); a declaration in the caller that only the block
  used goes with it, as the helper declares its own;
- `sh tools/obj_identical.sh MagickCore/<file>.c` prints `IDENTICAL` for the whole
  object file, the working tree against `HEAD`.

Nested replacements in one function may share the commit: an inner block becomes a call
of one helper, and the loop around it, now the same text as a second helper's body,
a call of that one; each replacement is checked by script, and the comparison covers
the result. The commit carries `Verified: guard` and quotes the `IDENTICAL` line. The
literal guard may report the dropped counts of a deduplication (`WARN` with every
value still present), and `--calls` may move only the functions the replaced code called (each
down by the number of calls it made) and the helper it now calls (up by one); any
other movement stops the step.

**Careful:**

- Why identical object code is proof: `obj_identical.sh` compiles the file as
  committed and as edited with the oracle build's own flags (`-O2`, the same
  includes and defines), at `-g0`, with `__LINE__` and `__FILE__` pinned and both
  copies under the same basename, and compares the two object files byte for byte.
  Identical bytes are the same compiled code for every function in the file, so the
  library built from them behaves the same on every input, reached by a case or
  not. It is a stronger proof than the oracle's, for this build; it also covers
  argument order, which the guard cannot see.
- A step whose object file differs is not this form, whatever the reason (the
  helper not inlined, blocks laid out differently): fall back to the oracle, or skip
  the step. Do not rework the block or the helper until the files compare
  identical: the form covers a plain replacement, nothing else.
- The comparison build has no OpenMP. A block inside a parallel loop body moves only
  under the OpenMP rules above, checked by reading: the helper call may write no
  variable declared outside the loop body (Recipe E step 5).
- The proof holds for this build. Code in a preprocessor branch it does not compile
  stays untouched, as everywhere.

**Evidence level:** oracle; or, for a function no case reaches, the identical object
file of *Unreached functions*.

**Provenance:** founding recipe (2026-09-29). *Casts to the variable's own type:* 2026-10-30, from
`colorspace.c`'s `SetImageGray` and `SetImageMonochrome`, whose closing blocks differ in
the image type set and in a `(Image *)` cast of the `Image *` parameter.

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
   elsewhere, so no two out-parameters can name the same object. One whose address the
   caller passes elsewhere, outside the block, only as an argument of calls that keep
   no copy of it (`TrimImage`'s `CropImage(image,&geometry,exception)`, which reads
   `*geometry` and stores no pointer; checked by reading the callee) still qualifies,
   as in Recipe I step 8: while the new function runs, no other pointer to it exists.
   So does one whose address the caller takes only to fill a member of another
   helper's targets struct, under Recipe I step 2's exception (a struct of Recipe I, LB
   step 8 or Q step 8, its `const` form included, read by nothing but that helper's
   call).
   A parameter the caller receives by value, of a struct or arithmetic type (not a
   pointer, and not an array parameter, which is one), counts as a plain local
   (`ApplySpliceGravity`'s `RectangleInfo splice_geometry`): it is the caller's own
   copy, and the same rule on its address applies. So does a parameter of pointer type
   (declared `T *p`, not as an array) whose own value the block assigns, a cursor such
   as `ParseEntities`' `xml` (`xml++`, `xml=strchr(xml,';')`): it passes as `T **p`,
   written `(*p)` at every use, so `*xml++=c` becomes `*(*xml)++=c` and `xml[i]` becomes
   `(*xml)[i]`. Writes through it stay writes through it, to the memory the caller's
   caller owns, as before. Recipes V, W, LB, I and OC read "plain local" the same way.
   A variable the block does not read before writing it, and that nothing after the
   block reads (a discarded out-argument, `MontageImageList`'s `sans` in
   `GetMontageGeometry(...,&sans,&sans)`), is not one of them: the new function
   declares it, as Recipe E step 4 does, and it is no parameter.
3. The block must contain no `return`, `goto`, `continue`, or `break` that leaves the
   block; a `break` that ends a `switch` or loop inside the block is fine.
4. Cut the block into a new `static void` function directly above the caller. Pass each
   variable it only reads by value, and each variable it writes as a pointer.
5. In the body, write `*v` for every use of a written variable `v`, reads included, so
   a value the block does not overwrite is the caller's value; its address `&v`, passed
   on to a callee, is written `v`, the same address (Recipe Q step 5). Change nothing
   else.
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
- Under step 2's exception the other call receives `&v` at a point outside the block,
  before or after the new function runs, and keeps no copy of it once it returns; so
  when the new function runs, its out-parameter is the only pointer to `v`, and no two
  of its pointers name one object. A callee that stores the address (in a struct, a
  global, a cache) gives `v` a second name: the variable does not qualify.
- Under step 2's targets-struct exception the struct does keep `&v`, but it is the
  caller's local, never passed to the new function, and read by nothing but the call of
  its own helper, which runs before or after the new function, never during it. While
  the new function runs nothing reads that member, so its out-parameter is the only
  pointer to `v` in use (Recipe I's *Careful*).
- A parameter received by value is an object of the function's own, initialised from
  the argument when the call starts and gone when it returns (C11 6.9.1p9); a write to
  it, directly or through `&v`, reaches no object of its caller. That holds for a
  parameter of pointer type too: the pointer is the function's own object, so
  assigning it (`xml++`) through `&xml` reaches nothing of the caller's. What it
  points to is the caller's, so a block whose only writes are through it never counts
  it as written; those writes stay writes through the same pointer value, now read as
  `(*p)`, in the same order. An array parameter is a pointer, but its declared form
  `T p[]` is refused, to keep the helper's `T **` parameter visibly the same type.
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
   The loop's status may have another name or type: a variable `v` declared outside the
   loop body, that every statement of the parallel loop writes only as `v=c;` of one
   constant `c` (`opaque=MagickFalse;`, `type=UndefinedType;`), and that the loop body
   reads only in its leading `if (v == c) continue;`, or in a leading
   `if ((status == MagickFalse) || (v == c)) continue;` that tests the loop's `status`
   too, is the loop's status. The new function is step 5's, `status=MagickFalse;` standing where the block wrote `v=c;`,
   and the caller writes `if (Helper(...) == MagickFalse) v=c;` (step 7). The body may
   also read `v` after the block, in the same iteration, as the test that ends an inner
   loop the block sits in (`IsHighDynamicRangeImage`'s `if (hdri != MagickFalse)
   break;` after its channel loop): the caller's write lands as the call returns,
   before that read.
3. Every variable of the loop body that the block writes is declared inside the block,
   or is not read after it (a row pointer `q` advanced along the row, say).
4. The block contains no `return`, `goto`, or a `continue` or `break` of the parallel
   loop itself; a `continue` or `break` of a loop inside the block is fine. One
   exception: a block that ends the parallel loop body (its last statements, after the
   statements that stay) may contain `continue;` of the parallel loop. Each becomes
   `return(status);`, and the call of step 7 is the body's last statement, so both end
   the iteration at the same point (Recipe E step 7's loop-tail form).
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
- Under step 2's other name, `v` plays `status`'s part exactly: the loop writes it only
  as `v=c;`, so "the block set `v`" and "the block's local `status` became
  `MagickFalse`" are the same event, and the caller writes the same `c` then, deferred
  to the end of the block as everywhere in this recipe. The block does not read `v`,
  and the only read inside the loop is the guard at the top of the next iteration, as
  with `status`. A guard that tests `status` and `v` together is still that one read:
  it skips the iteration on either value, before any statement of the body, and the
  block reads neither, so deferring the block's write of `v` to the end of the block
  changes what that guard sees no more than deferring a write of `status` does. A
  variable the loop also writes another value (`IdentifyImageGray`'s
  `type=GrayscaleType;`) or reads elsewhere in the body is not a status: skip it. The
  one exception is step 2's read after the block: in the block, `v=c;` is followed
  only by statements that write nothing outside the block (the `break` of the block's
  own loop, the block's end), so the caller's `v=c;` when the call returns comes
  before any later statement of the iteration, the read included, and that read sees
  the same value it saw before. Other threads get no ordering for this unsynchronised
  flag either way, as for `status`.
- Under step 4's exception a `continue;` of the parallel loop ends the iteration
  where it stands; `return(status);` ends the helper there, and the caller's
  `if (...) status=MagickFalse;` is the last statement of the body, so the iteration
  ends after it either way, having run the same statements. The shared `status` gets
  `MagickFalse` exactly when the block's local did (usually the `status=MagickFalse;`
  just before the `continue`), deferred to the end of the iteration as everywhere in
  this recipe. A block followed by a statement that stays (a progress block under
  `#pragma omp atomic`) is not this exception: its `continue` skipped that statement,
  and the call would not.

**Evidence level:** oracle, with `refactor_guard.py --calls`, and steps 1 to 4 checked by
reading: the oracle runs single-threaded and cannot see them.

**Provenance:** 2026-10-24, from `MagnifyImage` (its per-row column loop), with the same
shape in `resize.c`'s `SampleImage`, both refused by the OpenMP rule of Recipe E. (Not
`InterpolativeResizeImage`, whose block assigns and tests `status`, nor the two filters,
whose failures `continue` the parallel loop.) Owner's go-ahead the same day. Added by Claude
Code (Claude Opus 5.5) during task M01, on `magnify.c`. Step 4's exception (2026-10-30,
task W09): `paint.c`'s `FloodfillPaintImage`, whose tiling loop ends in a fetch that
sets `status=MagickFalse;` and `continue`s on failure, a fill and a sync. Step 2's
other name (2026-10-30, task W09): `attribute.c`'s `IsImageOpaque` (`opaque=MagickFalse;`)
and `IdentifyImageMonochrome` (`type=UndefinedType;`), whose row loops are T's shape with
the flag the function returns in place of `status`. Step 2's combined guard (2026-10-30,
task W10): `channel-alpha.c`'s `SetImageAlphaChannel`, whose `OffIfOpaqueAlphaChannel`
row loop skips on `status` or `opaque` and writes `opaque=MagickFalse;` at the first
pixel that is not opaque, its only depth-4 site. Step 2's read after the block
(2026-10-31, campaign B, task M116): `image.c`'s `IsHighDynamicRangeImage`, whose
channel loop sets `hdri=MagickTrue;` and breaks, and whose column loop then breaks on
`hdri`; it holds the file's only *Deep, Nested Complexity* (a probe: 2.77 to 2.90).

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
   with no statement between. So is the address of a plain local of the caller,
   written `(&v)`, for a pointer parameter whose argument was `&v` (a progress counter
   the callee increments): the address does not change while the function runs, so
   writes to `v` between the fill and a call (by an earlier call, through it) do not
   count against step 5.
5. Declare the local among the declarations at the top of the block that holds its
   calls (the loop body, for a struct per iteration), never after a statement: the
   macOS CI job compiles with `-Wdeclaration-after-statement -Werror`. Nothing may
   write an identifier of the initialiser between the point where the struct takes its
   values and its last call. Where that point can be the declaration, and the
   initialiser has no member access or cast, declare it `const` with the initialiser.
   Otherwise declare it without `const` or initialiser, and fill it at that point,
   `name.member=expr;` for every member in declaration order, with nothing between.
6. A struct declared outside a `#pragma omp` region and read inside it is `const`, and
   none of its members point to data the region writes through the struct. A struct
   the function reaches only through its `const Name *name` parameter, a local of the
   caller filled before the call (step 5) and written by nothing until the call
   returns, counts as `const` here. A member whose target the region writes (a
   progress counter, `exception`) is bound to a `const` local of its old name at the
   top of the body (step 3), and the region uses the local.
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
- An address member (`(&offset)`) is the value the argument `&offset` had: the same
  address at every call, whatever the earlier calls wrote to `offset` through it.
- A struct reached through a pointer to `const` (step 6) is read by every thread of the
  region and written by none: the caller filled it before the call and runs nothing
  until the call returns. A thread reading `name->member` in the region reads the value
  the parameter held, as it read the parameter before. A member whose target the region
  writes is read once, into the bound local, outside the region; the region writes
  through the local exactly as it wrote through the parameter, so a `#pragma omp
  atomic` statement on it (`(*progress)++;`) keeps its text.

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
inside a parallel loop body (or in a sequential function, step 6) that adds into two to
four of the iteration's own accumulators (`channel_similarity` and `channel_area`, or
three sums).

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
   that runs the block, so it is reset as often as before. A variable of the caller that
   the block writes before reading it, and that the caller does not read after the block
   before writing it again (a loop counter the caller also uses elsewhere), is declared
   in the new function instead, without an initialiser, and is not one of the four, as
   Recipe J step 3 and Recipe Q step 2 do.
4. Cut the block into a new `static void` function directly above the caller. Pass each
   variable it only reads by value; each array it writes as a pointer (its uses do not
   change); each scalar it writes as a pointer, written `*v` (or `(*v)++`) at every use,
   reads included.
5. Call it where the block stood, passing `&v` for each written scalar and the array
   for each written array, in parameter order; check every argument by reading.
6. A block in a function with no OpenMP region at all (a sequential pass, as Recipe Q
   step 1 admits) moves the same way: it writes at most four plain locals of the caller,
   none a parameter, a global or a `static`, and none whose address is taken elsewhere
   (Recipe O step 2); steps 1, 3, 4 and 5 hold as written. The written locals may be
   cursors the block advances and the caller reads afterwards (a partition's `l` and
   `h`), not only sums.

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
- A step-3 temporary moved into the new function is written there before it is read,
  as it was in the block, so its value on entry was never read; and the caller writes it
  again before it next reads it, so the value the block left was never read either. The
  new function's own copy behaves the same for both.
- Under step 6 there is no thread to share with: each pointer names the caller's own
  local, whose address is taken nowhere else, so no two pointers name one object, and
  `*v` reads and writes it at the same points and in the same order as `v` did (Recipe
  O step 5's argument); the value the caller reads after the call is the one the block
  left. A macro argument written `pixels[*l]` for `pixels[l]` expands to the same
  access.

**Evidence level:** oracle, with `refactor_guard.py --calls`, and steps 1 to 3 checked by
reading.

**Provenance:** 2026-10-24, from `compare.c`'s `GetFUZZSimilarity` (`channel_similarity`
and `channel_area`), `GetMEPPSimilarity` (mean and maximum error), `GetDPCSimilarity`
and `GetNCCSimilarity` (three sums each), whose per-pixel blocks Recipe E's step 4
refused after the single-accumulator siblings (`GetAESimilarity`, `GetMAESimilarity`,
`GetMSESimilarity`, `GetPASimilarity`, `GetPDCSimilarity`) had taken Recipe E. Added by
Claude Code (Claude Opus 5.5) during task M02, under the mandate of 2026-10-24. Step 6
(2026-10-30, task W09): `statistic.c`'s `GetMedianPixel`, whose quickselect partition
loop advances the two cursors `l` and `h` that the caller then reads.

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
   A plain struct value (a `PixelInfo`, `PointInfo` or `RectangleInfo` local or
   parameter; not an array, an array of structs, or a pointer's target) may be one of
   the group when, checked by reading: (a) every member of it is set on every path to
   the call, by an initialiser, a whole-struct assignment or a fill such as
   `GetPixelInfo(image,&v)`; (b) the block assigns neither `v` nor any member of it, and
   takes `&v` or `&v.m` only as an argument to a pointer-to-`const` parameter of a
   callee that only reads through it, neither storing nor comparing the address; (c) no
   pointer to `v` or to a member of it is held anywhere the block can write through; and
   (d) the block writes through none of its pointer members. Steps 3 to 6 then treat it
   as a scalar: a member of the struct's type, `const PixelInfo v = name->v;` or
   `name->v.m`, and `name.v=v;`, a copy taken directly before the call.
   A plain struct local that meets (b) to (d) but not (a), some member of it never set
   on the path to the call (`ScaleScanlineX`'s `PointInfo scale`, whose `y` nothing
   sets), may be one of the group by address instead: a member `const T *v` (a pointer
   to `const` of its exact type), filled `name.v=(&v);` directly before the call, bound
   `const T *const v = name->v;`, and each use `v.m` in the block written `(*v).m`, the
   form Recipe E step 3 gives a struct the block updates. Nothing is copied, so no
   member is read that the block did not read.
   When the caller is itself a function that binds `const` locals from the members of
   its own `const Name *name` parameter (a helper of this recipe, of Recipe Y or of
   Recipe LB's struct form, or a function whose parameters Recipe B bundled, in the
   form of its step 3 that binds a `const` local of each old name), and every
   variable of the group is one of those locals,
   bound from the member of its own name and type and not written since, the new
   function may take that `name` in place of a new struct (`PopulatePosterizeMap`'s
   `map_info`, passed on to its column loop): the caller passes `name` unchanged, the
   new function binds the members its block reads as step 4 does, and steps 3, 5 and
   6 do not apply (no typedef, no local, no assignment). A member the block does not
   read is not bound; the other parameters are as above, at most four in all.
3. Declare `typedef struct _Name { ... } Name;` directly above the new function, one
   member per variable in the group, with the variable's exact type and name: no `size_t`
   for a `ssize_t`, no `const` added or dropped on what a pointer points to. Only a
   top-level `const` of a parameter (`const double softness`) is left off the member,
   which step 6 assigns.
4. Cut the block into a new `static` function taking `const Name *name` in place of the
   group. At the top of its body declare one `const` local per member, of the member's
   type and name, initialised from the member; the block's text below is unchanged.
   Or, instead of the `const` locals, write `name->member` for every use of a member in
   the block, as Recipe B step 3 allows (Recipe Wa step 3): the form for a block whose
   sibling helper unpacks the same members or tests the same channel traits.
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
   *Sibling blocks* (the cases of one `switch`, each moved under this recipe in its own
   commit) may share one `Name` and one local: the typedef holds the union of their
   groups, and each new function reads only the members its block reads (the member
   form of step 4, or `const` locals for those members alone). The local is filled
   once, by step 6's assignments, at a point every call follows, and nothing on any path
   from the fill to a call writes a member's variable (Recipe B step 5's condition):
   checked by reading every path, the paths through blocks that stay in the caller
   included.

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
- A struct member by address (step 2's last form) reads the caller's own `v`: each
  `(*v).m` is the object `v.m` named, read at the same point, and (b) and (c) keep
  every write to `v` out of the block, so each read gives the value the block read.
  Unlike `name.v=v;`, which reads every member and so would read one never set (an
  indeterminate value), the pointer reads only the members the block reads. The
  address is held by the caller's struct local, read by nothing but the call, as a
  targets struct holds its addresses (Recipe I step 2's exception).
- A struct-valued member (step 2) is a copy: `name.v=v;` copies every member's value
  at the call, and the block reads the copy where it read `v`. The copy and `v` hold the
  same values for the whole block only while nothing writes `v`, so step 2 forbids,
  besides assignments to `v` and its members, every way the block could write it
  unseen: `&v` passed to a callee that writes through it (`GetPixelInfoPixel(image,p,&v)`
  inside the block refuses `v`), or a pointer to `v` kept elsewhere and written in the
  block. Reads through a pointer member reach the same object from the copy as from
  `v`, since only the pointer is copied; (d) keeps writes through it out.
- A callee that received `&v` through a pointer-to-`const` parameter now receives the
  copy's address: the same values at another address. That is the same behaviour only
  while the callee uses the pointer to read: one that stores it, compares it with
  another pointer, or prints it sees a difference. Read the callee (and what it passes
  `&v` on to) before choosing `v`; when in doubt, leave `v` out of the group.
- Copying a struct reads every member, where the block may have read only some, so a
  member left unset before the call would now be read uninitialised: (a) requires
  each to be set. Arrays stay out: one decays to a pointer, which is already a member
  of the pointer kind, and a copy of an array of structs is not a value C assigns.
- Every member is assigned before the call: an unassigned member is read uninitialised,
  and nothing checks it but reading (step 8). Count them against the typedef.
- Under step 2's pass-through form the new function reads, through `name`, the
  members its caller's `const` locals were initialised from. The struct is a local of
  the caller's caller, filled directly before that call and written by nothing while
  the call runs (it reaches the caller only through a pointer to `const`, and its
  address goes nowhere else), so each member still holds the value the caller's
  `const` local holds at the call, which is the value the block read. Nothing is
  copied, so no member is read that was not set. A Recipe B struct is the same: its
  local holds the argument expressions, `const` with them as its initialiser or filled
  member by member, and B step 5 keeps every identifier of them unwritten up to the
  last call. A pointer member the block writes through (`channel_sum`, under Recipe
  Q's read-only pointers) is read, not written: the new function reaches the same
  array through the same address the caller's `const` local held.
- Recipe E's limit on what the block writes stands: one value returned, or writes
  through a pointer the block already used for pixel data. A block writing more is
  skipped, as in Recipe E.
- In the member form of step 4 each `name->member` reads the same object the `const`
  local would have been initialised from, at each use instead of once; the struct is
  the caller's local, filled before the call and written by nothing in the block, so
  every read gives the value the block's own read of the variable gave. `->` binds
  tighter than a cast or an arithmetic operator, so `(double) name->i-x` groups as
  `(double) i-x` did.
- Under *Sibling blocks* (step 8), each call reads the values the members took at the
  fill. Nothing between the fill and the call writes a member's variable, so each holds
  the value the block's own read would have given, as in the single-call form; the
  argument is Recipe B's for one struct serving several calls. A block that writes a
  member's variable itself (a coordinate it computes before reading it) declares its own
  variable (Recipe E step 4) and does not read that member, and once moved it no longer
  writes the caller's variable.
- Two helpers that unpack the same members into `const` locals (the Manhattan and
  Voronoi nearest-point searches of `SparseColorImage`), or that test the same five
  channel traits, are reported as *Code Duplication*; the member form removes the
  shared locals. Keep the step only when no duplication appears.

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for the new function,
nothing else moved), and steps 1, 2 and 8 checked by reading.

**Provenance:** 2026-10-29, from `visual-effects.c`'s `WaveletDenoiseImage`, whose last
depth-4 site is the coefficient threshold loop (six inputs: the buffer, both offsets, the
pixel count, the magnitude and the softness). The Mac campaign tried it with Recipe E
(2.41 to 2.36 on the new function's *Excess Arguments*, M05) and refused `SteganoImage`'s
bit switch for the same reason; `MotionBlurImage`'s kernel loops (9 to 11 inputs, M101)
are the same shape. A probe of the struct form: the depth-4 finding leaves, 2.84 to 2.90.
Added by Claude Code (Claude Opus 5.5) during task W07, under the mandate of 2026-10-24.
Step 2's pass-through form (2026-10-31, task W10): `quantize-posterize.c`'s
`PopulatePosterizeMap`, a Recipe LB helper taking a `PosterizeMapInfo`, whose column loop
(depth 3) reads four of its five members and two locals of its own.

---

## Recipe Q - Extract Accumulating Block Taking an Input Struct

**Use when:** CodeScene reports *Deep, Nested Complexity*, *Bumpy Road Ahead*, *Brain
Method*, *Complex Method* or *Large Method*, and the block that would fix it is a
per-pixel accumulation inside a parallel loop body (or in a sequential function, step 1)
that meets Recipe K in every respect but one: with its inputs as parameters, the new
function would take five or more arguments (*Excess Number of Function Arguments*, which
the pre-commit gate refuses on a new function). The typical case is a filter's per-channel weighted sum (`pixel`, `gamma`
and a cursor), reading the window pointer, the window size, the weights and the channel.
In a sequential function (step 1) the block may also be a running selection over an
array: a loop that, element by element, compares a candidate with the best so far and
updates the best value and its index together (an argmax: `TriangleThreshold`'s
`threshold` and `max_distance` over the histogram's bins). It folds the array into its
written locals as a sum does, under the same steps.

**Not covered by:** Recipe K, which passes every input as a parameter; Recipe N, whose
block may write only one value or through a pixel pointer, and which keeps written
variables out of its struct; Recipe J, which also writes `status` and passes its inputs
as parameters. K then B is two commits, of which the first adds a finding.

**How:**

1. The block meets Recipe K steps 1 to 3: no `#pragma omp` line, no `return`, `goto`, or
   `continue`/`break` that leaves it; it writes at most four of the parallel loop body's
   own variables, and nothing declared outside that body. A block in a function with no
   OpenMP region at all (a sequential pass) qualifies too: it then writes at most four
   plain locals of the caller, none a parameter, a global or a `static`, and none whose
   address is taken elsewhere (Recipe O step 2). In such a function `status` is one of
   those plain locals: it may be one of the four, passed by pointer like the others,
   and the block may read it (`if (*status == MagickFalse) continue;`) as well as
   write it. A pointer the block only reads, though it writes through it (the object
   table `object` that a callee updates), is a read-only variable (step 3), not one of
   the four.
2. Split what the block touches in two: the variables it writes (the accumulators and
   any cursor it advances, such as `n`), and the variables it only reads. Variables it
   declares and uses alone move with it, as in Recipe K step 3. So does a variable of the
   caller that the block writes before reading it there, and that the caller does not
   read after the block before writing it again (a loop counter, a fetched pixel): it is
   declared in the new function instead, as Recipe J step 3 does, and is not one of the
   four.
3. Put the read-only variables, scalars, pointers and struct values (Recipe N step 2),
   in a struct as Recipe N steps 2 and 3 do: one member per variable, the variable's
   exact type and name, a top-level `const` or `magick_restrict` left off. A written
   variable is never a member.
4. Cut the block into a new `static void` function taking `const Name *name` first, then
   one pointer per written scalar, in the order the block first writes them. At the
   top of its body declare one `const` local per member, as Recipe N step 4 does, or
   write `name->member` for every use of a member, as Recipe N step 4 also allows.
5. In the block's text, write each written scalar `v` as `*v` (`(*v)++` for `v++`) at
   every use, reads included, as Recipe K step 4 does, and its address `&v` (passed on to
   a helper) as `v`, the same address; nothing else in the text changes. A written
   variable may be a plain struct value the block assigns whole (`error=f(...,error);`,
   a `DoublePixelPacket`), as Recipe Z's `start` and `stop` are: it goes by pointer like
   a scalar, `*v` at every use, parenthesised where a postfix operator follows
   (`(*node_info)->child[id]`, `(*v).m`; Recipe Z step 4).
6. In the caller, declare the `Name` local at the top of the block holding the call (in
   the parallel loop body, so each thread fills its own), and directly before the call
   assign every member from the variable of its name, in declaration order, with nothing
   between the assignments and the call.
7. Call it with `&name` and `&v` for each written scalar, at most four parameters in
   all; check every member and every argument by reading.
8. When the written scalars, each its own pointer, would bring the function to five or
   more parameters, pass their addresses instead in one `const` struct, Recipe I's form
   (steps 3, 5 and 6): a `typedef struct` directly above the new function with one
   member per written scalar, in the order the block first writes them, a pointer to the
   variable's exact type under its name (`const double *magick_restrict k` gives
   `const double *magick_restrict *k`), holding addresses only. The function takes
   `const Name *name`, then `const Targets *targets`, and binds each member once at the
   top of its body, `T *const v = targets->v;`; the text then follows step 5
   (`(*pixels)[i]` for `pixels[i]`, the parenthesised `*v` of Recipe Z step 4). Or,
   instead of the bound locals, write `targets->v` for every use of `v` in that text
   (`*targets->pixel+=`, `(**targets->k)`, `(*targets->k)++`,
   `(*targets->pixels)[i]`): the member form of step 4, for the second of two sibling
   sums. The caller declares the `Targets` local beside the `Name` local, and directly before the
   call assigns the input members, then `targets.v=(&v);` per member in member order,
   with nothing else between them and the call. Inside a parallel loop every address is
   of a variable declared in the loop body. In this form the block may write up to six
   such variables, in place of step 1's four. An array of the caller that the block
   updates element by element (Recipe K step 2's accumulator array) counts as one
   written variable here too: its member is a pointer to the array's exact type
   (`double sum[MaxPixelChannels+1]` gives `double (*sum)[MaxPixelChannels+1]`), the
   caller writes `targets.sum=(&sum);`, and each `sum[i]` in the text becomes
   `(*sum)[i]`; inside a parallel loop the array is declared in the loop body. The
   `Targets` local may instead be declared `const` with the addresses as its
   initialiser, `const Targets targets = { &v, ... };`, one per member in member order
   (Recipe B step 5's `const` form), among the declarations of the block holding the
   call, every `v` declared in that block or one enclosing it; the `targets.v=(&v);`
   assignments are then left out.

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
- More than four written variables is not this recipe, nor, inside a parallel loop, a
  write to `status`: skip the block (Recipe J covers `status` when the inputs fit in
  parameters).
- `status` in a function with no OpenMP region (step 1) is a plain local of the
  caller, and Recipes K, J and T refuse it only because inside a parallel loop it is
  shared between threads. Passed by pointer, every read and every write of it in the
  block goes through `*status` to the caller's own variable, in the same order, as for
  any written local (Recipe O); so the block may read it, and no guard proving its
  value on entry is needed, unlike Recipe J, whose helper starts from a local
  `MagickTrue`. A `break` or `continue` of a loop inside the block after
  `*status=MagickFalse;` stays in the new function and ends or skips the same loop.
  Check by reading that the function holds no `#pragma omp` line anywhere and that
  `&status` is taken nowhere else in it.
- A member that is a pointer (`object`, `neighbors`, a cache view, `exception`) is
  copied, not the object it points to: writes through the copy, the block's own or a
  callee's, reach the same object as writes through the caller's variable did, in the
  same order. `ResolveRowEquivalences`' `equivalences` member is written through the
  same way. The pointer itself must not be assigned in the block.
- In a sequential function there is no thread to share with: each written local becomes
  `*v` in the callee, the same object in the same order, which is Recipe O's argument.
  A cursor the block advances and the caller reads again (a kernel pointer `k`, a window
  pointer `pixels`, advanced row by row by repeated calls) is such a written local, passed
  by pointer.
- Two blocks that differ only by a line or two (a plain and an alpha-blended sum) become
  two helpers that CodeScene may report as *Code Duplication*; measure, and keep the step
  only when no duplication appears.
- A written struct value (step 5) is the caller's own object behind the pointer, as
  Recipe Z's `start` and `stop` are: `*error=f(...,*error);` reads every member of the
  caller's variable and assigns every member back, where `error=f(...,error);` did, and a
  member the block leaves alone keeps the caller's value (Recipe O's argument). Nothing
  is copied into the helper or back out of it. The parentheses of `(*v)->m` and `(*v).m`
  are what `*v` means there: `*v->m` would apply `->` to the pointer to `v`.
- Step 8 is step 7 with the pointers carried in one struct, and Recipe I's argument
  holds: the written variables stay the caller's own locals, under their own names,
  read and written directly everywhere else; only their addresses travel, set directly
  before the call and dead after it, each bound once to a `*const` local, so no pointer
  is reseated and nothing is copied in or back. Each `*v` is the object `&v` named in
  step 7, so the operations, their objects and their order are step 7's (Recipe K).
- Inside a parallel loop the `Targets` local is the loop body's own, filled by each
  thread, and every address in it must be of a variable declared in the loop body,
  which each thread owns: an address of `status`, `progress` or anything else declared
  outside the body would let the callee write a variable the threads share, and refuses
  the block, as Recipe K step 2 does. The addresses are of distinct locals whose address
  the caller takes nowhere else, so they alias neither each other nor a member's
  pointee; a `magick_restrict` on a cursor stays on the pointed-to type of its member, so
  the cursor's qualifier and the promise it makes are unchanged.
- In the member form of step 8, `targets->v` is the pointer the bound local would have
  held: the struct is the caller's local, filled before the call and written by nothing
  in the block, so each read of the member gives that address. `->` binds tighter than
  unary `*`, so `*targets->v` is `*(targets->v)`, the object `*v` named. Two sibling
  sums that bind the same locals and end in the same cursor advances are reported as
  *Code Duplication* (`SelectiveBlurImage`'s plain and blended sums: 2.77 to 2.65); with
  the second in the member form, none is (2.84).
- A `const` `Targets` local initialised with `{ &v, ... }` holds what step 8's
  assignments would have stored: the address of a variable is fixed while the
  variable lives, and each `v` is declared in the block of the declaration or one
  enclosing it, so it lives from the initialiser to every call; a `const` local is
  never reseated. The initialiser reads no variable's value and has no call and no side
  effect, so evaluating it at the declaration instead of before each call changes
  nothing (Recipe B's argument). C99 admits an initialiser of an automatic aggregate
  that is not constant, as Recipe B's `const` locals already do (`compare.c`'s `sums`).
  Inside a parallel loop the declaration stands in the loop body, so each thread holds
  the addresses of its own variables.
- The four of step 1 keep the block one accumulation; with the addresses in one
  parameter the count no longer costs arguments, and step 8 allows six for a sum whose
  cursors advance in lockstep with its accumulators (`SelectiveBlurImage`'s kernel,
  pixel and luminance cursors beside `pixel` and `gamma`). A block writing more is
  skipped.

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for each new
function, nothing else moved), and steps 1, 3 and 7 (step 8: every address, and that
each is of a loop-body variable) checked by reading: the oracle runs single-threaded.

**Provenance:** 2026-10-29, from `effect.c`'s `BilateralBlurImage`, whose per-channel
body holds a plain and an alpha-blended weighted sum (`pixel`, `gamma` and the cursor `n`,
reading eight variables): Recipe K gives each helper eleven parameters, Recipe N refuses
the accumulators, and the per-channel body cannot move (Recipe E step 7) while the two
sums keep it a Bumpy Road. A probe of the three steps (sums, then the body): the
function leaves Brain Method and Deep, Nested Complexity, 1.72 to 1.89. Added by Claude
Code (Claude Opus 5.5) during task W07, under the mandate of 2026-10-24. Step 8
(2026-10-30, task W09): `effect.c`'s `AdaptiveBlurImage` and `AdaptiveSharpenImage`,
whose plain and alpha-blended kernel sums (nesting 6) write `pixel`, `gamma` and the
cursors `k` and `pixels`, and `SelectiveBlurImage`'s, which also advance
`luminance_pixels`: with a pointer each, five or six parameters. Step 5's struct values
(2026-10-30, task W09): `quantize.c`'s `ClassifyImageColors`, whose tree descents write
the struct `error` and the cursor `node_info`. The running selection of *Use when*
(2026-10-30, task W10): `threshold-auto.c`'s `TriangleThreshold`, whose split-point
search reads seven variables and writes `threshold` and `max_distance`.

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
   no `return`, `goto`, or `continue`/`break` of the parallel loop, except Recipe T step
   4's `continue;` in a block that ends the parallel loop body.
2. Put the variables it only reads, scalars, pointers and struct values (Recipe N step
   2) none of which the block writes, in a struct as Recipe N steps 2 and 3 do: one
   member per variable, the variable's exact type and name, a top-level `const` or
   `magick_restrict` left off. A variable the block writes (a row pointer it advances)
   is never a member: it goes by value.
3. Cut the block into a new `static MagickBooleanType` function taking `const Name *name`
   first and then the written-but-not-read-after variables by value, at most four
   parameters. At the top of its body declare one `const` local per member, as Recipe N
   step 4 does; then a local `status` set to `MagickTrue` first, as Recipe T step 5
   does. The block's text is unchanged, and the function ends with `return(status);`.
   Or, instead of the `const` locals, write `name->member` for every use of a member in
   the block, as Recipe B step 3 allows (Recipe Wa step 3).
4. In the caller, declare the `Name` local among the declarations at the top of the
   block holding the call, inside the parallel loop body, so each thread fills its own.
5. Directly before the call, assign every member from the variable of its name, in
   declaration order, with nothing between the assignments and the call.
6. Where the block stood, write `if (Helper(&name,...) == MagickFalse)
   status=MagickFalse;`, and check every member and argument by reading.
7. A block that is the whole body of a loop inside the parallel loop body (a channel
   loop), declarations included, may contain `continue;` statements of that loop: each
   becomes `return(status);`, and the body becomes the declaration of the `Name` local,
   its assignments and the call, as Recipe E step 7 does for a `void` function. The
   loop's counter goes by value. The block contains no `break` or `return` of its own,
   and no `continue` of a loop nested inside it changes.

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

## Recipe Wa - Extract Accumulating Block Returning the Loop Status Taking an Input Struct

**Use when:** CodeScene reports *Deep, Nested Complexity*, *Bumpy Road Ahead*, *Brain
Method*, *Complex Method* or *Large Method*, and the block that would fix it meets Recipe J
in every respect but one: with the variables it only reads as parameters, the new
function would take five or more arguments (*Excess Number of Function Arguments*, which
the pre-commit gate refuses on a new function). The typical case is a filter's
per-channel kernel sum that fetches each tap from a cache view (`status=MagickFalse;` when
the fetch fails) and adds into `pixel` and a kernel cursor `k`, reading the view, the
offsets, the coordinates, the channel and `exception`.

**Not covered by:** Recipe J, which passes every input as a parameter (nine to eleven
arguments on `MotionBlurImage`'s kernel loops, M101); Recipe Q, whose block may not write
`status`; Recipe U, which returns the status but passes no accumulator back; Recipe N,
which refuses both. J then B is two commits, of which the first adds a finding.

**How:**

1. The block meets Recipe J steps 1 to 3: it lies in one iteration of a parallel loop
   body with no `#pragma omp` line, no `return` or `goto`, and no `continue` or `break`
   that leaves it; its only write to a variable declared outside the loop body is
   `status=MagickFalse;`, and it does not read `status`; it writes at most four of the
   loop body's own accumulators and cursors, each read after it or read in it before it
   is written; every other loop-body variable it writes is declared in the block or not
   read after it before being written again (declared in the new function instead).
2. Put the variables it only reads, scalars, pointers and struct values (Recipe N step
   2), none written in the block, in a struct as Recipe N steps 2 and 3 do: one member
   per variable, the variable's exact type and name, a top-level `const` or
   `magick_restrict` left off. A written variable is never a member.
3. Cut the block into a new `static MagickBooleanType` function taking `const Name *name`
   first, then one pointer per written accumulator or cursor, in the order the block
   first writes them, at most four parameters in all. At the top of its body declare one
   `const` local per member, as Recipe N step 4 does, then a local `status` set to
   `MagickTrue` first, as Recipe J step 4 does. Or, instead of the `const` locals, write
   `name->member` for every use of a member in the block, as Recipe B step 3 allows:
   the form for a block whose sibling helper unpacks the same members.
4. In the block's text write each written variable `v` as `*v` at every use, reads
   included (`(*v)++` for `v++`, `(**k)` for `(*k)` where `k` is a pointer cursor), as
   Recipe K step 4 does; every `status=MagickFalse;` stays; the function ends with
   `return(status);`. Nothing else in the text changes.
5. In the caller, declare the `Name` local at the top of the block holding the call, in
   the parallel loop body, so each thread fills its own.
6. Directly before the call, assign every member from the variable of its name, in
   declaration order, with nothing between the assignments and the call.
7. Where the block stood, write `if (Helper(&name,&v,...) == MagickFalse)
   status=MagickFalse;`, and check every member and argument by reading.

**Before:**

```c
        k=kernel;
        pixel=0.0;
        if ((blur_traits & BlendPixelTrait) == 0)
          {
            for (j=0; j < (ssize_t) width; j++)
            {
              r=GetCacheViewVirtualPixels(motion_view,x+offset[j].x,y+
                offset[j].y,1,1,exception);
              if (r == (const Quantum *) NULL)
                {
                  status=MagickFalse;
                  continue;
                }
              pixel+=(*k)*(double) r[i];
              k++;
            }
            SetPixelChannel(blur_image,channel,ClampToQuantum(pixel),q);
            continue;
          }
```

**After:**

```c
static MagickBooleanType SumMotionPixels(const MotionSum *sum,double *pixel,
  MagickRealType **k)
{
  CacheView
    *const motion_view = sum->motion_view;
  ...
  status=MagickTrue;
  for (j=0; j < (ssize_t) width; j++)
  {
    r=GetCacheViewVirtualPixels(motion_view,x+offset[j].x,y+
      offset[j].y,1,1,exception);
    if (r == (const Quantum *) NULL)
      {
        status=MagickFalse;
        continue;
      }
    *pixel+=(**k)*(double) r[i];
    (*k)++;
  }
  return(status);
}
...
        if ((blur_traits & BlendPixelTrait) == 0)
          {
            sum.motion_view=motion_view;
            ...
            sum.exception=exception;
            if (SumMotionPixels(&sum,&pixel,&k) == MagickFalse)
              status=MagickFalse;
            SetPixelChannel(blur_image,channel,ClampToQuantum(pixel),q);
            continue;
          }
```

**Careful:**

- The three parts keep the arguments of the recipes they come from. The write to the
  shared `status` moves to the end of the block (Recipe T): the block does not read
  `status`, and OpenMP orders this unsynchronised write against no other thread. The
  accumulators and cursors are the iteration's own and change through `*v`, the same
  operations on the same objects in the same order (Recipe K). The members are copied
  directly before the call from variables the block does not write (Recipe N).
- A `continue` of a loop inside the block after `status=MagickFalse;` stays in the new
  function and skips the same iteration of the same loop; a block that tests `status` is
  not this recipe.
- A cursor the block reads before it writes it (`k`, set to `kernel` before the block) is
  a written variable, passed by pointer, never a member. A pointer cursor becomes a
  pointer to a pointer: `(*k)` reads as `(**k)` and `k++` as `(*k)++`.
- Two blocks that differ only by a line or two (a plain and an alpha-blended sum, which
  share the fetch and the member locals) become helpers CodeScene reports as *Code
  Duplication*: extracting both measured 2.06 to 2.01 on `MotionBlurImage` and 2.02 to
  1.95 on `RotationalBlurImage`. The shared lines are mostly the `const` locals: write
  the second helper in the member form of step 3 (on `MotionBlurImage`, no duplication,
  2.18 to 2.20), and keep the step only when no duplication appears.

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for the new function,
nothing else moved), and steps 1, 2 and 7 checked by reading: the oracle runs
single-threaded.

**Provenance:** 2026-10-29, from `effect.c`'s `MotionBlurImage`, whose plain kernel sum
(nesting 6: row, column, channel, blend test, tap loop, fetch test) writes `pixel`, the
cursor `k` and `status` and reads seven variables: Recipe J gives the helper ten
parameters (refused in M101), Recipes Q and U refuse the status or the accumulator. A
probe of the plain sum: nesting 6 to 5, effect.c 2.06 to 2.10. The same shape is
`RotationalBlurImage`'s plain sum. Added by Claude Code (Claude Opus 5.5) during task
W08, under the mandate of 2026-10-24.

---

## Recipe V - Extract Block Returning Success for a Cleanup Jump

**Use when:** CodeScene reports *Deep, Nested Complexity*, *Bumpy Road Ahead*, *Brain
Method*, *Complex Method* or *Large Method*, and the block that would fix it leaves the
function on failure through `goto <label>;`, to a cleanup label further down, as
`MorphologyApply`'s image clone does (`goto error_cleanup;` when the clone fails).

**Not covered by:** Recipe E, whose step 6 covers the `return` hidden in the throw
macros and whose step 7 the `return` of a block that ends the function: a jump to the
caller's label is neither, and a new function cannot jump to it; Recipes O, T, K, J, N,
Q and U, whose first step refuses a block containing `goto`.

**How:**

1. The block lies outside every OpenMP region and contains no `#pragma` line. Every
   `goto` in it jumps to one label `L` outside the block; it contains no `return`, and no
   `continue` or `break` that leaves it.
2. The block writes at most one variable of the caller, `v`, a plain local (not a
   parameter, a global or a `static`; a parameter Recipe O step 2 counts as a plain
   local qualifies) whose address is not taken elsewhere. Writes
   through pointers the block already uses stay as they are; variables it declares move
   with it. A temporary it writes before reading, and that the caller does not read
   after the block before writing it again, is declared in the new function and is not
   `v` (Recipe Y step 2). A `v` whose address the caller passes elsewhere, outside the
   block, only as an argument of calls that keep no copy of it (`GetProperty`'s `text`,
   whose address goes to `GetPropertySeconds`, which reads and replaces `*text` and
   stores no pointer; checked by reading the callee) still qualifies, as in Recipe O
   step 2.
3. Cut the block into a new `static MagickBooleanType` function directly above the
   caller. Each `goto L;` becomes `return(MagickFalse);`, one for one, and the function
   ends with `return(MagickTrue);`. Nothing else in the text changes.
4. Pass every variable the block only reads by value, and `v` as a pointer, written `*v`
   at every use, reads included (Recipe O step 5), at most four parameters in all;
   otherwise skip the block.
5. Where the block stood, write `if (Helper(...) == MagickFalse) goto L;`, and check
   every argument against its parameter by reading.
6. A block that leaves the function instead through `return(c);`, where every `return`
   in the block returns the same constant `c` (a literal, an enum value or a cast null
   pointer such as `(Image *) NULL`) and contains no `goto`, moves the same way: each
   `return(c);` becomes `return(MagickFalse);`, one for one, and the caller writes
   `if (Helper(...) == MagickFalse) return(c);`. Steps 1, 2 and 4 hold as written, with
   `return(c);` in place of `goto L;`. A block returning a variable, or two different
   values, is not this recipe, except under step 7. Another copy of such a block, in
   another function and identical to it once the variables are renamed consistently
   (Recipe D's test, checked by script), may call the same function the same way, with
   its own variables as arguments and its own `c`, one commit per caller, as Recipe W
   step 7 allows. In a function returning `void`, `return;` plays the
   part of `return(c);`: every `return` in the block is `return;`, each becomes
   `return(MagickFalse);`, and the caller writes `if (Helper(...) == MagickFalse)
   return;`.
7. A block whose every exit is the same run of cleanup statements `S` followed by
   `return(v);`, `v` a local of the caller (`GetThresholdMapFile`'s
   `thresholds=DestroyXMLTree(thresholds); map=DestroyThresholdMap(map); return(map);`),
   moves with `S` left in the caller: each `S` plus `return(v);` becomes
   `return(MagickFalse);`, one for one, and the caller writes
   `if (Helper(...) == MagickFalse) { S return(v); }`, `S` copied unchanged. Check by
   script that every copy of `S` is identical up to indentation. The block writes no
   variable `S` or `return(v);` reads or writes (it may write through `v` when `v` is a
   pointer, passed by value), contains no other `return` and no `goto`, and the caller
   does nothing after the block but its own statements. Temporaries it writes before
   reading, and the caller does not read after it, are declared in the new function
   (Recipe Y). Steps 1 and 4 hold as written, with `S return(v);` in place of `goto L;`.
   `return(c);` of one constant `c` (step 6) may end every exit in place of
   `return(v);` (`OptimizeLayerFrames`' `bounds=(RectangleInfo *)
   RelinquishMagickMemory(bounds); disposals=(DisposeType *)
   RelinquishMagickMemory(disposals); return((Image *) NULL);`): the caller writes
   `{ S return(c); }`, and step 2 then holds as well, its one written variable being
   one that `S` neither reads nor writes.
   `S return(c);` may also be one call of a function-local cleanup-and-throw macro
   (Recipe I step 8), left unchanged in the caller after
   `if (Helper(...) == MagickFalse)`; in that form `S` may read and write the block's
   one written variable `v`, passed as a pointer (step 4), since every write the block
   made to it has landed before the helper returns.
   `S return(v);` may also be a single `return(<expr>);` whose expression is a call on
   the caller's variables (`ParseKernelArray`'s `return(DestroyKernelInfo(kernel));`),
   the same text at every exit (checked by script), left unchanged in the caller after
   `if (Helper(...) == MagickFalse)`. The block writes no variable the expression
   reads; it may write through one, a pointer passed by value (`kernel`).
   `return(v);` after `S` may also be `return(e);`, where `e` is an expression with no
   call, assignment, increment or decrement, the same text at every exit (checked by
   script), and reads no variable the block writes (`NewXMLTree`'s
   `utf8=DestroyString(utf8); return(&root->root);`, where the block writes neither
   `utf8` nor `root`): the caller writes `if (Helper(...) == MagickFalse) { S
   return(e); }`. As with `v`, the block may write through a pointer `e` reads, passed
   by value.
   `S` may also be followed by a `return(<expr>);` whose expression makes calls, in place of `return(v);`
   (`ResizeImage`'s `resize_filter=DestroyResizeFilter(resize_filter);
   return(DestroyImage(resize_image));`): the caller writes
   `if (Helper(...) == MagickFalse) { S return(<expr>); }`, both unchanged, and the
   block writes no variable that `S` or the expression reads or writes.

**Before:**

```c
          kernel_loop++;     /* the iteration of this kernel */

          /* Create a clone as the destination image, if not yet defined */
          if ( work_image == (Image *) NULL )
            {
              work_image=CloneImage(image,0,0,MagickTrue,exception);
              if (work_image == (Image *) NULL)
                goto error_cleanup;
              if (SetImageStorageClass(work_image,DirectClass,exception) == MagickFalse)
                goto error_cleanup;
            }

          /* APPLY THE MORPHOLOGICAL PRIMITIVE (curr -> work) */
```

**After:**

```c
static MagickBooleanType AcquireWorkImage(const Image *image,
  Image **work_image,ExceptionInfo *exception)
{
  if ( *work_image == (Image *) NULL )
    {
      *work_image=CloneImage(image,0,0,MagickTrue,exception);
      if (*work_image == (Image *) NULL)
        return(MagickFalse);
      if (SetImageStorageClass(*work_image,DirectClass,exception) == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}
...
          /* Create a clone as the destination image, if not yet defined */
          if (AcquireWorkImage(image,&work_image,exception) == MagickFalse)
            goto error_cleanup;
```

**Careful:**

- Under step 6's `void` form the caller's `return;` returns nothing, as the block's
  did, from the same point and after the same writes; the helper's `MagickBooleanType`
  only says whether the block fell through. No value crosses the call that the block
  did not already leave behind a pointer or in `v`.
- On a failure path the caller jumps from the same point as before: the block's
  statements after the jump run in neither version, and every write before it has
  landed in `v` through the pointer, so the cleanup code at `L` reads the same values
  (here a `work_image` that is NULL, or a clone it destroys).
- On the success path the function returns at the block's end and the caller goes on
  with the next statement, where the block fell through.
- The failure paths are mostly allocation failures that no case reaches: the oracle
  checks the success path; the failure path is checked by reading, each `goto L;`
  against its `return(MagickFalse);`, and `refactor_guard.py --calls` moves only `+2`.
- A block that jumps to two labels, or writes two of the caller's variables, is not this
  recipe: skip it.
- Under step 2's exception the other call receives `&v` at a point outside the block,
  before or after the new function runs, and keeps no copy of it once it returns; so
  while the new function runs, its pointer parameter is the only pointer to `v` in use,
  and every read and write through it reaches `v` in the block's order (Recipe O's
  argument). A callee that stores the address (in a struct, a global, a cache) gives `v`
  a second name: the variable does not qualify.
- A temporary (step 2) holds nothing the caller uses: every path through the block
  writes it before reading it, so its value on entry is never read, and the caller
  writes it again before any later read, so the value the block leaves is never read
  either. Declared in the new function, it is written and read in the same order, and
  the caller's own copy, which no statement reads between the call and its next write,
  is left alone; when nothing else uses it, its declaration goes. This is Recipe Y's
  step 2 (and Recipe J's step 3) for a block whose inputs fit in parameters. Check by
  reading every use of the variable in the caller, on every path out of the block,
  `return(c);` and the jump to `L` included.
- Under step 6 the caller returns the same constant from the same point: every statement
  the original ran before its `return(c);` has run in the helper, with its writes to `v`
  landed through the pointer, and the caller runs nothing between the call and its own
  `return(c);`. A warning the block throws with `GetMagickModule()` now names the helper
  as its location, which the oracle normalises (`@ warning/LOCATION`); the message text
  is compared as before.
- Under step 7 the cleanup does not move, so nothing depends on what it computes: the
  caller runs the same `S` and returns the same `v` as the block did, at the same point.
  On a failure path the helper runs every statement the block ran before that exit and
  returns at once; the caller runs nothing between the call and `S`. `S` reads only
  variables the block does not write, so it sees the values it saw before: the same
  `thresholds` tree, the same `map` pointer (the block wrote only `map->` members, now
  through the copy of the pointer). Nor does it matter whether `S` makes `v` NULL:
  `return(v);` stays the caller's own text. The alternative, the helper running `S` and
  the caller returning `(ThresholdMap *) NULL`, would rest on reading that
  `DestroyThresholdMap` returns NULL on every path and would pass `map` by pointer;
  keeping `S` in the caller needs neither. The helper's pointer copy is dead once it
  returns, before `S` frees the memory. `refactor_guard.py --calls` shows each call in
  `S` fall by one per copy that merged into the caller's single `S`.
- Under step 7's `return(<expr>);` form the argument is the cleanup form's with the
  cleanup folded into the return statement: the statement stays the caller's own text,
  run by the caller at the point where the block ran it, after every statement the
  block ran before that exit, on variables the block did not write. So it calls the same
  functions with the same arguments and returns the same value (`DestroyKernelInfo`
  frees the same kernel, whose members the block wrote through the pointer before the
  exit, as before). On the success path it runs in neither version.
- Under step 7's `return(e);` form, `e` has no side effect, so evaluating it is only
  reading: the caller evaluates it at the point where the block returned it, after `S`,
  as before, on variables the block did not write, and memory the block wrote through a
  pointer `e` reads has landed before the helper returns. So `e` has the same value, and
  the caller returns it. `&root->root` reads only `root` and computes an address.
- Under step 6's other copy the function was cut from the first copy's text, and the
  second copy is that text with its own variables in place of the first copy's: the
  call runs the same statements on them, writes its own `v` through the pointer, and
  on a failure path the caller returns its own `c` from the point where the copy
  returned it. This is Recipe D's argument for a block that leaves the caller, which
  D itself, moving copies whole, does not take (Recipe W's *Not covered by*).
- Under step 7's macro form the argument is Recipe I step 8's with one written
  variable: the helper writes `v` through its pointer, so on a failure path the macro
  in the caller reads the value the block left (`Classify`'s `head`, still NULL when
  the default cluster's allocation fails), and runs at the same point as before.
- Under step 7 with `return(c);` the argument is the same with `c` for `v`. The writes
  in `S` (`bounds` and `disposals` relinquished, an image destroyed) are dead on that
  path, since the caller returns `c` at once and reads none of them, but the recipe does
  not rest on that: `S` stays the caller's own text, run on the caller's own variables
  at the same point, so the helper neither takes them nor writes them. The block's one
  written variable (`prev_image`, from `ReferenceImage`) has landed through the pointer
  before `return(MagickFalse);`, and `S` does not read it, so `S` sees the values it saw
  before. On the success path `S` runs in neither version. The other way to admit
  these blocks, moving `S` into the helper with the variables it writes passed by value
  so that its writes land in dead copies, is not this recipe: it would pass every
  variable `S` reads (seven parameters at `OptimizeLayerFrames`' background clone) and
  would rest on reading that the caller reads none of them afterwards.

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for the new function,
nothing else moved; under step 7 also `-(k-1)` for each call in `S`, where `k` is the
number of exits), and steps 1, 2 and 5 (step 7: the identical copies of `S`) checked by
reading.

**Provenance:** 2026-10-29, from `morphology.c`'s `MorphologyApply`, whose deepest sites
inside its four nested loops (kernel, stage, compound method, kernel iteration) leave
through `goto error_cleanup`, the plateau the earlier W07 pass on the file recorded. A
probe of the work-image clone: nesting 6 to 5, 2.46 to 2.51. Cleanup labels reached by
`goto` also occur in `property.c` and `annotate.c`. Added by Claude Code (Claude Opus
5.5) during task W07, under the mandate of 2026-10-24. Step 6 (2026-10-29, task W08):
`MorphologyImage`'s `convolve:scale` setting, which leaves through
`return((Image *) NULL);` when the kernel clone fails (nesting 4, two bumps). Step 7
(2026-10-30, task W09): `threshold.c`'s `GetThresholdMapFile` (cc 18, 150 lines), whose
ten validation exits each throw a warning and end in the same destroy of the XML tree
and the map, then `return(map);`.
Step 7's `return(c);` (2026-10-30, task W09): `layer.c`'s `OptimizeLayerFrames`, whose
`NoneDispose` reference ends in releasing `bounds` and `disposals`, then
`return((Image *) NULL);` (nesting 4), and `CoalesceImages`' dispose-image clone, which
destroys the coalesced list first.
Step 2's temporary (2026-10-30, task W09): `morphology.c`'s `AcquireKernelInfo`, whose
token step writes `kernel` and, as a temporary, the parse result `new_kernel`.
Step 2's address passed to calls that keep no copy (2026-10-30, task W09): `fx.c`'s
`GetProperty`, whose "not a number" branch writes `text`, whose address the other
branch passes to `GetPropertySeconds`.
Step 7's `return(<expr>);` (2026-10-30, task W09): `morphology.c`'s `ParseKernelArray`,
whose `:` geometry parse (size and offset checks) and the `:`/odd-square selection
around it leave through `return(DestroyKernelInfo(kernel));` when an offset lies
outside the kernel.
Step 6's other copy (2026-10-30, task W09): `resize-image.c`'s `HorizontalFilter` and
`VerticalFilter`, whose contribution-buffer allocation (`return(MagickFalse);` when it
fails) is the same text in both.
Step 6's `return;` (2026-10-31, task W10): `resample.c`'s `ScaleResampleFilter`, a
public `void` function whose ellipse-fitting block (the limit test, the support scaling
of `F` and the four bounds) leaves through `return;` once the ellipse is too large.

---

## Recipe Y - Extract Block Returning Success Taking an Input Struct

**Use when:** CodeScene reports *Bumpy Road Ahead*, *Deep, Nested Complexity*, *Complex
Method* or *Large Method*, and the block that would fix it meets Recipe V in every respect
but one: with the variables it only reads as parameters and its written variable as a
pointer, the new function would take five or more arguments (*Excess Number of Function
Arguments*, which the pre-commit gate refuses on a new function). The typical case is one
pixel of a recursive subdivision that fetches its neighbours from cache views and leaves
through `return(c);` when a fetch fails, reading the views, the segment, the midpoint and
the noise amplitude.

**Not covered by:** Recipe V, which passes every input as a parameter and skips a block
that needs more than four (step 4); Recipe N, whose block meets Recipe E and so leaves
through no `goto` and no `return` of its own; Recipe U, whose block lies in a parallel loop
and returns the loop status, not a jump. V then B is two commits, of which the first adds
a finding.

**How:**

1. The block meets Recipe V steps 1 and 2: it lies outside every OpenMP region with no
   `#pragma` line; it leaves the function only through `goto L;` to one label, or only
   through `return(c);` of one constant `c` (Recipe V step 6), or only through exits of
   one of Recipe V step 7's forms (a cleanup run and its `return` that stay in the
   caller, the same text at every exit, checked by script); it writes at most one
   variable `v` of the caller that the caller reads after it, a plain local whose address
   is not taken elsewhere.
2. Every other variable of the caller that the block writes, it writes before reading it
   in the block, and the caller does not read it after the block before writing it again
   (a pixel pointer, a coordinate): it is declared in the new function instead, as
   Recipe J step 3 and Recipe U step 1 do.
3. Put the variables the block only reads, scalars, pointers and struct values (Recipe N
   step 2), none written in the block, in a struct as Recipe N steps 2 and 3 do: one
   member per variable, the variable's exact type and name, a top-level `const` or
   `magick_restrict` left off. Neither `v` nor a variable of step 2 is ever a member.
4. Cut the block into a new `static MagickBooleanType` function directly above the
   caller, taking `const Name *name` first and then `v` as a pointer, then any pointer
   the block writes through that step 3 leaves out of the struct (an image, `exception`,
   as Recipe N step 2 allows), at most four parameters in all. At the top of its
   body declare one `const` local per member, as Recipe N step 4 does (or write
   `name->member` at every use, its member form), then the variables of step 2. In the
   text write `*v` for every use of `v`, reads included (Recipe V step 4); each `goto L;`
   or `return(c);` becomes `return(MagickFalse);`, one for one; the function ends with
   `return(MagickTrue);`. Nothing else in the text changes.
   A variable of the caller that the block reads on entry and then writes, and that the
   caller does not read after the block before writing it again (a list cursor `next`
   that the block's loop advances), is neither a member nor `v`: it goes by value, right
   after the struct, its text unchanged (Recipe E step 4, Recipe TW step 2). Its writes
   land in the new function's copy, which nothing reads once the function returns;
   check by reading every path out of the block, the exits included.
5. In the caller, declare the `Name` local among the declarations at the top of the block
   that holds the call. Directly before the call, assign every member from the variable
   of its name, in declaration order, with nothing between the assignments and the call.
6. Where the block stood, write `if (Helper(&name,&v) == MagickFalse) goto L;` (or
   `return(c);`, or Recipe V step 7's `{ S return(v); }` and its other forms, the exit's
   text unchanged), and check every member and argument by reading.

**Before:**

```c
  if ((fabs(segment->x1-segment->x2) >= MagickEpsilon) ||
      (fabs(segment->y1-segment->y2) >= MagickEpsilon))
    {
      /*
        Middle pixel.
      */
      x=CastDoubleToSsizeT(ceil(segment->x1-0.5));
      y=CastDoubleToSsizeT(ceil(segment->y1-0.5));
      u=GetCacheViewVirtualPixels(context->u_view,x,y,1,1,context->exception);
      ...
      if (IsPlasmaPixelMissing(u,v,q) != MagickFalse)
        return(MagickTrue);
      SetPlasmaPixelChannels(context->image,context->random_info,u,v,q,plasma);
      status=SyncCacheViewAuthenticPixels(context->image_view,context->exception);
    }
```

**After:**

```c
static MagickBooleanType SetPlasmaMiddlePixel(const PlasmaMidpoint *midpoint,
  MagickStatusType *status)
{
  const PlasmaContext
    *const context = midpoint->context;
  ...
  ssize_t
    x,
    y;

  if ((fabs(segment->x1-segment->x2) >= MagickEpsilon) ||
      (fabs(segment->y1-segment->y2) >= MagickEpsilon))
    {
      ...
      if (IsPlasmaPixelMissing(u,v,q) != MagickFalse)
        return(MagickFalse);
      SetPlasmaPixelChannels(context->image,context->random_info,u,v,q,plasma);
      *status=SyncCacheViewAuthenticPixels(context->image_view,context->exception);
    }
  return(MagickTrue);
}
...
  midpoint.context=context;
  ...
  midpoint.plasma=plasma;
  if (SetPlasmaMiddlePixel(&midpoint,&status) == MagickFalse)
    return(MagickTrue);
```

**Careful:**

- The three parts keep the arguments of the recipes they come from. The caller jumps or
  returns from the same point as before, after every write the block made to `v` has
  landed through the pointer, and goes on with the next statement where the block fell
  through (Recipe V). The members are copied directly before the call from variables the
  block does not write (Recipe N).
- A variable of step 2 holds nothing the caller reads: the block writes it before its
  own first read, and the caller writes it again before its next read. The new function's
  own copy therefore changes no value anyone sees. A variable the block reads before
  writing it (a cursor carried in) is not one: skip the block.
- `return(c);` with `c` itself `MagickTrue` becomes `return(MagickFalse);` like any
  other constant: the helper's result says whether the block fell through, and the
  caller returns `c` when it did not.
- The failure paths are mostly cache fetches that no case makes fail: the oracle checks
  the fall-through path; the jumps are checked by reading, each against its
  `return(MagickFalse);`, and `refactor_guard.py --calls` moves only `+2`.
- Under step 1's Recipe V step 7 exits the argument is V step 7's: the cleanup and the
  `return` stay the caller's own text, run at the point where the block ran them, after
  every statement the block ran before that exit and every write to `v` has landed
  through the pointer, on variables the block does not write. A struct member is read
  in the caller before the call, so the cleanup may write one (`resize_filter`) after
  the helper returns; the block itself writes no member (step 3). With one exit,
  `refactor_guard.py --calls` moves only `+2`.

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for the new function,
nothing else moved), and steps 1, 2 and 6 checked by reading.

**Provenance:** 2026-10-30, from `visual-effects.c`'s `PlasmaImageProxy`, whose middle
pixel leaves through `return(MagickTrue);` when a fetch fails, writes `status` and the
temporaries `x`, `y`, `u`, `v` and `q`, and reads the context, the segment, both
midpoints and the plasma amplitude: Recipe V gives the helper six parameters. A probe:
bumps 5 to 4, 3.12 to 3.19. Its bottom and top pixels have the same shape. Added by
Claude Code (Claude Opus 5.5) during task W08, under the mandate of 2026-10-24.

---

## Recipe Z - Extract Selection with Out-Parameters Taking an Input Struct

**Use when:** CodeScene reports *Complex Method*, *Large Method* or *Bumpy Road Ahead*,
and the block that would fix it meets Recipe O in every respect but one: with the
variables it only reads as parameters and the locals it sets as pointers, the new
function would take five or more arguments (*Excess Number of Function Arguments*, which
the pre-commit gate refuses on a new function). The typical case is a colorspace
`switch` that converts a start and a stop colour together, reading the image, both
source colours and an illuminant.

**Not covered by:** Recipe O, which passes every input as a parameter (O then B is two
commits, of which the first adds a finding); Recipe N, whose block writes one value or
through a pixel pointer, and keeps written variables out of its struct; Recipe Q, whose
block is a per-pixel accumulation in a parallel loop body or in a function with no
OpenMP region, not a selection beside a parallel loop; Recipe Y, whose block leaves
through a jump or a `return`.

**How:**

1. The block meets Recipe O steps 1 to 3: it lies outside every OpenMP region (or in a
   parallel loop body under O step 1's condition) with no `#pragma` line; every variable
   it writes is a plain local of the caller whose address is not taken elsewhere, at
   most four of them; it contains no `return`, `goto`, or `continue` or `break` that
   leaves it.
2. Put the variables the block only reads, scalars, pointers and struct values (Recipe N
   step 2), none written in the block, in a struct as Recipe N steps 2 and 3 do: one
   member per variable, the variable's exact type and name, a top-level `const` or
   `magick_restrict` left off. A written variable is never a member.
3. Cut the block into a new `static void` function directly above the caller, taking
   `const Name *name` first, then one pointer per written variable, in the order the
   block first writes them. At the top of its body declare one `const` local per member,
   as Recipe N step 4 does, or write `name->member` at every use of a member.
4. In the block's text write each written variable `v` as `(*v)` at every use, reads
   and address-of included (`&start.red` becomes `&(*start).red`), as Recipe O step 5
   does. Nothing else in the text changes.
5. In the caller, declare the `Name` local among the declarations at the top of the
   block that holds the call. Directly before the call, assign every member from the
   variable of its name, in declaration order, with nothing between the assignments and
   the call.
6. Call it where the block stood with `&name` and `&v` for each written variable, at
   most four arguments in all; check every member and argument by reading.

**Before:**

```c
  start=(*start_color);
  stop=(*stop_color);
  switch (image->colorspace)
  {
    case HCLColorspace:
    {
      ConvertRGBToHCL(start_color->red,start_color->green,start_color->blue,
        &start.red,&start.green,&start.blue);
      ConvertRGBToHCL(stop_color->red,stop_color->green,stop_color->blue,
        &stop.red,&stop.green,&stop.blue);
      break;
    }
    ...
    case LabColorspace:
    {
      ConvertRGBToLab(start_color->red,start_color->green,start_color->blue,
        illuminant,&start.red,&start.green,&start.blue);
      ...
    }
    default:
    {
      start.red*=QuantumScale;
      ...
    }
  }
  start.red*=(double) QuantumRange;
```

**After:**

```c
typedef struct _ColorThresholdRange
{
  Image
    *image;

  const PixelInfo
    *start_color,
    *stop_color;

  IlluminantType
    illuminant;
} ColorThresholdRange;

static void ConvertColorThresholdRange(const ColorThresholdRange *range,
  PixelInfo *start,PixelInfo *stop)
{
  switch (range->image->colorspace)
  {
    case HCLColorspace:
    {
      ConvertRGBToHCL(range->start_color->red,range->start_color->green,
        range->start_color->blue,&(*start).red,&(*start).green,&(*start).blue);
      ...
    }
    ...
  }
}
...
  start=(*start_color);
  stop=(*stop_color);
  range.image=image;
  range.start_color=start_color;
  range.stop_color=stop_color;
  range.illuminant=illuminant;
  ConvertColorThresholdRange(&range,&start,&stop);
  start.red*=(double) QuantumRange;
```

**Careful:**

- The two parts keep the arguments of the recipes they come from. Every write of the
  block stays a write through the pointer and nothing more, so a field a path leaves
  alone keeps the caller's value, and reads go through the pointer too (Recipe O). The
  members are copied directly before the call from variables the block does not write,
  so each holds the value the block's own read gave (Recipe N).
- `(*v).m` is `v->m`; the parenthesised form is the literal `*v` of step 4, which keeps
  the text a one-for-one substitution the reader can check.
- Prefer the member form when the `const` locals would push the new function over
  *Large Method*'s 70 lines: in `ColorThresholdImage` they did (72 lines, 4.42), the
  member form did not (4.58). Re-wrap a line the prefix pushes past 80 columns; the
  tokens and their order stay as they were.
- Out-parameters must name different objects: two locals of the caller whose addresses
  are not taken elsewhere cannot alias, nor alias a member's pointee.
- The function must be oracle-verified, with a case for each branch the score rests on
  where one exists; the guard sees the moved literals but not a write that went missing.

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for the new function,
nothing else moved), and steps 1, 2 and 6 checked by reading.

**Provenance:** 2026-10-30, from `threshold.c`'s `ColorThresholdImage`, whose colorspace
`switch` sets `start` and `stop` and reads the image, both source colours and the
illuminant: six arguments under Recipe O. A probe: cc 20 to 13, 146 to 92 lines, 4.46 to
4.58. Added by Claude Code (Claude Opus 5.5) during task W08, under the mandate of
2026-10-24.

---

## Recipe I - Extract Block Returning Success Through a Struct of Out-Pointers

**Use when:** CodeScene reports *Deep, Nested Complexity*, *Bumpy Road Ahead*, *Complex
Method* or *Large Method*, and the block that would fix it meets Recipe V (steps 1 to 6)
in every respect but one: it writes two, three or four variables of the caller that the
caller reads after it, and with each as a pointer (Recipe O) the new function would take
five or more arguments (*Excess Number of Function Arguments*, which the pre-commit gate
refuses on a new function). The typical case is a parser step that, on success, rewrites
several of the caller's parse-state variables together (a function code, its name string,
a qualifier, an expected delimiter) and on an internal error throws and returns
`MagickFalse`.

**Not covered by:** Recipe E, whose step 4 skips a block writing more than one outer
variable and says not to invent an out-parameter struct (*Careful* says how this recipe
differs, deliberately); Recipes V and Y, whose block writes at most one variable of the
caller (V step 2, Y step 1); Recipes O and Z, which take up to four written variables by
pointer but refuse a block containing `return` or `goto` (O step 3, Z step 1); Recipe Q,
which also refuses `return` and `goto` (K step 1). V or Y with every written variable as
its own pointer is Recipe O's form plus an exit, but gives five or more parameters here.

**How:**

1. The block meets Recipe V steps 1, 3 and 6: it lies outside every OpenMP region and has
   no `#pragma` line; it leaves the function only through `goto L;` to one label, or only
   through `return(c);` of one constant `c` (written `return c;` in some files); no
   `continue` or `break` leaves it. In a `void` function, a plain `return;` at every
   exit stands for `return(c);`: each becomes `return(MagickFalse);` and the caller
   writes `return;` (step 7). Recipe V step 7's cleanup form is not this recipe,
   except as step 8 admits it.
2. List the variables of the caller the block writes: two to four, each a plain local of
   the caller or a by-value parameter of it (`FunctionE fe`), none a global, a `static`
   or an array, and none whose address `&v` the caller takes anywhere (check by
   `grep`), except to fill a member of another helper's targets struct under this
   recipe, Recipe LB step 8 or Recipe Q step 8, filled directly before that helper's
   call or declared `const` with the addresses as its initialiser (Recipe Q step 8's
   `const` form), and read by nothing but that call (Recipe LB step 2's "helpers of
   this recipe"), or as an argument of calls that keep no copy
   of it (checked by reading the callee: `GetPixelInfo(image,&pixel)` fills `pixel` and
   returns), as Recipe V step 2 and Recipe LB step 8 allow; in the block, `&v` becomes
   `v`. Writes through pointers the block already uses stay as they are; variables
   the block declares move with it; a temporary it writes before reading, and the caller
   does not read after it, is declared in the new function (Recipe Y step 2).
3. Directly above the new function declare a `typedef struct` with one member per
   written variable: a pointer to the variable's exact type, with the variable's name
   (`const char *funStr` gives `const char **funStr`). It holds addresses only: no value,
   no input, nothing else.
4. Cut the block into a new `static MagickBooleanType` function directly above the
   caller. It takes the variables the block only reads as parameters (Recipe V step 4),
   or in one `const` input struct (Recipe Y steps 3 to 5) when they would not fit, then
   `const Name *targets` last: at most four parameters in all, otherwise skip the block.
5. At the top of its body declare, per member and in member order,
   `T *const v = targets->v;`. In the block's text write each written variable `v` as
   `*v` at every use, reads included (`(*v)++` for `v++`), as Recipe O step 5 does; each
   `goto L;` or `return(c);` becomes `return(MagickFalse);`, one for one; the function
   ends with `return(MagickTrue);`. Nothing else in the text changes.
6. In the caller, declare the `Name` local among the declarations at the top of the
   block that holds the call. Directly before the call assign every member its
   variable's address, `targets.v=(&v);`, in member order, with nothing between the
   assignments and the call.
7. Where the block stood, write `if (Helper(...,&targets) == MagickFalse) goto L;` (or
   `return(c);`, or `return;`), and check every argument, member and address by
   reading. A block of another function of the file whose step-5 text would equal,
   under Recipe D's renaming (checked by script), the body of a function this recipe
   already made calls that function instead of a second copy, with its own targets
   local and its own exit, one commit per caller, as Recipe W step 7 and Recipe LB
   step 6 allow.
8. A block whose every exit is the same call of a function-local cleanup-and-throw
   macro `M(...)`, whose expansion is a run of cleanup statements ending in
   `return(c);` of one constant `c` (`Classify`'s `ThrowClassifyException`, which frees
   the cluster list and the `squares` table, then `ThrowBinaryException`), moves with
   the macro call left in the caller, as Recipe V step 7 leaves `S`: check by script
   that every exit is the same call with the same arguments; each becomes
   `return(MagickFalse);`, and the caller writes `if (Helper(...,&targets) ==
   MagickFalse)` followed by that call, unchanged. Unlike V step 7, the macro may read
   and write the written variables (step 2): their writes have landed through the
   members before the helper returns. In this form, a written variable whose address
   the caller passes only as an argument of calls that keep no copy of it (`memset`, a
   `static` function of the file that does not store its pointer parameter, checked by
   reading) still meets step 2; in the block, `&v` becomes `v`.
   The same holds when every exit is the same run of cleanup statements `S` followed by
   `return(c);` of one constant `c`, Recipe V step 7's `S return(c);`: each becomes
   `return(MagickFalse);`, and the caller writes `if (Helper(...,&targets) ==
   MagickFalse) { S return(c); }`, `S` copied unchanged (checked by script to be
   identical in every copy). Here, as in V step 7, `S` neither reads nor writes a
   written variable (step 2).
   So does V step 7's single `return(<expr>);` (`return(DestroyKernelInfo(kernel));`),
   left in the caller as `{ S return(c); }` is; its expression reads no written
   variable. So does V step 7's side-effect-free `S return(e);`, whose `e` reads no
   written variable.

**Before:**

```c
    if (IsCoordFunction(fe) != MagickFalse) {

      coordQual = (GetCoordQualifier (pfx, (int) fe) == 1) ? MagickTrue : MagickFalse;

      if (coordQual) {

        /* Remove last element, which should be fP */
        ElementT * pel = &pfx->Elements[pfx->usedElements-1];
        if (pel->operator_index != fP) {
          (void) ThrowMagickException (
            pfx->exception, GetMagickModule(), OptionError,
            "Bug: For function", "'%s' last element not 'p' at '%s'",
            funStr, SetShortExp(pfx));
          return MagickFalse;
        }
        chQual = pel->channel_qual;
        expChLimit = (pel->is_relative) ? ']' : '}';
        pfx->usedElements--;
        if (fe == fU) fe = fUP;
        else if (fe == fV) fe = fVP;
        else if (fe == fS) fe = fSP;
        funStr = Functions[fe-(int) FirstFunc].str;
      }
    }
```

**After:**

```c
typedef struct _CoordQualifierTargets
{
  FunctionE
    *fe;

  const char
    **funStr;

  PixelChannel
    *chQual;

  char
    *expChLimit;
} CoordQualifierTargets;

static MagickBooleanType ApplyCoordQualifier (FxInfo * pfx,
  const MagickBooleanType coordQual,const CoordQualifierTargets *targets)
{
  FunctionE
    *const fe = targets->fe;

  const char
    **const funStr = targets->funStr;

  PixelChannel
    *const chQual = targets->chQual;

  char
    *const expChLimit = targets->expChLimit;

  if (coordQual) {
    ...
    if (pel->operator_index != fP) {
      (void) ThrowMagickException (
        ...
        *funStr, SetShortExp(pfx));
      return(MagickFalse);
    }
    *chQual = pel->channel_qual;
    *expChLimit = (pel->is_relative) ? ']' : '}';
    pfx->usedElements--;
    if (*fe == fU) *fe = fUP;
    else if (*fe == fV) *fe = fVP;
    else if (*fe == fS) *fe = fSP;
    *funStr = Functions[*fe-(int) FirstFunc].str;
  }
  return(MagickTrue);
}
...
    if (IsCoordFunction(fe) != MagickFalse) {
      CoordQualifierTargets targets;

      coordQual = (GetCoordQualifier (pfx, (int) fe) == 1) ? MagickTrue : MagickFalse;

      targets.fe=(&fe);
      targets.funStr=(&funStr);
      targets.chQual=(&chQual);
      targets.expChLimit=(&expChLimit);
      if (ApplyCoordQualifier (pfx, coordQual, &targets) == MagickFalse)
        return MagickFalse;
    }
```

**Careful:**

- A call outside the block that only reads or fills through `&v` while it runs and
  keeps no copy of the address gives `v` no second name, so no two pointers the helper
  holds name one object (step 2); inside the block, `v` is the address `&v` was, the
  same argument of the same type.
- This is Recipe O's argument with the pointers carried in one struct, plus Recipe V's
  exit. Each `*v` in the helper is the caller's own object `v`, so every read sees the
  caller's current value and every write lands in the caller's variable, the same
  operations in the same order; a variable a path leaves alone keeps the caller's value.
  On a failure path every write made before the exit has landed, the helper returns at
  once, and the caller jumps or returns `c` from the same point, running nothing in
  between (Recipe V); on the success path the caller goes on with the next statement.
- How this differs from the out-parameter struct that Recipe E step 4 forbids, and why
  it is safe: the caller's variables are not moved into a struct; they stay its own
  locals, under their own names, read and written directly everywhere else. The struct
  holds only their addresses, set directly before the call and dead after it; the helper
  receives it as `const` and binds each member once to a `*const` local, so no pointer
  is ever reseated. Nothing is copied in or back. The alternative, a struct of values
  that the caller copies back, would need the copy-back on every exit, a statement
  each, and a reader would have to check that none is missing and that a value the block
  left alone comes back unchanged; with addresses there is nothing to miss.
- The pointers name different objects and cannot alias each other or a parameter's
  pointee: each is the address of a distinct local whose address the caller takes
  nowhere else (step 2). A by-value parameter is such a local: C passes it by copy, so
  `&fe` is the caller's own object and the caller's caller sees no change.
- `return MagickFalse;` with `c` itself `MagickFalse` becomes `return(MagickFalse);` like
  any other constant: the helper's result says whether the block fell through, and the
  caller returns `c` when it did not. A warning thrown with `GetMagickModule()` now names
  the helper as its location, which the oracle normalises.
- The failure paths are mostly internal checks no case reaches: the oracle checks the
  fall-through path, with a case that takes the block's writes where one exists; each
  exit is checked by reading against its `return(MagickFalse);`, and
  `refactor_guard.py --calls` moves only `+2`.
- Under step 8 the macro call does not move: the caller runs the same cleanup and
  throw, on its own variables, at the same point. On a failure path the helper has run
  every statement the block ran before that exit, its writes landed in the caller's
  variables through the members, and it returns at once; the caller runs nothing
  between the call and the macro, so the macro reads the values it read before (the
  list `head` starts, the `cluster` the failed allocation left NULL). Whatever the
  macro writes afterwards is the caller's own business, as before; the helper's
  pointers are dead by then. A variable whose address went only to `memset` or to a
  callee that keeps no copy has no second name when the helper runs, so its member
  aliases nothing; `&v` in the block and `v` (the member's value) are the same address,
  so the callee receives the same argument.
- Step 8's plain run `S return(c);` is Recipe V step 7's argument with Recipe I's
  members: `S` stays the caller's own text, run on the caller's own variables at the
  same point (the helper returns at once, and the caller runs nothing between the call
  and `S`); `S` reads none of the written variables, so it sees the values it saw
  before whatever the helper wrote through the members, and the helper takes none of
  the variables `S` writes (`bounds`, `disposals`, `prev_image` in
  `OptimizeLayerFrames`). On the success path `S` runs in neither version.
- Step 2's exception for another helper's targets struct: that struct is the caller's
  local, filled directly before its own call (or declared `const` with the addresses as
  its initialiser: an address does not change while its variable lives) and read by nothing but
  that call, so its copy of `&v` is used only while that helper runs, which is never
  while this one runs (C calls do not overlap here: no OpenMP region, step 1). The
  struct may be of Recipe I, LB step 8 or Q step 8: all three hold addresses only and
  hand them to their own helper alone. While this helper runs, its member is
  the only pointer to `v` in use, and no two of its own members name one object.
- A plain `return;` of a `void` function (step 1) is `return(c);` without a value: on
  a failure path the helper has run every statement the block ran before that exit and
  returns at once, and the caller's `return;` ends the caller at the same point, after
  the same writes, with nothing to return; on the success path the caller goes on with
  the next statement. The `void` caller may itself be a loop body moved by Recipe E
  step 7, whose `return;` was the loop's `continue;`.
- Under step 7's reuse the existing function's body is the text step 5 would cut from
  the second block, so calling it is calling the function the step would have made, a
  copy the file then does not carry; the second caller fills its own targets struct
  with the addresses of its own locals, which step 2 checks for that caller.
- More than four written variables, two labels, two different constants, or a written
  variable whose address the caller takes elsewhere: skip the block.

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for the new function,
nothing else moved), and steps 1, 2, 6 and 7 checked by reading.

**Provenance:** 2026-10-30, from `fx.c`'s `GetFunction`, whose coordinate-qualifier block
(`u.p[...]`, `v.p{...}`) is the file's last *Deep, Nested Complexity* (depth 4): it writes
`fe` (a by-value parameter), `funStr`, `chQual` and `expChLimit`, and returns
`MagickFalse` when the last element is not `p`. Recipes V and Y allow one written
variable, O, Z and Q no `return`, and E step 4 refuses it; a conditional return left in
the caller stays at depth 4, so the whole `if (coordQual)` moves. A probe: the function
leaves Deep, Nested Complexity, fx.c 3.08 to 3.23. Added by Claude Code (Claude Opus 5.5)
during task W09, at the owner's request of 2026-10-30. Step 8's plain cleanup run
(2026-10-30, task W09): `layer.c`'s `OptimizeLayerFrames`, whose duplicate-frame trial
(nesting 4) writes `dup_image`, `dup_bounds` and `cleared` and, when the clone fails,
releases `bounds` and `disposals`, destroys `prev_image` and returns
`(Image *) NULL`.

---

## Recipe LB - Extract Block Returning Success for a Loop Break

**Use when:** CodeScene reports *Deep, Nested Complexity*, *Bumpy Road Ahead*, *Brain
Method*, *Complex Method* or *Large Method*, and the block that would fix it leaves an
enclosing loop through `break;`: typically a pixel fetch inside a scan loop that gives up
on the loop when the cache returns NULL (`if ((p == (const Quantum *) NULL) || (q ==
(Quantum *) NULL)) break;`), as `FloodfillPaintImage`'s span scans do.

**Not covered by:** Recipe E, whose step 7 admits `break;` only of a `switch` and
`continue;` of a loop, never a `break` that ends a loop; Recipes V, Y and I, whose block
leaves only through `goto L;` or `return(c);` and contains no `break` that leaves it (V
step 1); Recipes O, T, K, J, N, Q, U, Wa and Z, which refuse a block with an exit of its
own. A `break` cannot be left in the caller as V leaves its jump: it would still sit at
the block's depth.

**How:**

1. The block lies outside every OpenMP region and contains no `#pragma` line. The
   innermost loop or `switch` around the block is a loop `Lp`, and every `break;` in the
   block that leaves it ends `Lp`. It contains no `return`, no `goto`, and no `continue`
   that leaves it; a `break` or `continue` of a loop nested inside the block stays as it
   is.
2. The block writes at most one variable `v` of the caller that the caller reads after
   it, a plain local whose address the caller takes only as the `v` argument of helpers
   of this recipe (Recipe V step 2). A temporary it writes before reading, and the caller
   does not read after it before writing it again, is declared in the new function
   (Recipe Y step 2).
3. Cut the block into a new `static MagickBooleanType` function directly above the
   caller. Each `break;` that ended `Lp` becomes `return(MagickFalse);`, one for one; the
   function ends with `return(MagickTrue);`. Write `*v` for every use of `v`, reads
   included (`(*v)++` for `v++`). Nothing else in the text changes.
4. Pass the variables the block only reads by value and `v` as a pointer, last, at most
   four parameters (Recipe V step 4); or put them in one `const` input struct as Recipe Y
   steps 3 to 5 do, the struct first, a read-only scalar the struct leaves out by value
   after it, and `v` last, at most four parameters in all. The member form
   `name->member` (Recipe N step 4) keeps two helpers of one caller from repeating the
   same `const` locals.
5. Where the block stood, write `if (Helper(...) == MagickFalse) break;`. It stands where
   the block stood, inside the same loops and `switch`es, so its `break` ends `Lp`.
6. Two blocks of one caller whose groups of read variables are the same take the same
   struct type; the caller fills its one local directly before each call (Recipe Y step
   5). Check every member and argument by reading. A block whose step-3 text would equal,
   under Recipe D's renaming (checked by script), the body of a function Recipe LB, Y or
   W already made in the file calls that function instead of a second copy, with its own
   struct local and its own `break;`, as Recipe W step 7 allows.
7. A block whose every exit is the same run of statements `S` followed by `continue;`
   of `Lp` (`i--; continue;`, which retries the iteration) moves the same way, with `S`
   left in the caller as Recipe V step 7 leaves its cleanup: each `S continue;` becomes
   `return(MagickFalse);`, one for one, and the caller writes
   `if (Helper(...) == MagickFalse) { S continue; }`, `S` copied unchanged (checked by
   script to be identical in every copy). The block writes no variable `S` reads or
   writes, and contains no `break` that leaves it; steps 1 to 4 hold as written, with
   `S continue;` in place of `break;`. A bare `continue;` is the form with `S` empty.
8. A block that writes two to four variables of the caller that the caller reads after
   it (step 2's `v`, more than one) passes their addresses in one `const` struct,
   Recipe I's form (steps 3, 5 and 6): a `typedef struct` directly above the new
   function with one member per written variable, in the order the block first writes
   them, a pointer to the variable's exact type under its name, holding addresses only;
   the function takes the input struct (step 4), then `const Targets *targets`, and
   binds each member once at the top of its body, `T *const v = targets->v;`, each `v`
   then written `*v` as step 3 says. The caller assigns `targets.v=(&v);` per member
   directly before the call (Recipe I step 6), or declares the local `const` with the
   addresses as its initialiser (Recipe Q step 8's `const` form). Each written variable
   meets Recipe I step 2, except that one whose address the caller passes elsewhere only
   as an argument of calls that keep no copy of it (`SetMatrixElement`'s `value`, which
   it copies from and does not store; checked by reading the callee) still qualifies,
   as in Recipe I step 8. Steps 1, 3, 5 and 6 hold as written.

**Before:**

```c
    do
    {
      if (skip == MagickFalse)
        {
          if (x < (ssize_t) image->columns)
            {
              p=GetCacheViewVirtualPixels(image_view,x,y,(size_t)
                ((ssize_t) image->columns-x),1,exception);
              q=GetCacheViewAuthenticPixels(floodplane_view,x,y,(size_t)
                ((ssize_t) image->columns-x),1,exception);
              if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
                break;
              ...
              x=FillFloodplaneSpan(&scan,p,q,x);
              status=SyncCacheViewAuthenticPixels(floodplane_view,exception);
              if (status == MagickFalse)
                break;
            }
          PushSegmentStack(y,start,x-1,offset);
```

**After:**

```c
static MagickBooleanType FillFloodplaneRun(const FloodfillRow *row,ssize_t *x)
{
  ...
  if (*x < (ssize_t) row->image->columns)
    {
      p=GetCacheViewVirtualPixels(row->image_view,*x,row->y,(size_t)
        ((ssize_t) row->image->columns-*x),1,row->exception);
      ...
      if ((p == (const Quantum *) NULL) || (q == (Quantum *) NULL))
        return(MagickFalse);
      ...
      *x=FillFloodplaneSpan(&scan,p,q,*x);
      status=SyncCacheViewAuthenticPixels(row->floodplane_view,row->exception);
      if (status == MagickFalse)
        return(MagickFalse);
    }
  return(MagickTrue);
}
...
          row.image=image;
          ...
          row.exception=exception;
          if (FillFloodplaneRun(&row,&x) == MagickFalse)
            break;
          PushSegmentStack(y,start,x-1,offset);
```

**Careful:**

- A `break;` that ends `Lp` runs nothing else of the loop body and skips `Lp`'s
  condition; so does the caller's `break;` right after the helper returns
  `MagickFalse`. On that path the helper has run every statement the block ran before
  that `break`, its write to `v` landed through the pointer, and it returns at once;
  the caller runs nothing between the call and its `break`. On the fall-through path
  the helper returns `MagickTrue` and the caller goes on with the next statement, as
  the block fell through. This is Recipe V step 6 with `break;` in place of
  `return(c);`.
- Under step 6's reuse the existing function's body is the text step 3 would cut from
  this block, `return(MagickFalse);` in place of each `break;`, so calling it is calling
  the function step 3 would have made; the caller's `break;` after it is step 5's.
- Step 1 matters: a block inside a `switch` inside the loop has its `break` end the
  `switch`, not the loop, and a `break` of a loop nested inside the block ends that
  loop, which moves with the block unchanged. Read each `break` against its loop.
- A temporary holds nothing the caller reads (Recipe Y's argument); check it by reading
  every use in the caller, on the path after `Lp` ends as well as the fall-through. A
  `status` the block writes and then tests, which the caller overwrites after the loop
  before any read, is such a temporary.
- The failure paths are mostly cache fetches no case makes fail: the oracle checks the
  fall-through path; each `break` is checked by reading against its
  `return(MagickFalse);`, and `refactor_guard.py --calls` moves only `+2` per helper.
- Under step 7 the argument is the first point's with `continue;` for `break;`: a
  `continue;` of `Lp` runs nothing else of the body and goes on to `Lp`'s increment and
  condition, and so does the caller's, after the helper has returned `MagickFalse` and
  the caller has run `S`. `S` runs at the same point as before, after every statement
  the block ran before that exit, on the caller's own variables; the block writes none
  of them, so `S` reads the values it read before. When `S` writes `Lp`'s counter
  (`i--;`, so that the increment repeats the same `i`), the counter goes to the helper
  by value and only the caller writes it.
- Under step 8 the variables stay the caller's own locals, read and written directly
  everywhere else; only their addresses travel, fixed before the call and dead after
  it, bound once to `*const` locals, so each `*v` is the object `v` named, in the same
  order (Recipe I's argument). On the `return(MagickFalse);` path every write the block
  made before its `break` has landed through its member before the caller's `break`.
  A call that only copies from or into `&v` while it runs and keeps no copy of the
  address (`GetMatrixElement` and `SetMatrixElement` `memcpy` into or from `value`, or
  write it to a file, and return) gives `v` no second name, so no two pointers the
  helper holds name one object, which is what Recipe O step 2 protects.

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for each new
function, nothing else moved), and steps 1, 2 and 5 checked by reading (step 8: every
member and address, and each callee that receives `&v` elsewhere).

**Provenance:** 2026-10-30, from `paint.c`'s `FloodfillPaintImage`, whose last two
depth-4 sites, the right-hand fill and the skip scan inside the segment loop's
`do`-loop, each end the `do`-loop through `break;` when a fetch or a sync fails, the
plateau W08 recorded. A probe of both: Brain Method and Deep, Nested Complexity leave,
5.10 to 5.77. Added by Claude Code (Claude Opus 5.5) during task W09, under the mandate
of 2026-10-24. Step 7 (2026-10-30, task W09): `statistic.c`'s `GetImageStatistics`,
whose depth check (nesting 5, the file's deepest site) raises a channel's depth and
retries the pixel through `i--; continue;` of the channel loop. Step 8 (2026-10-30,
task W09): `vision.c`'s `ConnectedComponentsImage`, whose label pass (nesting 4) labels
each pixel, writing `status`, the object count `n` and the row cursors `p` and `q`, and
ends the column loop through `break;` when the count reaches `MaxColormapSize`.

---

## Recipe SD - Split a Selection at its Default

**Use when:** CodeScene reports *Complex Method*, *Bumpy Road Ahead* or *Deep, Nested
Complexity*, and the block that would fix it is a `switch` that Recipe O or Z could move
whole, except that the new function would itself be a *Complex Method* (cc 9 or more: one
per case label, `default` included), which the pre-commit gate refuses on a new function.
The typical case is an operator `switch` of seven or eight labels whose cases fall into
two families, such as the unary and the binary complex operators.

**Not covered by:** Recipes O and Z, which move a selection whole, so the new function
keeps every label's branch; Recipe E step 7's `switch` form, which moves part of one case,
not a group of cases; Recipe D, which needs two copies of a block. Moving one case at a
time (E, O or Z on its body) leaves every label in the caller, and the cc with it.

**How:**

1. The `switch` has a `default:` label, and its controlling expression `e` is a
   variable, or an expression of variables with no call, assignment or increment, that
   no statement inside the `switch` writes. Every *group* (a run of labels and the body
   after them) ends with a `break;` of this `switch` and contains no other `break` of
   it, no `continue`, `return` or `goto`: no group falls through into another.
2. Choose the groups that move: the group carrying `default:`, and others beside it.
   Together their bodies meet Recipe O steps 1 to 3 (OpenMP, written variables, exits),
   as one selection does.
3. Cut the moved groups, labels and bodies, in their order, into a new `static void`
   function directly above the caller, whose body is `switch (e) { ... }` with the same
   text of `e`. Its parameters and the text of the bodies are those Recipe O steps 4 and
   5 give, or Recipe Z steps 2 to 4 when O's would number five or more.
4. Where the `default` group stood, write the group `default:` alone, whose body is a
   compound block holding the call: Recipe O step 6's call, or Recipe Z's struct local
   declared at the top of that block, its members assigned and the call (Z steps 5 and
   6), followed by `break;`.
5. The kept groups stay where they were, unchanged. Check by reading that every label
   of the original `switch` is in exactly one of the two, `default` in the new one, and
   check every member and argument as Recipe O step 6 or Z step 6 says.
6. Locals the caller derives from its parameters. When the moved groups read `const`
   locals declared at the top of the caller's body whose initialisers read only the
   caller's parameters (`const ssize_t e = neighbours->e, ...`), and neither the caller
   before the `switch` nor the moved groups write those parameters or anything their
   initialisers read through them, the new function may take the caller's parameters
   instead (the same types and names, in the same order), and declare at the top of its
   body those of the locals the moved groups read, each with its own initialiser's text
   unchanged. Step 4's call then passes the caller's parameters by name. A local the
   caller no longer reads after the cut leaves the caller's declaration. The moved groups
   need not be adjacent in the `switch`, and the step may be repeated on the same
   `switch`, whose `default` group is then the earlier call.
7. A `switch` without `default`. When the `switch` has no `default:` label, step 1
   holds without it, and the groups that move are any whose bodies meet step 2. The new
   function's `switch` ends with the group `default: break;`, and step 4's `default:`
   group, holding the call, stands in the caller's `switch` where the first moved group
   stood. A later step on the same `switch` (step 6's chain) moves the caller's
   `default` group as step 2 says; the `default: break;` stays in the innermost
   function. Nothing else is added.

**Before:**

```c
        switch (op)
        {
          case AddComplexOperator:
          {
            cr=ar+br;
            ci=ai+bi;
            break;
          }
          case ConjugateComplexOperator:
          default:
          {
            cr=ar;
            ci=(-ai);
            break;
          }
          case DivideComplexOperator:
          ...
          case MagnitudePhaseComplexOperator:
          {
            cr=sqrt(ar*ar+ai*ai);
            ci=atan2((double) ai,(double) ar)/(2.0*MagickPI)+0.5;
            break;
          }
          ...
        }
```

**After:**

```c
static void ApplyUnaryComplexOperator(const ComplexUnaryOperands *unary,
  double *cr,double *ci)
{
  const ComplexOperator
    op = unary->op;

  const double
    ai = unary->ai,
    ar = unary->ar;

  switch (op)
  {
    case ConjugateComplexOperator:
    default:
    {
      (*cr)=ar;
      (*ci)=(-ai);
      break;
    }
    case MagnitudePhaseComplexOperator:
    ...
  }
}
...
        switch (op)
        {
          case AddComplexOperator:
          {
            cr=ar+br;
            ci=ai+bi;
            break;
          }
          default:
          {
            ComplexUnaryOperands
              unary;

            unary.op=op;
            unary.ai=ai;
            unary.ar=ar;
            ApplyUnaryComplexOperator(&unary,&cr,&ci);
            break;
          }
          case DivideComplexOperator:
          ...
        }
```

**Careful:**

- Why the behaviour is the same, value by value. A value of `e` that matches a kept
  label selects the same group as before, unchanged. One that matches a moved label no
  longer matches any label of the outer `switch`, so it selects `default`, whose first
  act is the call; nothing between the two evaluations writes what `e` reads (the
  member assignments only read), so the inner `switch` sees the same value and selects
  the same group, whose body writes the caller's variables through the pointers (Recipe
  O); its `break` ends the inner `switch`, the function returns, and the outer `break`
  ends the outer one, where the original `break` ended the original. A value that
  matches no label went to `default`'s body before; now it goes to the outer `default`
  and then to the inner one, which is the same body.
- `default` must move: left in the caller with its body, the values of the moved labels
  would reach it instead of their own groups. A shared run of labels
  (`case ConjugateComplexOperator: default:`) moves whole.
- Fall-through would break the argument: a group that runs into the next one would,
  after the split, run into a different group or into none. Step 1 refuses it.
- Both `switch` statements have a `default`, so `-Wswitch` warns about neither; the
  controlling expression's type and promotions are those of the same text.
- The order of groups within each `switch` is the original order; C selects by label,
  not by position, once no group falls through.
- Choose the split along what the groups read when you can (the unary operators read
  `ar` and `ai` only): the new function then takes fewer inputs.
- Under step 6 the new function evaluates each initialiser again on entry, where the
  caller evaluated it at the top of its body. The value is the same because the
  initialiser reads only parameters, passed unchanged, and objects that nothing has
  written since (in `magnify.c` the groups write the pixel buffer behind
  `magnify->destination`, never the `Hq2XNeighbours` offsets). The form costs a
  signature and a declaration where Recipe Z's struct type, its fill and its `const`
  locals cost about forty lines, which can take a file over *Lines of Code in a Single
  File*. Repeating it is a chain: a value of a label moved by the second step reaches
  the outer `default`, then the second function's `switch`; one moved by the first
  step falls through both `default`s to the first function's `switch`, each `break`
  ending its own `switch`, as the first point argues for one step. Choose the groups
  so that the new functions do not repeat each other's shape: two functions of the
  same groups' kind are reported as *Code Duplication*.
- Under step 7, a value of a kept label selects its group as before; one of a moved
  label reaches the caller's new `default`, then its own group in the new function, as
  the first point argues. A value that matches no label of the original `switch` ran no
  statement of it; now it reaches the caller's `default`, then the new function's
  `default: break;`, which runs none either, and the function returns. The added group
  is there for the compiler: a `switch` on an enumeration that names only some of its
  constants and has no `default` draws `-Wswitch` (in `-Wall`, an error in ImageMagick's
  CI), and the new function's `switch` names only the moved groups' constants. The cost
  is a diagnostic, not behaviour: neither `switch` now warns about an enumeration
  constant added later that no label names, where the original, if it named every
  constant, would have. Say so in the commit message.

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for the new function,
nothing else moved), a case for each moved label where one exists, and steps 1 and 5
checked by reading.

**Provenance:** 2026-10-30, from `fourier.c`'s `ComplexImages`, whose operator `switch`
(seven operators and `default`) sits at depth 4 inside the parallel row loop. Recipe Z on
the whole `switch` gives a helper of cc 9, a *Complex Method* the gate refuses (5.09 in a
probe); the unary group (conjugate with `default`, magnitude-phase, real-imaginary) as its
own function, then Z on the binary rest, leaves neither helper with a finding: 4.83 to
4.85 to 5.21 in a probe. Added by Claude Code (Claude Opus 5.5) during task W09, under the
mandate of 2026-10-24. Step 6 (2026-10-30, task W10): `magnify.c`'s
`Hq2XHelper`, a rule table of twenty labels reading six offsets the function derives
from its `Hq2XNeighbours` parameter. Recipe Z's struct for the nine inputs took the file
to 1,031 lines (*Lines of Code in a Single File*, 6.39 in a probe); step 6's form, twice,
takes `Hq2XHelper` out of *Complex Method* and *Large Method*: 7.48 to 7.66 to 8.13.
Step 7 (2026-10-31, task W10): `resample.c`'s `ResamplePixelColor`, whose
virtual-pixel `hit` switch names all seventeen `VirtualPixelMethod` constants and has no
`default` (cc 22 of the function's 34). Its six-label image group alone with a `default`
is a function of cc 9, so it stays; the vertical-tile and dither groups, then the corner
and horizontal-tile groups, as two chained functions: 7.30 to 7.34 to 7.37 in probes.

---

## Recipe W - Extract Block Returning Success for an Early Return

**Use when:** CodeScene reports *Bumpy Road Ahead*, *Complex Method*, *Large Method* or
*Code Duplication*, and the block that would fix it stands in the middle of a function and
leaves it on failure through `return(R);`, as `xml-tree.c`'s buffer-growing blocks do
(`return((char *) NULL);` when the reallocation fails).

**Not covered by:** Recipe E, whose step 6 covers only the `return` hidden in the throw
macros and whose step 7 only a block that ends the function; Recipe V, whose block leaves
through `goto`; Recipes O, T, K, J, N, Q and U, whose blocks may not contain `return`;
Recipe D, whose copies are moved whole and so may not leave the caller.

**How:**

1. The block lies outside every OpenMP region and contains no `#pragma` line. Every
   `return` in it is `return(R);` with the same expression `R`; it contains no `goto`,
   and no `continue` or `break` that leaves it.
2. `R` is a constant, or reads only what the block writes through a pointer the caller
   also holds (`*source`), so the caller evaluating `R` after the call gets the value the
   block's `return` would have returned.
3. The block writes no plain local of the caller, apart from locals nothing after the
   block reads (Recipe E step 4), which the new function declares, and locals it writes
   only on paths that end in `return(R);`, which go by value. Writes through pointers it
   already uses stay as they are; variables it declares move with it.
4. Cut the block into a new `static MagickBooleanType` function directly above the
   caller. Each `return(R);` becomes `return(MagickFalse);`, and the function ends with
   `return(MagickTrue);`. Nothing else in the text changes.
5. Pass what the block reads, at most four parameters. A side-effect-free expression the
   block only reads (`xml_info->tag`) may go as one value parameter, every occurrence of
   it in the text becoming that parameter.
6. Where the block stood, write `if (Helper(...) == MagickFalse) return(R);` with the
   block's own `R`, and check every argument against its parameter by reading.
7. Another copy of the block, identical but for the expression of step 5 and its own `R`,
   may call the same function the same way (Recipe D), one commit per caller.

**Before:**

```c
  if ((*length+strlen(xml_info->tag)+MagickPathExtent) > *extent)
    {
      *extent=(*length)+strlen(xml_info->tag)+MagickPathExtent;
      *source=(char *) ResizeQuantumMemory(*source,*extent,sizeof(**source));
      if (*source == (char *) NULL)
        return(*source);
    }
  *length+=(size_t) FormatLocaleString(*source+(*length),*extent,
    "<%s",xml_info->tag);
```

**After:**

```c
static MagickBooleanType ExtendXMLBuffer(char **source,const size_t *length,
  size_t *extent,const char *text)
{
  if ((*length+strlen(text)+MagickPathExtent) > *extent)
    {
      *extent=(*length)+strlen(text)+MagickPathExtent;
      *source=(char *) ResizeQuantumMemory(*source,*extent,sizeof(**source));
      if (*source == (char *) NULL)
        return(MagickFalse);
    }
  return(MagickTrue);
}
...
  if (ExtendXMLBuffer(source,length,extent,xml_info->tag) == MagickFalse)
    return(*source);
  *length+=(size_t) FormatLocaleString(*source+(*length),*extent,
    "<%s",xml_info->tag);
```

**Careful:**

- On a failure path the caller returns from the same point as before, with the same
  value: a constant, or what the block left behind the pointer.
- On the success path the function returns at the block's end and the caller goes on
  with the next statement, where the block fell through.
- A step-5 expression is evaluated once at the call instead of at each use; that is the
  same value only when nothing in the block can change what it reads. Check it by
  reading (here the block writes `*extent` and `*source`, and the expression reads
  neither).
- The failure paths are mostly allocation failures that no case reaches: the oracle
  checks the success path; the failure path is checked by reading, each `return(R);`
  against its `return(MagickFalse);`, and `refactor_guard.py --calls`.

**Evidence level:** oracle, with `refactor_guard.py --calls`, and steps 1 to 3 and 5
checked by reading.

**Provenance:** 2026-10-29, from `xml-tree.c`'s `XMLTreeTagToXML` (four buffer-growing
blocks) and `EncodePredefinedEntities` (a fifth), each leaving through `return`; a probe
of all five: 2.39 to 2.44. Added by Claude Code (Claude Opus 5.5) during task M112, under
the mandate of 2026-10-24.

---

## Recipe TW - Extract Twin Blocks into One Function Taking an Input Struct

**Use when:** CodeScene reports *Deep, Nested Complexity*, *Bumpy Road Ahead*, *Complex
Method* or *Large Method* on two functions of one file that hold the same block: the same
text once each copy's variables are renamed consistently, differing in at most one value
(`shear.c`'s `XShearImage` and `YShearImage`, whose pixel-transfer cases differ in
`x_offset`/`y_offset`, `width`/`height` and `image->columns`/`image->rows`), with five or
more inputs. Moving one copy alone lowers the score (one more function raises the file's
mean complexity while the twin keeps its findings), and a second helper of the same text
would be *Code Duplication*.

**Not covered by:** Recipe N, which moves one block, names each member after the variable
it holds and fills it only from the variable of that name; Recipe D, whose *Use when* is
a reported *Code Duplication* (these twins, whole `switch` cases of two functions, are not
reported) and which passes the inputs as parameters (seven here); Recipe W step 7 and
Recipe LB step 6, which reuse a helper of their own forms, one caller per commit; Recipe E
step 4, which declares a moved temporary in the new function only when nothing between
its declaration and the block writes it.

**How:**

1. Each copy meets Recipe N step 1: Recipe E steps 1 and 3 to 7 and the OpenMP rule, no
   `#pragma` line. Check by script that the copies are the same token sequence once each
   copy's variables are renamed to one common name per role and the differing value is
   replaced by a name (`extent`). Literals and operators are not renamed; whitespace and
   line breaks are ignored.
2. Inputs. The variables each copy only reads go into one struct, as in Recipe N steps 2
   and 3, each member of the variable's exact type and named for its role (`offset` for
   `x_offset` and `y_offset`); the two variables of a role have the same type. The
   differing value, a side-effect-free expression of one type in both copies
   (`image->columns`, `image->rows`) that nothing in the block can change, is one more
   member of that type. A variable the block reads on entry and then writes, and that
   nothing after the block reads, goes by value as a parameter (Recipe E step 4). With
   the struct and the pointers the block writes through, at most four parameters.
   A `const` struct local whose members are pointers (`CorrelationSums`), which the
   block neither assigns nor writes a member of, and whose address it takes only as `&s`
   passed to a pointer-to-`const` parameter of a callee that reads the members and
   writes through them, is one of those pointers: N step 2 (d) keeps it out of the
   struct, and it goes by value as a parameter of its exact type
   (`const CorrelationSums sums`), the block's `&sums` unchanged.
3. Temporaries. A variable of the caller that the block writes before it reads it, on
   every path, and that nothing after the block reads, is declared in the new function
   without an initialiser, even when the caller writes it before the block (a
   `GetPixelInfo(image,&source)` that stays): its value on entry is never read (Recipe J
   step 3, Recipe Q step 2). Its declaration leaves the caller only when nothing else
   there uses it.
4. Write one `static` function directly above the first caller: a `const` local per
   member it reads, as in Recipe N step 4 (or the member form), then the block's text
   with the common names, nothing else changed. Recipe E step 7's forms apply as for one
   block (a `break;` of the `switch` becomes `return;`).
5. In each caller, declare the struct local among the declarations at the top of the
   block that holds the call (inside a parallel loop's body, as Recipe N step 7 says).
   Directly before the call assign every member, in declaration order, from the variable
   that plays its role in that copy (`span.offset=y_offset;`) or from that copy's
   differing value (`span.extent=image->rows;`). The call stands where the block stood.
6. Both copies move in one commit (rule 1, several functions); the message gives the
   score of the first copy moved alone. Several twin pairs of the same two functions
   (two `switch` cases each, say) may share the commit, one function per pair, and may
   share one struct type.
7. Check by reading that every call assigns every member, each from the variable or
   expression it replaced in that copy, and that every other argument fills its
   parameter.

**Before:**

```c
/* XShearImage, inside the parallel row loop */
      case LEFT:
      {
        if (step > x_offset)
          break;
        q=p-step*(ssize_t) GetPixelChannels(image);
        for (i=0; i < (ssize_t) width; i++)
        {
          if ((x_offset+i) < step)
            { ... continue; }
          GetPixelInfoPixel(image,p,&source);
          ...
        }
        ...
        break;
      }
/* YShearImage, inside the parallel column loop: the same with y_offset and height */
      case UP:
      {
        if (step > y_offset)
          break;
        ...
      }
```

**After:**

```c
typedef struct _ShearSpan
{
  Image
    *image;

  PixelInfo
    background;

  double
    area;

  size_t
    length,
    extent;

  ssize_t
    offset,
    step;
} ShearSpan;

static void TransferPixelsForward(Quantum *magick_restrict p,PixelInfo pixel,
  PixelInfo destination,const ShearSpan *span)
{
  Image
    *image = span->image;
  ...
  const ssize_t
    offset = span->offset,
    step = span->step;

  PixelInfo
    source;
  ...
  if (step > offset)
    return;
  q=p-step*(ssize_t) GetPixelChannels(image);
  for (i=0; i < (ssize_t) length; i++)
  ...
}
...
      case UP:
      {
        span.image=image;
        span.background=background;
        span.area=area;
        span.length=height;
        span.extent=image->rows;
        span.offset=y_offset;
        span.step=step;
        TransferPixelsForward(p,pixel,destination,&span);
        break;
      }
```

**Careful:**

- The function's text is each copy's text with that copy's variables substituted, so each
  call runs the operations its copy ran on the values its copy read. The token comparison
  of step 1 is the check; a literal or operator that differs stops the step.
- The differing value is read once, at the fill, where the copy read it at each use: the
  same value only while nothing in the block can change what it reads. In `shear.c` the
  block writes pixels through `q` and its own temporaries, never the image's size.
- A by-value input (`p`, `pixel`, `destination`) is copied at the call, and the block's
  writes land in the copy, which nothing reads once the function returns. A struct value
  passed this way is read whole by the copy, so it must be set on every path to the call
  (`pixel=background;`, `GetPixelInfo(image,&destination)`), as Recipe N step 2 (a) asks
  of a member.
- A member that one of the functions does not read (`extent`, in a call of the forward
  transfer) is still assigned, from a side-effect-free expression: one struct type serves
  both functions, and step 7's count stays simple.
- Inside a parallel loop every variable the call writes is the loop body's own: the struct
  local, the by-value copies and the new function's temporaries. The OpenMP rule holds as
  for Recipe N.
- `refactor_guard.py --calls` shows each new function and the callees of the removed
  copies, down by the calls the second copy made: a deduplication, as Recipe D's.
- A struct of pointers passed by value (step 2) is a copy of the same addresses. `&sums`
  in the new function names the copy, whose members the callee reads as it read the
  caller's local, so every write through them lands in the same arrays in the same
  order; nothing writes either struct, so the copy never differs from the original.

**Evidence level:** oracle (the cases of both functions), with `refactor_guard.py --calls`,
and steps 1 (by script), 2, 3 and 7 checked by reading.

**Provenance:** 2026-10-30, from `shear.c`'s `XShearImage` and `YShearImage`: their
depth-4 sites and three of their five bumps are the two pixel-transfer cases of each
function's direction `switch`, twins under the renaming. A probe: `XShearImage`'s two cases
alone, 6.47 to 5.99; both functions' four cases, 6.47 to 6.63, both leaving *Deep, Nested
Complexity*. Added by Claude Code (Claude Opus 5.5) during task W10, under the mandate of
2026-10-24.
Step 2's struct of pointers (2026-10-31, task W10): `compare.c`'s `GetDPCSimilarity` and
`GetNCCSimilarity`, whose column loops are the same text, each passing its row's
`CorrelationSums` to `AccumulateCorrelationPixel` as `&sums`.

---

## Recipe SV - Extract Sibling Blocks Differing in One Value

**Use when:** CodeScene reports *Complex Method*, *Bumpy Road Ahead* or *Large Method* on
a function that holds two or more copies of one small block, typically sibling `switch`
cases, that are the same text except for one constant (`SetImageAlphaChannel`'s
`DeactivateAlphaChannel`, `DiscreteAlphaChannel` and `OnAlphaChannel` cases, which
differ only in the trait they assign). CodeScene does not report the copies as *Code
Duplication*, but moving each with Recipe E would write helpers that differ only in that
constant.

**Not covered by:** Recipe E, which passes the block's variables as parameters but no
constant, so each copy would become its own near-identical helper; Recipe D, whose *Use
when* is a reported *Code Duplication* (these copies, a few lines each, are not
reported); Recipe TW, whose copies stand in two functions and whose inputs, five or more,
go in a struct; Recipe SD, which moves a group of different cases, not copies of one.

**How:**

1. Each copy lies in the same function and meets Recipe E steps 1 and 3 to 7 and the
   OpenMP rule above, with no `#pragma` line.
2. The copies are the same token sequence once one value is replaced by a name, checked
   by script (whitespace and line breaks ignored; no variable renamed). The value is a
   literal or an enumeration constant, the same at each of its positions in one copy
   (Recipe D's *One value at several positions*), and differs between at least two
   copies. Or the copies read different variables: they are the same token sequence
   once each copy's variables are renamed consistently (Recipe D's renaming) and the
   value is replaced by a name, and the value is then a literal, an enumeration constant
   or a member access of an unwritten variable (`image->columns` in one copy,
   `image->rows` in another), of one type in every copy, that nothing in the block
   writes (Recipe TW step 2's differing value). Each copy passes its own variables.
   The value may also be an object-like macro the file defines whose expansion is a
   call, with constant arguments, of a `static inline` function that reads and writes
   nothing but its own parameters (`AccentuateModulate`, `ScaleCharToQuantum(80)`): the
   call passes the macro's name, and the parameter has the function's return type. A
   copy's one written variable `v`, which the copy assigns whole before it reads it
   (`accentuate=matte;`) and the caller reads after it, is the new function's local of
   `v`'s type and its return value; the copy becomes `v=Helper(...);` (Recipe E steps 3
   and 4).
   A copy whose one write is `v=c;` of one constant under its test, so that a path
   leaves `v` alone, and whose `v` the caller sets on every path to the copy and reads
   after it, takes `v` by value as its last parameter and returns it, as Recipe E
   step 7's `switch` form does: the function ends with `return(v);` and the copy
   becomes `v=Helper(...,v);`. A member-access value may reach the member through an
   element of an array member (`image->colormap[i].red`) when the block writes neither
   the pointer, the index nor the element.
3. The new function's parameters are the variables a copy reads (Recipe E step 3) and
   the value, at most four in all. The value's parameter is `const` and has the type of
   every object it is assigned to or parameter it is passed to in the block (an
   enumeration's type for an enumeration constant assigned to a member of that type);
   used in an expression or comparison, it has the constant's own type, as in Recipe D.
   A member-access value's parameter has the member's declared type.
4. Write one `static` function directly above the caller: the text of one copy, the
   value replaced at each of its positions by the parameter's name, nothing else
   changed. Recipe E step 7's forms apply as for one block (in a `switch` case, a single
   output `v` taken by value and returned, the copy becoming `v=Helper(...);`).
5. Replace every copy with the call, passing that copy's value where the parameter
   stands and each variable in the position of its parameter. All copies move in one
   commit; the message gives the score of the first copy moved alone under Recipe E.
6. Check by script that each copy equals the helper's body with that copy's arguments
   substituted, and by reading that every argument fills its parameter.

**Before:**

```c
    case DeactivateAlphaChannel:
    {
      if ((image->alpha_trait & BlendPixelTrait) == 0)
        status=SetImageAlpha(image,OpaqueAlpha,exception);
      image->alpha_trait=CopyPixelTrait;
      break;
    }
    case DiscreteAlphaChannel:
    {
      if ((image->alpha_trait & BlendPixelTrait) == 0)
        status=SetImageAlpha(image,OpaqueAlpha,exception);
      image->alpha_trait=UpdatePixelTrait;
      break;
    }
```

**After:**

```c
static MagickBooleanType SetOpaqueAlphaTrait(Image *image,
  const PixelTrait alpha_trait,MagickBooleanType status,
  ExceptionInfo *exception)
{
  if ((image->alpha_trait & BlendPixelTrait) == 0)
    status=SetImageAlpha(image,OpaqueAlpha,exception);
  image->alpha_trait=alpha_trait;
  return(status);
}
...
    case DeactivateAlphaChannel:
    {
      status=SetOpaqueAlphaTrait(image,CopyPixelTrait,status,exception);
      break;
    }
    case DiscreteAlphaChannel:
    {
      status=SetOpaqueAlphaTrait(image,UpdatePixelTrait,status,exception);
      break;
    }
```

**Careful:**

- A constant has no side effect, so evaluating it as an argument before the block runs
  instead of where the block used it changes nothing; the statements of the block run
  in the same order on the same variables.
- The type rule of step 3 is what keeps the value: converting a constant to the type of
  the object it is assigned to at the call, instead of at the assignment, gives the same
  value, and an enumeration constant is a valid value of its enumeration. A parameter of
  another type (an `int` for a `double` literal, a `float` for a `double`) would round
  or truncate: then the parameter takes the constant's own type.
- Only one value may differ. Two differing values, or a variable that differs
  without step 2's consistent renaming, is not this recipe: leave the copies, or move
  one with Recipe E and report the rest.
- Under step 2's renaming each copy passes its own variables, in the positions of the
  parameters that stand for them, so each call runs its copy's operations on the values
  its copy read, as in Recipe D. A member-access value is read once, at the call, where
  the copy read it at each use: the same value only while nothing in the block can
  change it (Recipe TW's *Careful*); check by reading that the block writes neither the
  member nor, through any pointer, the struct that holds it. Its parameter, of the
  member's declared type, converts nothing.
- A macro value (step 2's last form) is evaluated once, at the call, where the copy
  evaluated it at each use. A call with constant arguments of a function that reads and
  writes nothing but its parameters gives the same value every time, already converted
  to the function's return type by its `return`; a parameter of that type converts
  nothing, and each `(double) value` in the block casts the same value the call's
  result was cast from. Read every definition of the function that a configuration can
  compile (`ScaleCharToQuantum` has one per quantum depth, with and without HDRI, each
  a cast of the argument times a constant) before choosing the value.
- A returned `v` (step 2) is the copy's own work on a local: the copy assigned `v` whole
  before reading it, so the local starts from the same value, takes the same member
  writes, and the caller's `v=Helper(...);` copies every member back, as the copy left
  them. Nothing in the copy may read `v` before that whole assignment or take its
  address.
- A `v` taken by value (step 2's conditional form) enters the function holding the
  caller's value, which the caller set on every path to the copy. The path that
  writes it returns the constant the copy assigned; the path that does not returns
  the value it took. Either way the caller's `v` holds after the call what it held
  after the copy. The copy did not read `v`, and reading it as an argument has no
  side effect. An array-element member value is read once at the call, where the copy
  read it once inside the test; nothing in the block writes the array, the index or
  the element, so it is the same value.
- `refactor_guard.py` reports the removed copies as deduplication (`WARN`, every value
  still present) and `--calls` moves only the callees of the copies (down by the copies
  removed) and the helper (its definition and one call per copy); anything else stops
  the step.

**Evidence level:** oracle, with `refactor_guard.py --calls` and steps 2 and 6 checked
by script; each copy reached by a case (the case list names the option or path that
reaches it).

**Provenance:** 2026-10-30, from `channel-alpha.c`'s `SetImageAlphaChannel`, whose
`DeactivateAlphaChannel`, `DiscreteAlphaChannel` and `OnAlphaChannel` cases differ only
in the trait assigned. Recipe E on the first alone: 7.56 to 7.58; as three Recipe E
helpers: 7.62, with three functions that differ in one constant; as one helper: 7.62.
Added by Claude Code (Claude Opus 5.5) during task W10, under the mandate of 2026-10-24.
Step 2's renaming and member-access value (2026-10-30, task W10): `shear-rotate.c`'s
`IntegralRotateImage`, whose 90- and 270-degree tile loops each clip the tile's width
(`tile_width`, `tile_x`, `image->columns`) and height (`tile_height`, `tile_y`,
`image->rows`) with the same two statements.
Step 2's macro value and returned variable (2026-10-30, task W10): `decorate.c`'s
`FrameImage`, whose `accentuate` and `highlight` colours are the same five statements on
`matte` with `AccentuateModulate` or `HighlightModulate`, and whose `shadow` and `trough`
colours the same with `ShadowModulate` or `TroughModulate`.
Step 2's conditional `v` and array-element member value (2026-10-31, task W10):
`attribute-depth.c`'s `RaiseColormapEntryDepth`, whose red, green and blue tests each
end in `if (IsPixelAtDepth(ClampToQuantum(image->colormap[i].<c>),range) ==
MagickFalse) atDepth=MagickFalse;`.

---

## Recipe AR - Extract Block Through a Struct of Array Pointers

**Use when:** CodeScene reports *Deep, Nested Complexity*, *Bumpy Road Ahead*, *Complex
Method* or *Large Method*, and the block that would fix it reads or updates, element by
element, several local arrays of the caller: with one pointer parameter per array
(Recipe E step 3, Recipe K step 4) the new function would take five or more arguments,
or the block writes more arrays than Recipe Q step 8 admits. The typical case is a
function that keeps one family of per-channel values in parallel arrays and runs pass
after pass over them (`statistic-moments.c`'s `GetImageMoments`: the moment sums `M00`
to `M30` and `centroid`).

**Not covered by:** Recipe E, whose block produces one value (an array included, step 3)
and takes its inputs one parameter each; Recipe N, whose block writes one value and
whose struct holds scalars, pointers and struct values, not arrays (its *Careful*);
Recipes K and Q, whose block is an accumulation writing at most four variables (six
under Q step 8, which passes each array as a pointer to the array and rewrites every use
`v[i]` as `(*v)[i]`); Recipes O, Z and I, which write plain locals, not arrays (I step 2
refuses arrays).

**How:**

1. The block lies in a function with no OpenMP region and contains no `#pragma` line,
   no `return`, `goto`, or `break` or `continue` that leaves it. It is one nested block,
   or a run of sibling statements named as such in the commit message (Recipe E).
2. List the arrays of the caller the block indexes: arrays declared in the caller's body
   (not parameters, globals or `static`s), of any element type. The block uses each only
   as `v[e]` (read, assigned or compound-assigned) or passes it whole to a callee's
   pointer parameter (`RotateEllipseAngle(channel_moments,c,M20,M02)`); never `&v`,
   `sizeof v`, or `v` in any other expression.
3. Everything else follows Recipe E steps 3 to 7: besides array elements and writes
   through pointers the block already uses (`channel_moments[c].ellipse_angle`), it
   writes at most one variable of the caller that the caller reads after it, which the
   new function returns (`y=Helper(...);`); a temporary it writes before reading is
   declared in the new function, and one it reads on entry and then writes, dead after
   the block, goes by value (E step 4). The other inputs go by value, or in one `const`
   input struct as Recipe N steps 2 to 8 build it; a struct type the file already
   declares for another helper of the same caller, whose members are exactly the
   variables the block reads (types and names), is reused, not declared twice. At most
   four parameters in all, the struct of step 4 included.
4. Declare `typedef struct _Name { ... } Name;` directly above the first function that
   uses it, one member per array of the caller, a pointer to its element type under its
   name (`double M00[2*MaxPixelChannels+1]` gives `double *M00`), in the order the
   caller declares them, holding addresses only. One struct holds the whole family, so
   later blocks of the same caller reuse it; a block need not use every member.
5. In the caller, declare `const Name name = { v1, v2, ... };` after the arrays'
   declarations, one array per member in member order: each array's name stands for the
   address of its first element (Recipe Q step 8's `const` form). In a helper of this
   recipe, the `const Name *name` it received stands for the struct: a block there that
   moves on (`AdjustEllipseAngle` out of `ComputeEllipseMoments`) takes the same
   pointer.
6. Cut the block into a new `static` function directly above the caller, taking the
   step-3 parameters and `const Name *name`. At the top of its body bind each array the
   block indexes, in member order, `T *const v = name->v;`, or `const T *const v =
   name->v;` for one it only reads (and passes only to pointer-to-`const` parameters).
   The block's text is unchanged, except that `&name` passed on to another helper of
   this recipe becomes `name`.
7. Call it where the block stood, with `&name` (`name` inside a helper of this recipe),
   and check by reading every argument against its parameter, and every member of the
   initialiser against the array of its name.

**Before:**

```c
  double
    channels,
    M00[2*MaxPixelChannels+1] = { 0.0 },
    ...
    M30[2*MaxPixelChannels+1] = { 0.0 };
  ...
  for (c=0; c <= MaxPixelChannels; c++)
  {
    /*
      Normalize image moments.
    */
    M10[c]=0.0;
    M01[c]=0.0;
    M11[c]*=MagickSafeReciprocal(pow(M00[c],1.0+(1.0+1.0)/2.0));
    ...
    M00[c]=1.0;
  }
```

**After:**

```c
typedef struct _MomentArrays
{
  double
    *M00,
    ...
    *M30;

  PointInfo
    *centroid;
} MomentArrays;
...
static void NormalizeMoments(const MomentArrays *moments)
{
  double
    *const M00 = moments->M00,
    ...
    *const M30 = moments->M30;

  ssize_t
    c;

  for (c=0; c <= MaxPixelChannels; c++)
  {
    ...
    M00[c]=1.0;
  }
}
...
  const MomentArrays
    moments = { M00, M01, M02, M03, M10, M11, M12, M20, M21, M22, M30,
      centroid };
  ...
  NormalizeMoments(&moments);
```

**Careful:**

- `v[e]` is `*(v+e)` whether `v` is the array or a pointer to its first element, so each
  use names the same element of the caller's array, read or written at the same point
  and in the same order; nothing is copied in or back. The bound pointers are `const`
  and the struct is a `const` local, so no pointer is reseated.
- `&v` and `sizeof v` differ between an array and a pointer (the address of the whole
  array, the size of the array against that of a pointer), so step 2 refuses a block
  that uses either. An array passed whole to a callee was already converted to the same
  pointer the bound local holds.
- The arrays are distinct objects, so no two members alias, and none aliases a pointer
  the block already uses; the caller's other uses of the arrays (its own indexing, other
  helpers' parameters) see every element the helper wrote, as they saw the block's.
- The initialiser reads no element, only addresses, fixed from the arrays' declarations
  to the end of the function, so evaluating it once at the declaration stores what an
  assignment before each call would (Recipe Q step 8's `const` form; C99 admits a
  non-constant initialiser of an automatic aggregate, as `compare.c`'s `sums` already
  has). The arrays' own initialisers (`= { 0.0 }`) run first, in declaration order.
- `const T *const v` for an array the block only reads adds `const` to the pointed-to
  type: reads give the same values, and the compiler refuses a write.
- The returned variable of step 3 is Recipe E's single value: the block wrote it on
  every path the caller reads afterwards, and `v=Helper(...);` hands the caller the
  value the block left (`y`, the row at which a cache read stopped a pass).

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for the new function,
nothing else moved), and steps 2, 5 and 7 checked by reading.

**Provenance:** 2026-10-31, from `statistic-moments.c`'s `GetImageMoments` (W10): eleven
moment arrays and `centroid`, filled by two pixel passes, then averaged, normalised and
read by per-channel loops. The second pass's per-pixel sum writes eight arrays (Q step 8
admits six, and its form rewrites sixteen uses); the composite average and the
normalisation write all eleven; the ellipse and Hu invariant loops read four and seven,
with the result array, five or more parameters under Recipe E. A probe of the whole
function under this recipe, Recipe E and Recipe Q: 8.05 to 10.0. Added by Claude Code
(Claude Opus 5.5) during task W10, under the mandate of 2026-10-24.

---

## Recipe RF - Reuse a Struct Fill Across Calls

**Use when:** CodeScene reports *Large Method* on a function in which
earlier steps (Recipes N, Q, U, Y, LB, B) left one struct local filled member by member
directly before each of several calls, and a later fill assigns every member the same
expression as an earlier one, with nothing in between that writes the local or anything
those expressions read (`FloodfillPaintImage`'s `FloodfillRow row`, filled with the same
eight assignments before `FillFloodplaneLeft`, `FillFloodplaneRun` and
`SkipFloodplaneRun` in one iteration of its segment loop).

**Not covered by:** Recipe D, which replaces copies of a block with calls to one
function and whose *Use when* is a reported *Code Duplication*; Recipes N (step 6), Y,
LB and B, which place a fill directly before each call and say nothing of a later call
reading an earlier fill; the prohibition on deleting code that looks dead, which is
about code some path or configuration may reach: a repeated fill runs, and stores
values the members already hold.

**How:**

1. Find two fills `F1` and `F2` of one struct local `name`: runs of `name.m=e;`
   assignments in which `F2` assigns the same members as `F1`, in the same order, each
   from the same expression text. Check this by script.
2. Every expression `e` of the fill is an identifier or a member access (`row.y=y;`,
   `row.target=target;`): no call, no increment, nothing with a side effect.
3. `F1` runs on every path to `F2`, after the last time that path reaches `name`'s
   declaration: `F1` precedes `F2` in the same block, or in a block enclosing it, and no
   `goto`, `case` label or loop back edge reaches `F2` without passing `F1` since the
   declaration (a loop that encloses `F2` but not `F1` lies wholly inside the region
   in which `name` is declared, so its back edge does not reach the declaration again).
4. On every path from `F1` to `F2`, the loop back edges included, nothing writes `name`
   or a member of it, and nothing writes a variable an expression of the fill reads.
   Check by reading every statement and expanded macro on those paths, and every callee
   they call: `&name` may be passed only to a `const Name *` parameter whose function
   writes nothing through it (no cast removes the `const`), and no address of a
   variable the fill reads may be passed to anything that writes through it.
5. Delete `F2`. Nothing else changes. Several repeated fills of the same `name` in one
   function, each meeting steps 1 to 4 against the same `F1`, go in one commit.
6. Run `sh tools/obj_identical.sh MagickCore/<file>.c` and give its verdict in the
   commit message: `IDENTICAL` proves the step for the build's configuration. When it
   differs (the compiler kept a store it could not prove redundant), the oracle and
   the reading of steps 3 and 4 are the evidence.

**Before:**

```c
    row.image=image;
    ...
    row.y=y;
    row.exception=exception;
    if (FillFloodplaneLeft(&row,x1,&x) == MagickFalse)
      break;
    ...
    do
    {
      if (skip == MagickFalse)
        {
          row.image=image;
          ...
          row.y=y;
          row.exception=exception;
          if (FillFloodplaneRun(&row,&x) == MagickFalse)
            break;
          PushSegmentStack(y,start,x-1,offset);
          ...
        }
      ...
      if (x <= x2)
        {
          row.image=image;
          ...
          row.exception=exception;
          if (SkipFloodplaneRun(&row,x2,&x) == MagickFalse)
            break;
        }
      start=x;
    } while (x <= x2);
```

**After:**

```c
    row.image=image;
    ...
    row.y=y;
    row.exception=exception;
    if (FillFloodplaneLeft(&row,x1,&x) == MagickFalse)
      break;
    ...
    do
    {
      if (skip == MagickFalse)
        {
          if (FillFloodplaneRun(&row,&x) == MagickFalse)
            break;
          PushSegmentStack(y,start,x-1,offset);
          ...
        }
      ...
      if (x <= x2)
        {
          if (SkipFloodplaneRun(&row,x2,&x) == MagickFalse)
            break;
        }
      start=x;
    } while (x <= x2);
```

**Careful:**

- A struct member holds the value last stored in it. `F2` stores in each member the
  value of an expression whose operands nothing has written since `F1` evaluated the
  same expression, so it leaves every member as it was, and each call after `F2` reads
  the same values with or without it. Step 2 keeps the expressions free of side
  effects, so evaluating them once fewer changes nothing else.
- A local declared without an initialiser in a loop body becomes indeterminate each
  time its declaration is reached (C11 6.2.4): a fill made before that point, in an
  earlier iteration or before the loop, does not serve a call after it. Step 3 is
  what rules this out.
- A `const Name *` parameter does not by itself prove the callee leaves the struct
  alone: C permits writing through a pointer whose `const` a cast removed when the
  object itself is not `const`. Step 4 reads the callees.
- A variable a fill reads that changes between the fills (a loop counter `y` advanced
  by a loop around `F2` alone, an `x` passed by address to a callee) makes the fills
  differ in value though they agree in text: step 4 refuses it.
- A macro on the path counts with its expansion: `PushSegmentStack` writes `s` and the
  segment it points to, and on overflow destroys the views and returns, leaving the
  function; it writes no variable the fill reads.
- `refactor_guard.py` sees no literal and no call move (the fills hold neither);
  `--calls` must report nothing.

**Evidence level:** oracle, with `tools/obj_identical.sh` (step 6), and steps 3 and 4
checked by reading.

**Provenance:** 2026-10-31, from `paint.c`'s `FloodfillPaintImage` (W10): three LB
steps (W09) left the `FloodfillRow` local filled with the same eight assignments before
each of its three calls in one iteration of the segment loop; `y` is set once per
iteration before the first fill, and nothing between the fills writes `row` or what
they read. A probe deleting the second and third fills: 6.19 to 6.24 (172 to 156
lines), object code `IDENTICAL`. A script finds runs of three or more identical
`name.m=...;` assignments repeated within one file in `effect.c`, `morphology.c`,
`vision.c`, `fx.c`, `layer.c`, `shear.c` and others, mostly fills of this campaign's
helpers; whether each meets steps 3 and 4 is unchecked. Added by Claude Code (Claude
Opus 5.5) during task W10, under the mandate of 2026-10-24.

---

## Recipe LT - Extract Loop Whose Early Exit Is Tested After It

**Use when:** CodeScene reports *Deep, Nested Complexity*, *Bumpy Road Ahead*, *Complex
Method* or *Large Method*, and the block that would fix it is a `for` loop that leaves
itself through `break;`, followed directly by a test of the loop's own condition,
`if (<cond>) X;`, which runs `X` exactly when the loop ended through a `break`:
`histogram.c`'s `CheckImageColors`, whose tree descent is `for (level=1; level <
MaxTreeDepth; level++) { ... break; ... }` followed by `if (level < MaxTreeDepth)
break;`, and whose column loop is followed by `if (x < (ssize_t) image->columns)
break;`.

**Not covered by:** Recipe LB, whose step 3 turns into `return(MagickFalse);` only a
`break;` that ends the enclosing loop `Lp`, so the loop's own `break;`, its counter and
the test would move unchanged (for the descent, a near-copy of `DescendHCube`, which
returns at that point instead); LB step 6 and Recipe I step 7, which reuse a function
only when the texts are equal under renaming; Recipes E, N, V, Y, I and W, whose block
contains no `break` that leaves it; Recipe G, which adds guard clauses and moves no
code.

**How:**

1. The run is a `for` loop whose condition `<cond>` has no side effect and calls no
   function, followed directly, with nothing between, by `if (<cond>) X;`, the same
   condition token for token (checked by script). `X` is `break;` or `continue;` of a
   loop `Lp` enclosing both, `return(k);` of a constant `k`, or `goto L;`. The run lies
   outside every OpenMP region and contains no `#pragma` line.
2. The loop's counter `c` is written only by the `for`'s initialisation and increment,
   and nothing in the body writes `c` or a variable `<cond>` reads, or the object a
   pointer in `<cond>` reaches (`image->columns`: the body passes `image`, a `const
   Image *`, only to callees that do not change it). Check by reading every statement,
   expanded macro and callee of the body.
3. The body leaves the loop only through `break;` statements of the loop itself; it
   contains no `return` and no `goto`. A `continue;` of the loop, and a `break` or
   `continue` of a loop or `switch` nested inside the body, stay as they are.
4. Nothing after the test reads `c` before writing it again, on every path, `Lp`'s exit
   included: `c` is a temporary of the run, declared in the new function (Recipe Y
   step 2).
5. Cut the run into a new `static MagickBooleanType` function directly above the
   caller: the loop's text, each `break;` of the loop becoming `return(MagickFalse);`,
   one for one; the test is dropped; `return(MagickTrue);` follows the loop. Nothing
   else in the text changes. The variables the run reads, writes and uses as
   temporaries are passed and declared as Recipe LB steps 2, 4 and 8 say (a cursor the
   run reads on entry and advances, dead after it, goes by value: Recipe E step 4); a
   struct type the file already declares whose members are exactly the variables the
   run reads (types and names) is reused, not declared twice (Recipe AR step 3).
6. Where the run stood, write `if (Helper(...) == MagickFalse) X;`, `X` unchanged.
   Check every argument and member by reading.
7. A run whose step-5 text would equal, under Recipe D's renaming (checked by script),
   the body of a function of the file that Recipe I, LB, W or Y made, calls that
   function instead of a second copy, filling its struct locals directly before the
   call (Recipe I step 6), as Recipe LB step 6 allows; a declaration of the caller that
   only the run used goes with it.

**Before:**

```c
      node_info=cube_info->root;
      index=MaxTreeDepth-1;
      for (level=1; level < MaxTreeDepth; level++)
      {
        ...
        if (AcquireHNodeChild(&child) == MagickFalse)
          break;
        node_info=node_info->child[id];
        index--;
      }
      if (level < MaxTreeDepth)
        break;
```

**After:**

```c
      descent.image=image;
      descent.p=p;
      descent.cube_info=cube_info;
      descent.exception=exception;
      targets.node_info=(&node_info);
      targets.pixel=(&pixel);
      if (DescendHCube(&descent,&targets) == MagickFalse)
        break;
```

where `DescendHCube` (made from `ClassifyImageColors` by Recipe I) is the loop with
`return(MagickFalse);` at its `break;`.

**Careful:**

- The argument is the loop's own semantics. The condition held when the iteration
  began, and step 2 keeps every operand of it unchanged up to a `break;` of the loop,
  which skips the increment: after that `break`, `<cond>` is true and the caller runs
  `X` next, having run nothing in between. The helper returns `MagickFalse` at the
  same point, after the same statements, and the caller runs `X` next. When the loop
  ends because `<cond>` is false, the test is false and the caller goes on; the helper
  returns `MagickTrue` and the caller goes on. A `continue;` of the loop runs the
  increment and the condition, as before. Step 1's side-effect-free condition makes
  evaluating it once fewer change nothing.
- The counter's final value is the only thing the test conveyed; step 4 makes sure
  nothing else reads it. An unsigned counter that would wrap changes nothing: the
  argument uses only what the condition evaluates to.
- The other variables follow Recipe LB's argument: their writes land through the same
  pointers, and a temporary is written before it is read on every path.
- Under step 7 the existing function's body is the text step 5 would cut, so calling it
  is calling the function step 5 would have made.
- The failure paths are mostly allocation failures no case reaches: the oracle checks
  the fall-through path, and each `break` is checked by reading against its
  `return(MagickFalse);`, with `refactor_guard.py --calls`.

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for a new function,
or under step 7 the calls the run made moving to the reused function's count), and
steps 1 to 4 checked by reading.

**Provenance:** 2026-10-31, from `histogram.c`'s `CheckImageColors` (W10): its tree
descent ends the level loop through `break;` and tests `level < MaxTreeDepth` after it,
where `ClassifyImageColors`' identical descent returned `0` and is now `DescendHCube`;
LB would write a second, near-identical descent. Its column loop ends through `break;`
and is tested `x < (ssize_t) image->columns` after it. Probes: the descent calling
`DescendHCube`, 8.58 to 8.64; with the column body as a Recipe LB helper and the
column loop as this recipe's helper, 9.68, `CheckImageColors` with no finding left.
Added by Claude Code (Claude Opus 5.5) during task W10, under the mandate of
2026-10-24.

---

## Recipe OC - Extract Block Ending the Iteration with Out-Parameters

**Use when:** CodeScene reports *Deep, Nested Complexity*, *Bumpy Road Ahead*, *Complex
Method* or *Large Method*, and the block that would fix it sits in a loop body, is
followed directly by that loop's `continue;` (so every path out of it ends the
iteration), leaves early through `continue;` itself, and moves the loop's cursors: a
character decoder's inner `switch` that advances the input and output pointers, as
`InterpretDelegateProperties`' backslash escapes do.

**Not covered by:** Recipe O, whose step 3 refuses a block containing a `continue` that
leaves it; Recipe E step 7, whose `continue` forms need the block to be the loop's whole
body or its tail and to write at most one variable (step 4); Recipe LB step 7, whose
helper returns a status the caller tests, which here would always lead to the same
`continue;`, and which needs a struct of out-pointers for two written cursors (step 8).

**How:**

1. The block lies outside every OpenMP region and contains no `#pragma` line. It is
   followed directly by `continue;` of the innermost loop around it, `Lp`; every
   `continue;` in it that leaves it continues `Lp`. It contains no `return`, no `goto`,
   and no `break` that leaves it; a `break` or `continue` of a loop or `switch` nested
   inside it stays as it is. A block that is the last statements of `Lp`'s body (its
   tail, after statements that stay) counts as followed by `continue;`: the end of the
   body ends the iteration as that `continue;` would. Step 5 then writes the call as
   the body's last statement, with no `continue;` after it.
2. The variables of the caller it writes meet Recipe O step 2 (plain locals, addresses
   not taken elsewhere); with those it only reads, at most four parameters.
3. Cut the block into a new `static void` function directly above the caller. Each
   `continue;` that continued `Lp` becomes `return;`, one for one. Write `*v` for every
   use of a written variable `v`, reads included (`(*v)++` for `v++`, `*(*v)++` for
   `*v++`, `**v` for `*v`, `*(*v+1)` for `*(v+1)`). Nothing else in the text changes.
4. Pass the variables it only reads by value and each written variable as a pointer,
   in the order the block first writes them.
5. Where the block stood, write the call. The `continue;` that followed the block stays,
   directly after the call. Check every argument against its parameter by reading.

**Before:**

```c
      case '\\':
      {
        switch (*(p+1))
        {
          case '\0':
            continue;
          case 'r':  /* convert to RETURN */
          {
            *q++='\r';
            p++;
            continue;
          }
          ...
          default:
          {
            p++;
            *q++=(*p);
          }
        }
        continue;
      }
```

**After:**

```c
static void DecodeDelegateEscape(const char **p,char **q)
{
  switch (*(*p+1))
  {
    case '\0':
      return;
    case 'r':  /* convert to RETURN */
    {
      *(*q)++='\r';
      (*p)++;
      return;
    }
    ...
    default:
    {
      (*p)++;
      *(*q)++=(**p);
    }
  }
}
...
      case '\\':
      {
        DecodeDelegateEscape(&p,&q);
        continue;
      }
```

**Careful:**

- Why the behaviour is the same. Before, every path out of the block reached a
  `continue;` of `Lp`: one inside it, or the one that follows it. After, every path out
  of the function (a `return;` where a `continue;` stood, or the end of the body where
  the block fell through) returns to the call, and the caller runs the `continue;` that
  followed the block. The statements before each exit are the same, in the same order,
  and every write to a written variable has landed through its pointer when the
  function returns (Recipe O).
- `continue;` in a `for` loop runs the loop's increment expression; it still runs, in the
  caller, after the helper has written the cursors it reads.
- The rewriting of step 3 is mechanical but easy to get wrong in precedence: `*v++` with
  `v` a pointer parameter is `*(*v)++`, not `**v++`. Check each by reading, and let the
  oracle cover the reached paths.
- A block not followed by `continue;` (one after which the loop body goes on) is not this
  recipe. A block that ends the body is (step 1): in C, `continue;` in a loop body
  jumps to the end of that body (C11 6.8.6.2), so falling off the end and running
  `continue;` are the same transfer, and the call, as the body's last statement, is
  followed by that same end.

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for the new function,
nothing else moved), and steps 1, 3 and 5 checked by reading.

**Provenance:** 2026-10-30, from `delegate.c`'s `InterpretDelegateProperties`, whose
backslash case is an inner `switch` on the next character that advances `p` and `q` and
ends each case with `continue;` of the scan loop, followed by a `continue;` for the
`default` path; it holds the function's nesting of 4. A probe: nesting 4 gone, cc 45 to
33, delegate.c 3.83 to 4.05. Added by Claude Code (Claude Opus 5.5) during task M115
(campaign B), under the mandate of 2026-10-24. Step 1's body tail (2026-10-31, campaign
B, task M116): `image.c`'s `AppendImages`, whose scan loop ends in the extent update
(stacked: widen and `continue;`; otherwise add the columns and take the taller height),
writing `width` and `height`; part of the probe that takes the function from 4 bumps
to 3.

---

## Recipe VC - Extract Loop-Tail Block Returning Success for a Cleanup Exit

**Use when:** CodeScene reports *Deep, Nested Complexity*, *Bumpy Road Ahead*, *Complex
Method* or *Large Method*, and the block that would fix it ends a loop body (or is
followed directly by that loop's `continue;`) and has two kinds of exit: `continue;` of
that loop, and a cleanup exit of the function in one of Recipe V's forms (`return(c);`,
or the same `S return(...)` at every exit, as V step 7 and I step 8 admit). The typical
case is one branch of a declaration parser at the end of its scan loop, which skips an
empty declaration with `continue;` and gives up on a malformed one by releasing a buffer
and returning `MagickFalse`, as `xml-tree.c`'s `<!ATTLIST` branch does.

**Not covered by:** Recipes V, Y, I and W, whose block contains no `continue` or
`break` that leaves it (V step 1); Recipe OC, whose block contains no `return` (step 1)
and whose function is `void`; Recipe LB, whose exits are `break;` (or, under step 7,
`S continue;`), not a return of the function; Recipe E step 7, whose `continue` form
needs a block with no `return` of its own. Leaving the `continue;` in the caller is not
an option: it would sit at the block's depth, as LB's *Not covered by* says of `break`.

**How:**

1. The block lies outside every OpenMP region and contains no `#pragma` line. It is the
   tail of the body of the innermost loop around it, `Lp`, or is followed directly by
   `continue;` of `Lp` (Recipe OC step 1). Every `continue;` in it that leaves it
   continues `Lp`; it contains no `goto` and no `break` that leaves it; a `break` or
   `continue` of a loop nested inside it stays as it is.
2. Every other exit is a return of the function in one form for all exits: `return(c);`
   of one constant (Recipe V step 6), or one run `S return(...)` that Recipe V step 7 or
   Recipe I step 8 admits, under the conditions those steps set (checked by script:
   every copy identical). With `S`, every other exit of the block is that same text.
3. The written variables meet Recipe V step 2 (one, by pointer) or Recipe I step 2 (two
   to four, through a `const` struct of out-pointers, Recipe I steps 3, 5 and 6); the
   variables it only reads go by value or in one `const` input struct (Recipe Y steps 3
   to 5); at most four parameters. Temporaries are declared in the new function (Recipe
   Y step 2).
4. Cut the block into a new `static MagickBooleanType` function directly above the
   caller. Each `continue;` that continued `Lp` becomes `return(MagickTrue);`, one for
   one; each exit of step 2 (the `return(c);`, or `S` with its return) becomes
   `return(MagickFalse);`, one for one; the function ends with `return(MagickTrue);`.
   Write `*v` for every use of a written variable `v` (Recipe O step 5). Nothing else in
   the text changes.
5. Where the block stood, write `if (Helper(...) == MagickFalse) return(c);`, or
   `if (Helper(...) == MagickFalse) { S return(...); }` with `S` and the return copied
   unchanged; a `continue;` that followed the block stays after it. Check every argument
   and member by reading.

**Before:**

```c
      else
       if (strncmp(xml,"<!ATTLIST",9) == 0)
         {
            /*
              Parse default attributes.
            */
            t=xml+strspn(xml+9,XMLWhitespace)+9;
            if (*t == '\0')
              {
                (void) ThrowMagickException(exception,GetMagickModule(),
                  OptionWarning,"ParseError","unclosed <!ATTLIST");
                predefined_entities=(char **) RelinquishMagickMemory(
                  predefined_entities);
                return(MagickFalse);
              }
            xml=t+strcspn(t,XMLWhitespace ">");
            if (*xml == '>')
              continue;
            ...
        }
      else
        if (SkipDoctypeMarkup(root,&xml) == MagickFalse)
          break;
    }
```

**After:**

```c
static MagickBooleanType ParseAttributeListDeclaration(XMLTreeRoot *root,
  ExceptionInfo *exception,const AttributeListTargets *targets)
{
  char
    **const xml = targets->xml;

  char
    **const n = targets->n;
  ...
  t=*xml+strspn(*xml+9,XMLWhitespace)+9;
  if (*t == '\0')
    {
      (void) ThrowMagickException(exception,GetMagickModule(),
        OptionWarning,"ParseError","unclosed <!ATTLIST");
      return(MagickFalse);
    }
  *xml=t+strcspn(t,XMLWhitespace ">");
  if (**xml == '>')
    return(MagickTrue);
  ...
  return(MagickTrue);
}
...
         {
            AttributeListTargets
              attribute_list_targets;

            attribute_list_targets.xml=(&xml);
            attribute_list_targets.n=(&n);
            if (ParseAttributeListDeclaration(root,exception,
                &attribute_list_targets) == MagickFalse)
              {
                predefined_entities=(char **) RelinquishMagickMemory(
                  predefined_entities);
                return(MagickFalse);
              }
        }
```

**Careful:**

- Why the behaviour is the same. Before, every path out of the block either reached a
  `continue;` of `Lp` (one inside it, or the end of the body or the `continue;` that
  follows it, which C treats as the same jump, C11 6.8.6.2), or ran an exit of step 2.
  After, the first kind returns `MagickTrue` and the caller, the call being the body's
  last statement or followed by that `continue;`, ends the iteration at the same point;
  the second returns `MagickFalse` and the caller runs the exit's own text, unchanged,
  at once. The statements before each exit are the same, in the same order, and every
  write to a written variable has landed through its pointer when the function returns.
- The exit's argument is Recipe V's (step 6 or 7) or Recipe I step 8's, whichever form
  it has, with the same conditions on what `S` and the returned expression may read.
- `continue;` in a `for` loop runs the loop's increment expression: it still runs, in
  the caller, after the helper has written the cursors it reads.
- The result now says "go on with the loop" as well as "fell through": both are
  `MagickTrue`, because the caller does the same thing after either, end the iteration.
  A block after which the body goes on (neither the tail nor followed by `continue;`)
  is not this recipe: there a `continue;` and a fall-through differ.

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for the new function;
under `S`, also `-(k-1)` for each call in `S`, where `k` is the number of exits), and
steps 1, 2 and 5 checked by reading.

**Provenance:** 2026-10-31, from `xml-tree.c`'s `ParseInternalDoctype`, whose
`<!ATTLIST` branch, the tail of the scan loop's body, skips an empty list with
`continue;` and leaves on three malformed forms by releasing `predefined_entities` and
returning `MagickFalse`; its default-value test holds the function's nesting of 5. A
probe of the branch: cc 37 to 25, xml-tree.c 4.05 to 4.19; with its attribute loop's
body moved the same way, 4.21. Added by Claude Code (Claude Opus 5.5) during task M116
(campaign B), under the mandate of 2026-10-24.

---

## Recipe H - Hoist Function-Local Macros

**Use when:** a function `#define`s constants or helper macros at the top of its body, and
the blocks Recipe E or P would move use them. A helper outside the function cannot sit
above those lines, so every such block is pinned: `HuffmanDecodeImage`, `LZWEncodeImage`,
`TransformSignature`, `GenerateDifferentialNoise`, `SyncExifProfile`, `GetEXIFProperty`.
Also when CodeScene reports *Complex Method* or *Large Method* on a function whose
leading `#define` run holds helper macros with branches (`if`, `&&`, `||`, `?:`, a `case`
label) or many lines: CodeScene counts a macro's body in the function whose braces hold
the `#define`, not where the macro is used, so the run alone carries part of the finding
(`magnify.c`'s `Corner`, `HelperCond`, `caseA`/`caseB`). The step is then kept for what
it measures on its own, under rule 2, with no extraction after it.

**Not covered by:** Recipe E, whose step 7 moves a macro only together with a block that is
its sole user; here the macros serve the whole function. Recipe S, which moves whole
functions between files. No other recipe changes where a preprocessor line sits.

**How:**

1. Take the run of `#define` lines (with their `\` continuations and the blank lines
   between them) that starts on the first line after the function's opening `{` and ends
   before its first declaration. Nothing else may sit in that run.
   A run that follows only declarations at the top of the body, none of which names one
   of its macros (`Xbr2X`'s `const MagnifyPixels magnify = { pixels, result, channels };`
   before `WeightVar`), may be taken the same way: the run, from its first `#define` to
   its last line, moves; the declarations stay. Step 4's proof covers them too.
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
  signature, and under step 1's second form over the declarations before the run.
  `pp_identical.sh` proves that no token there changed meaning.
- Do not hoist a macro that is `#define`d differently in an earlier function of the same
  file: the run would then redefine it above code that used the other definition. The
  compiler warns about a redefinition, and the build must stay free of warnings.
- For a run of constants the step changes no code Code Health measures and leaves the
  score flat. It is kept under rule 2 only when the same task follows it with the
  extractions it unblocks, and its commit names them. A run of helper macros with
  branches is measured: the function loses the macros' decisions from its cyclomatic
  complexity and their lines from its length, while the compiled code, which the
  preprocessed translation unit fixes, is the same. Keep that step only when the file
  score rises, and say in the commit that the code did not change, only where the
  lines stand.

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
   once) does not move. A non-`const` array counts as never written when every use of
   its name is an element read or an argument to a `const`-qualified pointer parameter;
   check each use by `grep`. It moves as written, without adding `const`.
   no statement assigns, or every use of it is `sizeof`, an argument to a parameter of
   pointer-to-`const` type (`SetsRGBImageProfile`'s `sRGBProfile`, passed only as
   `const void *blob`), or an element read by subscript that is not assigned, incremented
   or address-taken (`EncipherAESBlock`'s `D[...]`). A static the function updates (a counter, a cache, a flag set
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

## Recipe CN - Combine Nested Conditions

**Use when:** CodeScene reports *Bumpy Road Ahead* (or *Deep, Nested Complexity*), and a
bump is an `if` whose whole body is a second `if`, neither with an `else`: a nesting
level that carries no statement of its own, as `GetDelegateInfo`'s three match tests
(`if (decode matches) if (encode matches) break;`) and `IsExecutableToken`'s
`if (length >= 3) { if (IsWindowsDrivePath(token) != MagickFalse) return(MagickTrue); }`.

**Not covered by:** Recipe G, which inverts a condition into an early exit and keeps both
tests as separate statements; Recipe P, which moves one expression into a predicate and
leaves the statement structure as it is; Recipes E, O, LB and W, which move the block into
a new function, where the nesting stays (a helper of nested `if`s is itself bumpy).

**How:**

1. Find `if (A) S` where `S` is `if (B) T`, or a compound statement `{ if (B) T }` holding
   that one statement and no declaration; neither `if` has an `else`. `T` is any
   statement.
2. Replace the two by `if ((A) && (B)) T`: `A` and `B` copied character for character,
   each in its own parentheses, in that order; `T` unchanged, re-indented one level
   less. Wrap at 80 columns as ImageMagick does, the second operand under the first.
3. Every such pair in one function may go in one commit (one recipe, one function).
4. Run `sh tools/obj_identical.sh <file>`. It must print `IDENTICAL`; otherwise revert
   the step (the compiler laid the code out differently, and the proof below does not
   hold for it).

**Before:**

```c
    if (LocaleCompare(decode,delegate_info->decode) == 0)
      if (LocaleCompare(encode,delegate_info->encode) == 0)
        break;
```

**After:**

```c
    if ((LocaleCompare(decode,delegate_info->decode) == 0) &&
        (LocaleCompare(encode,delegate_info->encode) == 0))
      break;
```

**Careful:**

- Why the behaviour is the same. `&&` evaluates `A` first and `B` only when `A` is
  non-zero, as the nested form does; `T` runs exactly when both are non-zero, in both
  forms. With no `else` on either `if`, no path runs anything when `A` or `B` is zero.
- An `else` changes this: on the inner `if` it runs when `A` holds and `B` does not,
  which the combined form cannot express; on the outer one it would bind differently.
  Step 1 refuses both.
- A declaration in the braces, or a second statement there, means the level carries
  something: step 1 refuses it.
- The combined condition has two operands; a chain that would reach *Complex Conditional*
  (three or more) is not this recipe's aim, and rule 2 judges it.

**Evidence level:** guard, with `tools/obj_identical.sh` printing `IDENTICAL` against
`HEAD` (byte-identical object code under the oracle build's pinned flags is the same
compiled code, so the same behaviour on every input, reached or not), and
`refactor_guard.py` literal and call counts unchanged.

**Provenance:** 2026-10-30, from `delegate.c`'s `GetDelegateInfo`, whose three match
tests are each an `if` holding one `if` and a `break`: a probe combining them gave
bumps 6 to 3 and delegate.c 3.26 to 3.32, with `IDENTICAL` object code. The same shape
holds in `IsExecutableToken` (twice) and `InvokeDelegate`. Added by Claude Code (Claude
Opus 5.5) during task M115 (campaign B), under the mandate of 2026-10-24.

---

## Recipe NS - Name a Constant Selection

**Use when:** CodeScene reports *Complex Method* (or a helper Recipe SD, O or Z would add
is refused as one), and much of the complexity is one conditional expression
`(c ? k1 : k2)` that picks between two constants and recurs at many sites, as
`AnnotateImage`'s gravity offsets do: `(geometry.width == 0 ? -1.0 : 1.0)` and three
variants, eighteen sites in one `switch`, each counted as a decision.

**Not covered by:** Recipe P, which moves a boolean condition and returns
`MagickBooleanType`; this expression yields a value of another type (a `double` sign),
and naming only its condition leaves the `?:` and its decision in place. Recipe D,
which merges copies of statements, not of an expression inside them. Recipes E, O and
SD, which move statements: the decision moves with them and the helper inherits it.

**How:**

1. The expression is `(c ? k1 : k2)` in its own parentheses: `k1` and `k2` are
   literals of one arithmetic type (`-1.0` and `1.0`), and `c` reads only identifiers,
   member accesses and literals, with no call, assignment, increment or decrement.
2. Write a `static inline` function directly above the first function that uses it,
   with one parameter per variable expression of `c` (`const size_t extent` for
   `geometry.width`), of that expression's exact type, returning the type of `k1`:
   `return(c' ? k1 : k2);`, `c'` being `c` with the parameters substituted, character
   for character otherwise. A second variant (`k2 : k1`) is a second function.
3. Replace each site, parentheses included, by the call with the site's expressions as
   arguments. Check by script that every replaced site equals the function's body with
   the arguments substituted, character for character, as Recipe P's *Several sites*
   asks.
4. All sites of the functions this step adds, in one function of the caller's, go in
   one commit (one recipe, one function); a site in another function is its own
   commit.
5. Run `sh tools/obj_identical.sh <file>`. It must print `IDENTICAL`; otherwise revert
   the step.

**Before:**

```c
      case (GravityType) NorthWestGravity:
      {
        offset.x=(geometry.width == 0 ? -1.0 : 1.0)*annotate_info->affine.tx+i*
          annotate_info->affine.ry*height+annotate_info->affine.ry*
          (metrics.ascent+metrics.descent);
```

**After:**

```c
static inline double GravityOffsetSign(const size_t extent)
{
  return(extent == 0 ? -1.0 : 1.0);
}
...
      case (GravityType) NorthWestGravity:
      {
        offset.x=GravityOffsetSign(geometry.width)*annotate_info->affine.tx+i*
          annotate_info->affine.ry*height+annotate_info->affine.ry*
          (metrics.ascent+metrics.descent);
```

**Careful:**

- Why the behaviour is the same. The argument is evaluated once, as `c` read it once;
  it has no side effect (step 1), and passing it converts nothing (step 2: the
  parameter has the expression's own type). The function returns `k1` or `k2` under
  the same test, of the same type, so the product or sum the site stood in receives the
  same value; the parentheses kept the site a single operand, so no operator regroups.
- `static inline` and identical object code under the oracle build's flags (step 5)
  prove it for this build on every input, reached or not.
- The variant with the constants swapped is a different function, not a negated
  argument: `!c` would be a new expression.

**Evidence level:** guard, with `tools/obj_identical.sh` printing `IDENTICAL` against
`HEAD`; `refactor_guard.py` reports `WARN` (copies removed, every value still
present), each literal of `c`, `k1` and `k2` dropping by its count per site times the
sites replaced less one per function, which the step-3 script accounts for; `--calls`
moves only the new functions, by one plus one per site each.

**Provenance:** 2026-10-30, from `annotate.c`'s `AnnotateImage`, whose gravity `switch`
carries eighteen `(geometry.width == 0 ? -1.0 : 1.0)`-shaped signs (cc 56 of the
function; a probe: cc 56 to 38 with `IDENTICAL` object code), and which Recipe SD can
split only once they are gone (a helper of three gravity cases would otherwise be a
*Complex Method*). Added by Claude Code (Claude Opus 5.5) during task M115 (campaign B),
under the mandate of 2026-10-24.

---

## Known plateaus

None recorded yet. When a file stops improving and no recipe in this catalogue applies,
and you do not add one, record it here: the file, its score, and the smells left.

| File | Score | Smells left, and why no recipe reaches them | Recorded |
| --- | ---: | --- | --- |
| `MagickCore/resize.c` | 7.08 | Brain Method, Deep Nesting (4), Bumpy Road (3) and Large Method (104 lines) in `InterpolativeResizeImage`, OpenMP-bound: its channel loop, the depth-4 site, assigns the shared `status` from `InterpolatePixelChannels` (`MagickTrue` too) and tests it inside `omp parallel for`, which Recipes T and J refuse; the other bumps are the loop's leading `continue`s and the atomic progress block; the parallel loop alone is over 70 lines. Excess Arguments: public signatures (`InterpolativeResizeImage` 5, `LiquidRescaleImage` 6 and its uncompiled non-LQR stub 5, `ResampleImage` 5). Lines of Code in a Single File (1,341): Recipe S awaits the session. `LiquidRescaleImage` (LQR compiled, no case reaches it) left Bumpy Road, Complex and Large Method with E and P at guard level; `ReportResizeFilter` left Excess Arguments with B (`resize_filter`, `B`, `C`; flat). 2.89 at M01, 6.50 at W08; from 6.50 with E, P, B and W. See W10 | 2026-10-31, WSL (W10) |
| `MagickCore/magnify.c` | 6.57 | Complex and Large Method: `Hq2XHelper` is a rule table written as a `switch` (splitting its cases yields clones with 7-8 arguments, 5.16 in a probe); `Fish2XBottomRightPixel`'s ladder shares six flags that only a struct declared after statements could carry; the 3x3 kernels and `IsEpbx2XBlendable` split into clones of each other (5.93 in a probe). Bumpy Road and Large Method in `MagnifyImage`: its last bumps are the parallel loop's `continue` and the atomic progress block. Overall Code Complexity: the mean of the above. From 6.21, with Recipes E, T, M and B. See [M01's report](tasks/M01-resize.md) | 2026-10-24, Mac |
| `MagickCore/compare.c` | 2.43 | FFTW is compiled now (`--with-fftw`, W08), so the `SIM*` region is measured and reached. Deep Nesting stays in `SimilarityImage` (its `omp critical` best-match `switch` nests two `if`s at depth 4 and cannot move; extracting its channel loop, metric `switch` and best-match `switch` (L then Z) left it flat, and the frequency-domain dispatch returns a variable, which no recipe takes) and in `SIMFilterImageNaNs` (no case; its channel or row loop as a helper joins the `SIM*` shell duplication). Brain Method in `SimilarityImage` (seven public arguments, depth 4). Complex Method in the `SIM*` row shells (cc 10 from the parallel loop's `status`/`continue` paths: moving the row tail needs a `continue` Recipe T does not take, and was flat on `SIMSquareImage`) and in `MSESimilarityImage`/`NCCSimilarityImage` (cleanup-throw macros whose blocks write what the cleanup reads, refusing V step 7). Code Duplication: the row shells, the `Accumulate*Pixel` helpers and the `GetPSNR`/`GetRMSE`/`GetDSSIM` wrappers differ in more than one value. From 2.43 on, steps that took a function out of a category (`AccumulateCorrelationPixel`, Excess Arguments; `CompareImages`, Bumpy Road) and probes that did (`SIMSquareImage`, Complex Method; the trait test as `IsChannelSkipped` at 33 sites) left the score flat. See [W08's report](tasks/W08-night-session.md) and M02's | 2026-10-30, WSL (W09) |
| `MagickCore/colorspace.c` | 3.15 | Bumpy Road (10 each) and Deep Nesting (4) in `sRGBTransformImage` and `TransformsRGBImage`: every row loop's `if (status == MagickFalse) continue;`, `if (q == NULL) { status=MagickFalse; continue; }` and `if (sync == MagickFalse)` stay in the parallel body (T and U refuse a block that reads `status` or `continue`s the parallel loop), the depth-4 site is the `omp atomic` progress block, and each `switch` case holds its own `#pragma omp parallel for`. Probes: removing those three `if`s in every row loop would take `sRGBTransformImage` from 10 bumps to 3 (3.12), but moving the column loops alone is flat (M06) and as helpers adds *Code Duplication* (2.80); the property parsing (`white-luminance`, `film-gamma`, `reference-black`, `reference-white`, 8 sites) as one helper is flat; removing 140 lines is flat. Complex and Large Method and Excess Arguments in the public `ConvertGenericToRGB`/`ConvertRGBToGeneric` (Recipe SD chains to bring each piece under cc 9 give five functions per side, reported as *Code Duplication*: 2.83); Excess Arguments in the public `ConvertHSLToRGB`/`ConvertRGBToHSL`. From 2.80 with D, V, E, O, N (W08), then D (W09, `SetImageGray`/`SetImageMonochrome`, leaving *Code Duplication*). See [W08's report](tasks/W08-night-session.md) and [M06's](tasks/M06-colorspace.md) | 2026-10-30, WSL (W09) |
| `MagickCore/fourier.c` | 6.02 | With FFTW compiled (W08): `ForwardFourierTransformImage` and `InverseFourierTransformImage` keep most of their complexity inside `omp parallel sections`; the normalize blocks and `InverseFourier`'s merge hold `#else` branches for no `complex.h`, which this build does not compile; the remaining loop pairs become *Code Duplication* when moved (5.66, 5.67 in probes); `ComplexImages`' column loop advances six row pointers (seven or more parameters). See W11 | 2026-10-31, WSL (W11) |
| `MagickCore/semaphore.c` | 9.38 | Code Duplication between `LockSemaphoreInfo`, `UnlockSemaphoreInfo` and `RelinquishSemaphoreInfo`: shared assertions and preprocessor structure, a different primitive in each, most of the text in branches not compiled here. See [M19's report](tasks/M19-semaphore.md) | 2026-10-25, Mac |
| `MagickCore/shear-rotate.c` | 7.63 | Six functions; a seventh switches on *Overall Code Complexity* (mean cc over 4, `IntegralRotateImage` alone is cc 50), 7.63 to 7.20 in a probe (the page-offset `if`s as one helper), and nothing legal brings the mean under 4. `IntegralRotateImage` keeps Deep Nesting (5), Bumpy Road (10), Complex and Large Method: its three `switch` cases each hold their own `#pragma omp parallel for`, which no recipe moves; the tile loops' `y` loops read the shared `status` (`if (status == MagickFalse) continue;`, which T and U refuse), the 270-degree sync sits under `omp critical`, and the progress blocks write the shared `progress`. The 90- and 270-degree column loops as Recipe N helpers differ in one operator (`-=`/`+=`), 7.55 to 7.56 for two near-identical functions (rejected). From 7.05 with E, D, SV. See W10 | 2026-10-30, WSL (W10) |
| `MagickCore/threshold-auto.c` | 8.36 | Deep Nesting (4), Bumpy Road (3) and Complex Method (cc 13) in `KapurThreshold`: the depth-4 sites are its black and white entropy sums, which differ in the loop's bounds and in the divisor (`cumulative_histogram[j]`, `1.0-cumulative_histogram[j]`), three values, so SV and TW refuse one helper; as two Recipe E helpers they are reported as *Code Duplication* and take scalar parameters that switch on *Primitive Obsession* (7.82 to 7.66 in a probe; the black sum alone 7.62). Its allocation-failure cleanup as a Recipe N helper (`KapurFailure`) also switches on *Primitive Obsession*, 8.36 to 8.15. Bumpy Road (2) in `TriangleThreshold`: the end-bin search is `FindTriangleStartBin` run backwards (both as helpers: *Code Duplication*), and the search above the peak as a second Recipe Q helper is reported as a duplicate of `ScanTriangleBelowPeak` (8.36 to 7.62). Overall Code Complexity: the mean of the above. From 7.42 with E, H and Q (as amended). See W10 | 2026-10-30, WSL (W10) |
| `MagickCore/statistic-function.c` | 9.23 | Six functions; a seventh switches on *Overall Code Complexity* (mean cc over 4: `FunctionImage` alone is cc 13, and twelve parameter-default ternaries sit in the three case helpers), so every remaining step that needs a new function costs more than it gains: the column loop as a Recipe N helper takes `FunctionImage` out of *Large Method* (71 lines before) and still scores 8.45 in a probe. `FunctionImage` keeps Bumpy Road (2), Complex Method (cc 13) and Large Method without a new function: its bumps are the parallel loop's `if (q == NULL) { status=MagickFalse; continue; }` (which skips the progress block, so no T or LB form moves it) and the `omp atomic` progress block; the preamble's `assert`s and `LogMagickEvent(...GetMagickModule()...)` would report another function and line if moved. Excess Arguments: `FunctionImage` is public (5); `ApplyFunctionToPixel` (6) under Recipe B would put the struct local in `FunctionImage`, which then grows to 72 lines while keeping *Large Method* (9.22 in a probe). The `(number_parameters >= k) ? parameters[k-1] : d` defaults differ in three values and occur nowhere else in MagickCore, so no recipe was added for them. From 8.70 with E (the Sinusoid, Arcsin and Arctan cases) and B. See W10 | 2026-10-31, WSL (W10) |
| `MagickCore/resize-sample.c` | 9.47 | Bumpy Road (2), Complex Method (cc 16) and Large Method (70 lines) in `SampleImage`, all OpenMP-bound or position-bound: its bumps are the parallel loop's `if (q == NULL) { status=MagickFalse; continue; }`, whose `continue` skips the progress block that stays (so no T, U or LB form moves it), and the progress block itself, which writes the shared `progress`; the preamble's `assert`s, `LogMagickEvent(...GetMagickModule()...)` and `ThrowImageException` would report another function and line if moved; `CloneImage` and the cache-view acquisition write locals read later. The row's `SampleImageRow` call and sync as one Recipe T block would take five arguments (the `SampleRow` struct, `sample_view`, `y`, `q`, `exception`), and a Recipe U struct filled in the loop body grows `SampleImage`; the two `DestroyCacheView` lines cannot join `FinishSampleImage` without a fifth argument. From 9.10 with E (the offset parsing, the tail), and B (`SampleImageRow`, 7 to 4 arguments, 9.18 to 9.47). See W10 | 2026-10-31, WSL (W10) |
| `MagickCore/prepress.c` | 9.61 | Bumpy Road (2) and Complex Method (cc 9) in `GetImageTotalInkDensity`, OpenMP-bound: its bumps are the parallel loop's `if (p == NULL) { status=MagickFalse; continue; }`, whose `continue` skips the column loop that follows (no T, U or LB form moves a head block), and the column loop, which holds the `#pragma omp critical` maximum and writes the shared `total_ink_density` (no recipe moves a pragma or a block writing a shared variable). The column loop as a helper taking `&total_ink_density` scores 10.0 in a probe, and is forbidden for those reasons. The colorspace guard as a helper is flat (9.61) in a probe, and its `ThrowMagickException(...GetMagickModule()...)` would report another function and line. From 9.49 with E (the tail after the loop). See W10 | 2026-10-31, WSL (W10) |
| `MagickCore/effect.c` | 3.52 | No function keeps Deep Nesting or five bumps; fourteen functions each keep Bumpy Road (2-4), Complex and Large Method, five keep Excess Arguments (public signatures), and each category's penalty is saturated: probes removing 11 bumps file-wide, deleting PreviewImage, or taking four functions out of every category leave 3.52; removing five or six four-bump functions whole gives 3.68 or 4.00. Single legal E steps (ShadeImage's cleanup, AdaptiveBlurImage's kernel loop) are flat. A split (Recipe S) is the lever. From 1.59 with E, D, LB, N, Q, Z, T, V (W07-W09). See W10 | 2026-10-31, WSL (W10) |
| `MagickCore/resize-image.c` | 6.83 | Bumpy Road (2), Complex Method (cc 16) and Large Method (102 lines) in `HorizontalFilter` and `VerticalFilter`, OpenMP-bound: their bumps are the parallel loop's `if ((p == NULL) \|\| (q == NULL)) { status=MagickFalse; continue; }`, whose `continue` skips the progress block (no T, U or LB form moves it), and the `omp atomic` progress block; the per-column prelude (`bisect`, `start`, `stop`, the weights loop) writes five loop-body variables, and the pixel loop as a Recipe U helper needs an eight-member struct whose fill is longer than the loop it replaces, growing the filters while they keep *Large Method*. Complex Method (cc 9) in `ResizeImage`: the uncompiled `MAGICKCORE_OPENCL_SUPPORT` branch holds an `if` CodeScene counts (without that branch, 6.61 to 6.77 in a probe before the B step), and the logging, size and identity tests stay for their `GetMagickModule()` and throw. Complex Method (cc 10), Bumpy Road (2), Excess Arguments (10) and Code Duplication in `FilterHorizontalPixel`/`FilterVerticalPixel`: their index formulas differ in more than one value (TW and SV refuse one helper), and Recipe B adds a seven-member fill to each filter, which keeps *Large Method*. Code Duplication between the two filters (twins under renaming except the cache fetches). Excess Arguments: `ResizeImage` is public (5); Recipe B on `VerticalFilter` (7 to 4, as on `HorizontalFilter`) is flat at 6.83 and was not kept. From 6.29 with W, O, D, E, P (W09), then Y, P, E, D, B (W10). See W10 | 2026-10-31, WSL (W10) |
| `MagickCore/layer.c` | 7.07 | Lines of Code in a Single File (1,479), past the threshold since before the campaign: only a split (Recipe S, not asked for) reaches it. `OptimizeLayerFrames` keeps Bumpy Road (3), Complex Method (cc 33) and Large Method (255 lines): its validation loop and allocations throw through `ThrowImageException`, whose `GetMagickModule()` would report another function if moved, and every branch of the frame loop that holds its bumps (the previous, background and expanded-background trials and their acceptance tests) contains `#if DEBUG_OPT_FRAME` blocks this build does not compile; the moveable pieces left (the background clone, Recipe V; the deleted-frame marking, Recipe LB step 7) are flat at 7.07 in probes. `CompareImagesBounds` keeps Bumpy Road (3) and Complex Method (cc 12): its four edge scans are one loop each, and any two of them as helpers are reported as Code Duplication (6.11 in probes, column or row, ascending or descending), while their headers and fetch geometry differ in more than one value, so SV, TW and D refuse one helper; one scan moved (Recipe E) was kept. Excess Arguments: `CompositeLayers` is public (6); `IsBetterFrameBounds` (5) can be bundled (Recipe B) only with a struct local that grows `OptimizeLayerFrames`; `CompositeCanvas` (6) bundled into the existing `LayerCompositeInfo` passed through from its three callers is flat at 6.52 in a probe and needs an amendment to Recipe B (reuse of a struct, pass-through, a parameter the body writes). Overall Code Complexity follows from the above. From 4.27 with P, E, D, I, V (W08, W09), then B, N, E, Y, V, K (W10). See W10 | 2026-10-31, WSL (W10) |
| `MagickCore/paint.c` | 6.24 | Lines of Code in a Single File (1,145; suppressed by the owner). `FloodfillPaintImage` keeps Bumpy Road (5), Complex Method (cc 27) and Large Method (156 lines): its bumps sit around `PushSegmentStack`, a macro that names the caller's locals, pushes on success and on overflow destroys four resources and throws through `ThrowBinaryException`, whose `GetMagickModule()` would report another function if moved, so no block holding it can leave; the segment pop writes five locals; the offset tests as one Recipe P predicate are flat (6.24). `OilPaintImage` keeps Bumpy Road (2), Complex Method (cc 16) and Large Method (101 lines): its bumps are the parallel loop's `if ((p == NULL) || (q == NULL)) { status=MagickFalse; continue; }`, whose `continue` skips the progress block (no T, U or LB form moves it), and the `omp atomic` progress block; the histogram failure throws through `ThrowImageException`. `OpaquePaintImage` (cc 11, 74 lines) and `TransparentPaintImage` (cc 11) keep Bumpy Road (2) and Complex Method for the same two OpenMP-bound blocks, and Opaque *Large Method*: its preamble's `ConformPixelInfo` calls write the locals the row struct reads (W step 3 refuses them). Their column loops are N helpers of their own (`OpaquePaintRowPixels`, `TransparentPaintRowPixels`), which differ in two places (target expression and action), so TW and SV refuse one helper; neither is reported as duplication. `TransparentPaintImageChroma` (Bumpy Road 3, cc 14, 76 lines) has no case (only Magick++ calls it), so only E, G and P apply: its column loop takes seven inputs, and its preamble is `PrepareTransparentImage`'s body, which only Recipe W step 7 (oracle level) may reuse. Excess Arguments: public signatures (`FloodfillPaintImage`, `GradientImage`, the three Opaque/Transparent functions). Overall Code Complexity: the mean of the above. From 6.10 with N, W, N, W, RF (W10). See W10 | 2026-10-31, WSL (W10) |
| `MagickCore/histogram.c` | 9.68 | Excess Number of Function Arguments only. `MinMaxStretchImage` (5) is public API. `UniqueColorsToImage` (5, `static`, recursive) takes Recipe B (its four invariant parameters in one struct; the recursive call fills a `const` copy from the bound locals), which removes its finding but leaves the file flat at 9.68, since `MinMaxStretchImage` keeps the category: not committed (rule 2, no later step). Other probes, flat: `IsPixelInfoColorMatch`'s `beta` line calling `GetColorMatchAlpha` (Recipe D, cc 8 to 7). Rejected: `CheckImageColors`' count-or-add as its own LB helper is reported as *Code Duplication* of `CountHNodeColor` (and its column loop then of `ClassifyRowColors`), 8.81; the two differ in the search, the append and the colour limit, so D, TW and SV refuse one helper; the column body as one LB helper avoids it. From 6.39 (W09) to 7.47, then 9.68 in W10 with E, Y, W, LT, LB, LT (Recipe LT added). See W10 | 2026-10-31, WSL (W10) |
| `MagickCore/resample.c` | 7.42 | `ResamplePixelColor` keeps Complex Method (cc 26), Bumpy Road (3) and Large Method (145 lines): what is left of its virtual-pixel `hit` switch is the image group (six labels and an `if`), which as a function needs a `default` and is then cc 9 (7.23 in a probe), and the tile group (six labels, empty body), which alone would be a function that does nothing; its `if` alone as a Recipe E helper takes the bumps 3 to 2 and is flat (7.40). The scan loop's braces interleave with the uncompiled `FILTER_LUT` and `DEBUG_HIT_MISS` branches, and its head (fetch, then `return(MagickFalse);`) writes `u1`, `Q`, `DQ` and `pixels`, whose Recipe I or Y fill would grow the function. `ScaleResampleFilter` keeps Large Method (95 lines), most of them in uncompiled `#if EWA` and `DEBUG_ELLIPSE` branches; its compiled coefficient lines write `A`, `B`, `C` and `F`, and its preamble as a Recipe V helper is flat (7.42). Excess Arguments: `ResamplePixelColor` and `ScaleResampleFilter` are public (5); `ClampUpAxes` (10) under Recipe B or I is flat (7.30), since the category stays. Code Duplication: `IsAreaOutsideCorner`/`IsAreaOutsideDitherCorner` differ in the edge literal (`0.0`/`-32.0`) and in the far edges (`-1.0`/`+31.0`, operator and literal), so D, SV and TW refuse one helper; one function taking the three bounds in a struct leaves the two callers' fills reported as duplicates (flat in a probe). `SetResampleFilterInterpolateMethod`/`SetResampleFilterVirtualPixelMethod` share only the assert-and-log prologue, whose `GetMagickModule()` would report another function if moved. Overall Code Complexity: the mean of the above. From 5.13 (M51) to 7.30 (W09), then SD (step 7 added), Q, Y and V (step 6's `void` form added) in W10. See W10 | 2026-10-31, WSL (W10) |
| `MagickCore/attribute-depth.c` | 7.33 | `GetImageDepth` and `SetImageDepth` keep Deep Nesting (4), Bumpy Road (5, 4), Complex Method (cc 27, 25) and Large Method (127 lines each), nearly all of it in their `#if !defined(MAGICKCORE_HDRI_SUPPORT)` depth-map branches, which this HDRI build does not compile. What the build compiles of them is bound: the HDRI row loops read and write the shared `status` and `continue` (T and U refuse what is left), the `assert`s, `LogMagickEvent(...GetMagickModule()...)` and `ThrowFatalException` would report another function and line if moved, and `SetImageDepth`'s colormap loop carries its own `#pragma omp parallel for`. `GetImageDepth`'s `current_depth[i]=1` loop as a Recipe E helper scores 7.32 from 7.30 but leaves `i` unused in this build (`-Wunused-variable`; its other use is in the uncompiled branch, whose declaration may not move). Reusing `FinishImageDepth` in `FloydSteinbergImageDepth` (Recipe D, no reported duplication) is flat. Overall Code Complexity: the mean of the above. Reaching 10.0 needs a build that compiles the non-HDRI branch (then guard- or oracle-verified there) and a ruling on the colormap loop's pragma. From 6.37 (W09, after the split) with N, Q, E, U, P, then SV (step 2 amended), P and E in W10. See W10 | 2026-10-31, WSL (W10) |
| `MagickCore/feature.c` | 5.24 | Line budget: CodeScene counts 999 lines of code, and *Lines of Code in a Single File* appears at 1,002 (a probe: 5.24 to 4.95); every extraction adds 5 to 8 lines, two small ones together measured 4.96. Beyond that, `MeanShiftImage` keeps Brain Method and depth 6 (its window loop assigns the shared `status` from a call, which T and J refuse) and `TraceEdges` needs a two-level extraction whose helper would be a *Complex Method*. A split (Recipe S) is the lever. See W11 | 2026-10-31, WSL (W11) |
| `MagickCore/statistic.c` | 5.84 | `EvaluateImages` (Deep Nesting 4, 4 bumps, cc 29), `EvaluateImage` and `PolynomialImage` are OpenMP-bound: their parallel loops sit inside `if (op == Median)`, and what is left of their bodies is the `status` `continue` blocks and the atomic progress blocks. The *Code Duplication* among `GetImageEntropy`, `Extrema`, `Kurtosis`, `Mean` and `Median` is their logging prologue; the rest differs in more than one value. *Lines of Code in a Single File* needs a split (Recipe S). See W11 | 2026-10-31, WSL (W11) |
| `MagickCore/segment.c` | 5.70 | `GetImageDynamicThreshold` keeps Brain Method, Deep Nesting (4) and 5 bumps, but no case reaches it and no in-tree file calls it, so only E, G, P and D's unreached form apply; its blue loop (three written variables, Recipe I), its allocation and default-cluster exits (W, V) and its removal loop need the oracle. A case reaching it would open them. `Classify` (6 bumps, cc 25) is OpenMP-bound, and its removal loop writes `count` and `head` in a function with a parallel region, which K step 6 and Q step 1 refuse. See W11 | 2026-10-31, WSL (W11) |
| `MagickCore/transform.c` | 5.60 | Every remaining bump is the `#pragma omp atomic` progress block of a parallel loop (a probe removing `FlipImage`'s took it to 0 bumps); shrinking `FlopImage`, `TransposeImage` or `ExcerptImage` exposes the near-identical row-loop skeletons of the flip, flop, transpose, transverse and excerpt functions as *Code Duplication* (probes 5.01 to 5.13); `FlipImage`'s closing block needs a fifth argument; `ChopImage` and `SpliceImage` are held by two parallel loops each and by `ThrowImageException`. *Lines of Code in a Single File* needs a split (Recipe S). See W11 | 2026-10-31, WSL (W11) |
| `MagickCore/quantize.c` | 5.60 | 110 functions: a 111th switches on *Number of Functions in a Single Module* (about -0.12; `SetGrayscaleImage`'s colormap compaction as a helper, 5.48), which rules out every extraction recipe. `SetGrayscaleImage` keeps Deep Nesting (5) from the `omp critical` colormap insert inside its parallel loop; the duplicated pairs differ in more than one value or are assert/log prologues; `KmeansImage`'s seeding leaves through `return(status);` with `status` read later, which W and V refuse. A split (Recipe S) is the lever. See W11 | 2026-10-31, WSL (W11) |
| `MagickCore/threshold.c` | 5.35 | The nine parallel row loops each keep 2 bumps and cc 11 to 14 from their `status` and progress skeleton; a row-loop helper in `PerceptibleImage`, `BilevelImage` or `WhiteThresholdImage` widens the *Code Duplication* group (5.18 in probes). `ColorThresholdImage` (80 lines) is held by its start/stop scaling, which writes two struct locals in place and is not a selection (neither E nor O); `OrderedDitherImage` (79) by its map lookup, which writes `threshold_map` and `map`. Excess Arguments rests on the public `RangeThresholdImage`. See W11 | 2026-10-31, WSL (W11) |

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
| 2026-10-28 | AMEND | E | Step 4: a variable the block writes but nothing after it reads is not counted; the new function declares it with its initialiser, when nothing between that declaration and the block writes it (Recipes T and J already say "or is not read after it"); `tools/extract_case.py` applies it to one case of a function's closing `switch` | quantum-import.c's `Import*Quantum`: each depth case advances the span's `p` and `q` to the end of the function; extracting the cases raised the file 1.88 → 2.34, the fallthrough chains (24, 64, default) 2.34 → 2.59 in a probe | mandate (2026-10-24) |
| 2026-10-29 | NEW | N | Extract Function Taking an Input Struct: an E-legal block whose inputs would give the new function five or more parameters moves to a `static` function taking a `const` struct of the inputs it only reads, filled member by member directly before the call; `const` locals of the old names keep the block text unchanged | `WaveletDenoiseImage`'s threshold loop (six inputs; with Recipe E 2.41 to 2.36 in M05, with the struct 2.84 to 2.90); M05's `SteganoImage` bit switch and M101's `MotionBlurImage` kernel loops refused for the same reason | mandate (2026-10-24) |
| 2026-10-29 | AMEND | E | Step 3: a block whose single output is an array of the caller's, updated element by element, takes the array as a pointer with its uses unchanged, as Recipe K does for arrays | `BlendMeanImage`'s serial first pass sums each channel into `mean[MaxPixelChannels]`; outside OpenMP, so Recipe K (parallel loop bodies) does not name it, and E's "return the single value" cannot return an array. A probe: the function's last depth-4 site goes, 2.43 to 2.49 | mandate (2026-10-24) |
| 2026-10-29 | AMEND | E | Step 7: a loop's whole body may move even when it `continue`s that loop; each such `continue;` becomes `return;` of the new `void` function, whose call is the whole body, so both end the iteration at the same point | `statistic.c`'s GetImageStatistics entropy pass (W06: moving the outer loop gives the helper a Bumpy Road; moving the inner loop needs six parameters), and the per-channel bodies of `effect.c`'s blur family (W07), both refused for want of this | mandate (2026-10-24) |
| 2026-10-29 | NEW | Q | Extract Accumulating Block Taking an Input Struct: a Recipe K accumulation whose inputs would give the new function five or more parameters moves to a `static void` function taking a `const` struct of the inputs it only reads (Recipe N) and a pointer per written scalar (Recipe K), at most four parameters | `BilateralBlurImage`'s plain and alpha-blended per-channel sums (`pixel`, `gamma`, `n`; eight inputs): K gives eleven parameters, N refuses accumulators; a probe of the sums and then the per-channel body (Recipe N), 1.72 to 1.89 | mandate (2026-10-24) |
| 2026-10-29 | AMEND | E | Step 7: inside a `switch` case, a block after which the case runs only its closing `break;` may contain `break;` statements of that `switch` when its single output is one variable: the new function takes it by value and returns it, each such `break;` becomes `return(v);`, and the block becomes `v=Helper(...);`, so both reach the case's end with the same value | `fx.c`'s `ExecuteRPN`: its pixel-lookup cases (`fU`, `fU0`, `fUP`, `fS`/`fV`, `fP`/`fSP`/`fVP`) hold its nesting of 8 and each sets `regA` and leaves the `switch` by `break` from inside nested `if`s. A probe removing them: depth 8 to 4, fx.c 2.62 to 2.92 | mandate (2026-10-24) |
| 2026-10-29 | NEW | U | Extract Function Returning the Loop Status Taking an Input Struct: a Recipe T block whose inputs would give the new function five or more parameters moves to a `static MagickBooleanType` function taking a `const` struct of the inputs it only reads (Recipe N) and the written row pointers by value; the caller writes `status=MagickFalse` from its result (Recipe T) | `FxImage`'s per-channel loop inside its parallel row loop (seven inputs and `status`): T gives seven parameters, N refuses the `status` write, the whole column loop is a Complex Method (cc 9). A probe: FxImage leaves Deep, Nested Complexity, fx.c 2.74 to 2.80 | mandate (2026-10-24) |
| 2026-10-29 | AMEND | Q | Step 1: a block in a function with no OpenMP region (a sequential pass) qualifies, its written variables being plain locals of the caller (Recipe O step 2); *Careful* notes the cursors (`k`, `pixels`) such a block advances for the next call | `morphology.c`'s `MorphologyPrimitiveDirect`: its forward pass repeats one kernel-row body (`pixel`, `k`, `pixels` written; `image`, `kernel`, `i` read) in the Distance and Voronoi double loops; the function is sequential (two views of one image), so Recipe K's parallel-body wording refuses it, Recipe E step 4 refuses three outputs and Recipe O wants a selection. A probe of the row helper at both sites: nesting 6 to 5, 2.41 to 2.46 | mandate (2026-10-24) |
| 2026-10-29 | AMEND | F | Step 2: a non-`const` static read only through subscripts (no element assigned, incremented or address-taken) qualifies as well | `cipher.c`'s `EncipherAESBlock`: the 52-line AES table `D[]`, read only as `D[...]`; a probe: 6.90 to 7.28 | mandate (2026-10-24) |
| 2026-10-29 | AMEND | F | Step 2: a static that is not declared `const` qualifies when every use is `sizeof` or an argument to a pointer-to-`const` parameter, so nothing can write it; the step-6 proof is unchanged | `profile.c`'s `SetsRGBImageProfile`: a 270-line `static unsigned char` ICC table passed only to `BlobToProfileStringInfo(...,const void *blob,...)`; a probe: 2.60 to 2.92, `SAME CODE` | mandate (2026-10-24) |
| 2026-10-29 | AMEND | W | Step 3: a local nothing reads after the block does not count (as in Recipe E step 4), and neither does one the block writes only on paths that leave through `return(R);`, which goes by value | `SetImageInfo`'s copy to a seekable file writes `status` (not read afterwards) and `image` (destroyed only before a `return`) | mandate (2026-10-24) |
| 2026-10-29 | NEW | W | Extract Block Returning Success for an Early Return: a block outside OpenMP that leaves through `return(R);`, `R` a constant or read through a pointer the caller holds, and writes no caller local, moves to a `static MagickBooleanType` function, each `return(R);` becoming `return(MagickFalse);`; the caller writes `if (Helper(...) == MagickFalse) return(R);`, and identical copies call the same function (D) | `xml-tree.c`'s five buffer-growing blocks in `XMLTreeTagToXML` and `EncodePredefinedEntities`, refused by E (a mid-function `return`) and V (no `goto`). A probe of all five: 2.39 to 2.44 | mandate (2026-10-24) |
| 2026-10-29 | NEW | V | Extract Block Returning Success for a Cleanup Jump: a block outside OpenMP that leaves through `goto L;` and writes at most one caller local moves to a `static MagickBooleanType` function, each `goto L;` becoming `return(MagickFalse);`, the local passed by pointer; the caller writes `if (Helper(...) == MagickFalse) goto L;` | `MorphologyApply`'s work-image clone inside four nested loops (nesting 6), refused by every extraction recipe for its `goto error_cleanup`; the plateau of the earlier W07 pass on `morphology.c`. A probe: nesting 6 to 5, 2.46 to 2.51 | mandate (2026-10-24) |
| 2026-10-29 | NEW | Wa | Extract Accumulating Block Returning the Loop Status Taking an Input Struct: a Recipe J block whose inputs would give the new function five or more parameters moves to a `static MagickBooleanType` function taking a `const` struct of the inputs it only reads (Recipe N) and a pointer per written accumulator or cursor (Recipe K); the caller writes `status=MagickFalse` from its result (Recipe T) | `MotionBlurImage`'s plain kernel sum (nesting 6; `pixel`, `k` and `status` written, seven inputs): J gives ten parameters (M101), Q and U refuse the status or the accumulator. A probe: nesting 6 to 5, 2.06 to 2.10 | mandate (2026-10-24) |
| 2026-10-29 | AMEND | Wa | Step 3: instead of the `const` locals, the body may write `name->member` at every use of a member (Recipe B step 3's form); *Careful* names it as the way to extract the second of a plain and a blended sum without the two helpers repeating each other's locals | `MotionBlurImage`'s blended kernel sum beside `SumMotionPixels`: with `const` locals the pair is reported as Code Duplication (2.06 to 2.01); in the member form no duplication, 2.18 to 2.20 | mandate (2026-10-24) |
| 2026-10-29 | AMEND | U | Step 3: the member form of Recipe Wa step 3. Step 7: a block that is the whole body of a channel loop inside the parallel loop body may contain `continue;` of that loop, each becoming `return(status);`, as Recipe E step 7 does for a `void` function | `MotionBlurImage`'s per-channel body after both kernel sums moved: the channel loop as one helper is a Complex Method (cc 11, its `continue`s counted); the body as a helper (cc 6) called from a channel-loop helper (cc 3) takes the function out of Brain Method and Deep, Nested Complexity, 2.20 to 2.43 in a probe | mandate (2026-10-24) |
| 2026-10-29 | AMEND | V | Step 6: a block that leaves through `return(c);` of one constant `c` of the caller's return type, in place of `goto L;`, moves the same way; each `return(c);` becomes `return(MagickFalse);` and the caller writes `if (Helper(...) == MagickFalse) return(c);`. *Careful* gives the argument and notes the warning location the oracle normalises | `morphology.c`'s `MorphologyImage`: its `convolve:scale` branch (clone the kernel if it is the caller's, `return((Image *) NULL)` when the clone fails, scale it) is its only depth-4 site and both its bumps; Recipe E's step 6 covers only the throw macros' hidden returns and step 4 refuses the status plus `curr_kernel`. A probe: `MorphologyImage` keeps only its public signature's Excess Arguments, 2.55 to 2.62 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | N | Step 4: instead of the `const` locals, the body may write `name->member` at every use of a member (Recipe B step 3's form, as Recipes U and Wa step 3 already allow); *Careful* gives the argument and names the duplication it avoids | `distort.c`'s `SparseColorImage`: the Voronoi search beside `ManhattanSparseColor`, and the Bilinear plane beside the Barycentric one, are reported as Code Duplication with `const` locals (2.70 to 2.57); in the member form neither is (2.70 to 2.74). With the remaining cases the function leaves Brain Method and Deep, Nested Complexity, 2.70 to 3.01 in a probe | mandate (2026-10-24) |
| 2026-10-30 | AMEND | Q | Step 4: the member form of Recipe N step 4 | `SparseColorImage`'s inverse-distance accumulation (`pixel`, `denominator` written, eight inputs): with `const` locals its helper would repeat `ManhattanSparseColor`'s seven locals and loop header; in the member form no duplication | mandate (2026-10-24) |
| 2026-10-30 | NEW | Y | Extract Block Returning Success Taking an Input Struct: a Recipe V block whose inputs would give the new function five or more parameters moves to a `static MagickBooleanType` function taking a `const` struct of the inputs it only reads (Recipe N) and its one written variable by pointer (Recipe V); temporaries it writes before reading, and the caller does not read after it, are declared in the new function (Recipe J step 3) | `visual-effects.c`'s `PlasmaImageProxy`: its middle pixel (`return(MagickTrue)` on a failed fetch; `status` written; five inputs) needs six parameters under Recipe V. A probe: bumps 5 to 4, 3.12 to 3.19 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | Q | Step 2: a variable of the caller that the block writes before reading it, and that the caller does not read after the block before writing it again, is declared in the new function and is not one of the four written variables (Recipe J step 3). Step 5: `&v` of a written scalar becomes `v` | `visual-effects.c`'s `SteganoImage`: its column loop (sequential) advances the cursors `c`, `k` and `j` and writes the temporaries `x`, `pixel` and `q`, declared at the top of the function and used nowhere else; step 1 counted six written locals. A probe of the loop as a helper (nine inputs): the function leaves Deep, Nested Complexity, 3.19 to 3.27 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | F | Step 2: a non-`const` array whose every use is an element read or an argument to a `const`-qualified pointer parameter counts as never written, and moves unchanged | `resize.c`'s `AcquireResizeFilter`: its `static double jinc_zeros[16]` (18 lines of Bessel zeros) is only indexed, and passed to `SetJincSupport`'s `const double *`; it is the last data in the function. A probe: with it and four extractions the function leaves Large Method, 5.83 to 6.18. Machine code and data unchanged (objdump on Linux) | mandate (2026-10-24) |
| 2026-10-30 | NEW | Z | Extract Selection with Out-Parameters Taking an Input Struct: a Recipe O selection whose inputs would give the new function five or more parameters moves to a `static void` function taking a `const` struct of the inputs it only reads (Recipe N's struct) and one pointer per local it sets (Recipe O) | `threshold.c`'s `ColorThresholdImage`: the colorspace `switch` sets `start` and `stop` and reads four inputs (six arguments under O); a probe 4.46 to 4.58 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | V | Step 7: a block whose every exit is the same cleanup run `S` followed by `return(v);` moves with `S` left in the caller: each exit becomes `return(MagickFalse);` and the caller writes `if (Helper(...) == MagickFalse) { S return(v); }`; the block may not write what `S` reads or writes. *Careful* gives the argument: the caller runs the same cleanup on the same values and returns the same variable, so nothing rests on the destroy function returning NULL | `threshold.c`'s `GetThresholdMapFile`: its level-parsing tail has four exits of `thresholds=DestroyXMLTree(thresholds); map=DestroyThresholdMap(map); return(map);`, which step 6 refuses (a variable, not a constant). A probe: cc 18 to 12, 150 to 104 lines, 4.60 to 4.71 | owner |
| 2026-10-30 | AMEND | V | Step 7: `return(c);` of one constant `c` (step 6) may end every exit in place of `return(v);`; the caller writes `{ S return(c); }` and step 2 holds (one written variable, which `S` neither reads nor writes). *Careful*: the cleanup's writes are dead on the failure path, but the recipe does not rest on that, since `S` stays in the caller; moving `S` into the helper instead is not admitted | Owner's request of 2026-10-30 after the W08 report (V allowing dead cleanup writes on the failure path). `layer.c`'s `OptimizeLayerFrames`: the `NoneDispose` reference's failure path releases `bounds` and `disposals`, then `return((Image *) NULL);`, which step 6 refused (three written variables) and step 7 did not name (a constant, not a variable). A probe: bumps 9 to 8, 4.99 to 5.00; with `CoalesceImages`' dispose-image clone, 5.01 | owner |
| 2026-10-30 | AMEND | N, Q, U, Wa, Y, Z | N step 2: a plain struct value the block only reads (`PixelInfo`, `PointInfo`, `RectangleInfo`) may be a member, copied by `name.v=v;` directly before the call, when every member is set before the call, the block assigns neither it nor a member, takes `&v` only for a callee's pointer-to-`const` parameter that only reads through it (stores and compares no address), no pointer to it is held where the block can write, and the block writes through none of its pointer members; arrays stay out. *Careful* gives the argument (a copy holds the same values while nothing writes the original; a `const` callee sees the copy's address). Q step 3, U step 2, Wa step 2, Y step 3 and Z step 2 name struct values as N step 2 does | Owner's request of 2026-10-30 after the W08 report (struct-valued read-only members in N and Q). `feature.c`'s `MeanShiftImage`: its convergence block reads `sum_location`, `sum_pixel`, `previous_location`, `previous_pixel` (`PointInfo`/`PixelInfo`) and `count`; `layer.c`'s `CompareImagesBounds` and `OptimizeLayerFrames` (`RectangleInfo try_bounds`, a `PixelInfo` read through `&alpha_pixel`). MeanShiftImage's block also writes the struct values `mean_location` and `mean_pixel`, which Q passes only as scalars, so this amendment alone does not admit it: a probe that also passed them by pointer measured 5.20 to 5.23 | owner |
| 2026-10-30 | AMEND | Q | Step 1: in a function with no OpenMP region, `status` is a plain local of the caller and may be one of the four written variables, passed by pointer, its reads in the block included; a pointer the block writes through but never assigns is a read-only member, not a written variable. *Careful* gives the argument: K, J and T refuse `status` only because a parallel loop shares it; through `*status` every read and write reaches the caller's variable in the same order, so no guard on its entry value is needed | Owner's request of 2026-10-30 after the W08 report (a sequential form of J and W). `vision.c`'s `ConnectedComponentsImage`: the merge census's neighbour loop (nesting 6, no OpenMP region in the function) writes `status=MagickFalse` and `count` and tests `status`, which J, T and U refuse; in the pointer form its helper takes the struct, `&status` and `&count`. A probe: nesting 6 to 4, 4.18 to 4.21; with the neighbour fetch then moved out of the helper (Recipe E), 4.30 | owner |
| 2026-10-30 | NEW | I | Extract Block Returning Success Through a Struct of Out-Pointers: a Recipe V block (a `goto L;` or `return(c);` exit) that writes two to four of the caller's locals or by-value parameters, which as separate pointers would give five or more parameters, moves to a `static MagickBooleanType` function taking its read-only inputs (as V, or in Y's input struct) and last a `const` struct holding only the written variables' addresses, bound to `*const` locals and used as `*v` (Recipe O). *Careful* states how it differs from the out-parameter struct Recipe E step 4 forbids: the variables stay the caller's own, only their addresses travel, and nothing is copied back | Owner's request of 2026-10-30 after the W08 report (V with an out-parameter struct). `fx.c`'s `GetFunction`: the coordinate-qualifier block (depth 4, the file's last Deep Nesting) writes `fe`, `funStr`, `chQual` and `expChLimit` and returns `MagickFalse` on one path, refused by V, Y, O, Q and E step 4. A probe: 3.08 to 3.23 | owner |
| 2026-10-30 | AMEND | V | Step 2: a temporary the block writes before reading, and the caller does not read after it before writing it again, is declared in the new function and is not the one written variable (Recipe Y step 2, Recipe J step 3). *Careful* gives the argument: neither its entry value nor the value the block leaves is ever read | `morphology.c`'s `AcquireKernelInfo`: the token step of its kernel-list loop writes `kernel` and the parse result `new_kernel` (declared at the top of the function, used nowhere else) and returns `(KernelInfo *) NULL` when a parse fails; step 2 counted two written variables, and Recipe Y (whose step 2 already admits the temporary) needs five inputs. A probe of the token step (V) and the `@file` read (Recipe I): the function leaves Bumpy Road Ahead and Complex Method, 2.68 to 2.70 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | Q | Step 8: when the written scalars as separate pointers would give five or more parameters, their addresses go in one `const` struct (Recipe I's form), bound to `*const` locals; inside a parallel loop every address is of a loop-body variable; in this form up to six written variables. *Careful* gives the argument: the variables stay the caller's own, only their addresses travel, and none of a variable the threads share | Session's brief after the W08 report (`effect.c`'s plateau: the adaptive and selective kernel sums advance four or five cursors and accumulators). `AdaptiveBlurImage`'s plain sum writes `pixel`, `gamma`, `k` and `pixels` and reads four inputs: Q gives five parameters. A probe of that sum: nesting 6 to 5, bumps 9 to 8, 2.54 to 2.59 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | Q | Step 8: instead of binding the `*const` locals, the body may write `targets->v` for every use of `v` (`*targets->pixel+=`, `(**targets->k)`), the member form of step 4; *Careful* gives the argument and the duplication it avoids | `SelectiveBlurImage`'s plain and blended kernel sums (`pixel`, `gamma`, `k`, `pixels`, `luminance_pixels`): with bound locals in both helpers the pair is reported as Code Duplication (2.77 to 2.65); with the blended one in the member form, none, 2.77 to 2.84 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | E | Step 7: a block that ends a loop's body, after statements that stay, may contain `continue;` of that loop; each becomes `return;` of a new `void` function whose call is the body's last statement (the whole-body form's conditions; step 7's first form, for a loop body instead of a `void` function) | `effect.c`'s `SelectiveBlurImage`: its channel-loop body moved whole (Recipe N) gives a helper of 79 lines (*Large Method*), refused by the gate; its tail after the trait checks (the two kernel sums and their stores, three `continue`s) moved first, then the body: the function leaves Brain Method and Deep, Nested Complexity, 2.84 to 3.12 in a probe | mandate (2026-10-24) |
| 2026-10-30 | AMEND | D | *Unreached functions*: in a function no oracle case executes (casemap 0 for it and its callers in the catalogue), a block identical to an existing `static` helper's body (checked by script) may become a call of that helper when `tools/obj_identical.sh` prints `IDENTICAL` for the whole object file against `HEAD`; nested replacements in one function (an inner block onto one helper, the loop around it onto another) may share the commit; the commit carries `Verified: guard` and quotes that line. *Careful* gives the argument (byte-identical object code under the oracle build's pinned flags is the same compiled code, so the same behaviour on every input) and says a step whose object file differs is not this form: fall back to the oracle or skip. *Two levels of evidence* names it as the one exception for D | `transform.c`'s `ExcerptImage` (0 cases, no in-tree caller): its pixel loop is `CopyPixelRow`'s body with `CopyPixelChannelsTo`'s written out; a probe calling `CopyPixelRow` compared `IDENTICAL` and measured 4.40 to 4.69, the file's last Deep, Nested Complexity. Under the founding evidence level neither D (oracle only) nor E (a new helper would duplicate an existing one) applied | owner |
| 2026-10-30 | AMEND | Q | Step 8: an array the block updates element by element counts as one written variable; its member is a pointer to the array's type, `targets.v=(&v);`, and `v[i]` becomes `(*v)[i]` | `compare.c`'s `GetSSIMSimularity` and `GetPHASESimilarity`: five accumulators per pixel, some of them per-channel arrays; step 8 said "written scalar" while step 1 (through Recipe K step 2) already counts such arrays among the written variables. The address of an array points to the same elements, so every update reaches the caller's array in the same order | mandate (2026-10-24) |
| 2026-10-30 | NEW | SD | Split a Selection at its Default: a `switch` whose groups all end in `break;` moves some of its groups, the `default` group among them, into a new `static void` function with the same `switch (e)` (built as Recipe O or Z builds one); the caller's `switch` keeps the other groups and a `default:` that calls it | `ComplexImages`' operator `switch` (eight labels, depth 4 in the parallel loop): Recipe Z on the whole `switch` gives a cc-9 helper the gate refuses (5.09); the unary group as its own function, then Z on the binary rest, leaves no helper finding, 4.83 to 5.21 in a probe | mandate (2026-10-24) |
| 2026-10-30 | AMEND | Q | Step 5: a written variable may be a plain struct value the block assigns whole (`error=f(...,error);`); it goes by pointer like a scalar, `*v` at every use, parenthesised before `->` or `.` (`(*node_info)->child[id]`), as Recipe Z does for `start` and `stop`. *Careful* gives the argument: the pointer names the caller's own object, so every member is read and assigned where it was, and nothing is copied in or out | `quantize.c`'s `ClassifyImageColors`: each row loop's tree descent (the level loop with its node creation, nesting 5, a `continue` of the level loop inside) writes the cursor `node_info` and the struct `error` (`DoublePixelPacket`) and reads six inputs; step 5 named only scalars, as the N row of 2026-10-30 noted for `MeanShiftImage`. A probe of both descents: the function leaves Deep, Nested Complexity, cc 26 to 16, 4.05 to 4.24 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | I | Step 8: a block whose every exit is the same call of a function-local cleanup-and-throw macro (cleanup statements, then `return(c);`) moves with the call left in the caller, as V step 7 leaves `S`; the macro may read and write the written variables, whose writes have landed through the members. In this form a written variable whose address the caller passes only to calls that keep no copy (`memset`, a `static` callee that does not store it) meets step 2, `&v` becoming `v` in the block. *Careful* gives the argument | `segment.c`'s `Classify`: the cluster allocation inside its triple `while` (the file's last depth-4 site in an oracle-reached function) writes `cluster`, `head` and, through `DefineRegion(extrema[Blue],&blue)`, `blue`, and leaves through `ThrowClassifyException`, which frees the list from `head`; step 1 refused V step 7's form and step 2 the address of `blue` (passed to `memset` and `DefineRegion`). A probe of the innermost `while`: `Classify` leaves Deep, Nested Complexity, cc 35 to 33, 4.93 to 4.96 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | V | Step 7: `S return(c);` may be one call of a function-local cleanup-and-throw macro (Recipe I step 8), left unchanged in the caller; in that form `S` may read and write the block's one written variable, passed by pointer, whose writes have landed before the helper returns. *Careful* gives the argument | `segment.c`'s `Classify`: the default cluster (`if (head == NULL)`, allocate, initialise, `head=cluster`) writes `head` and the temporary `cluster` and leaves through `ThrowClassifyException`, which walks the list from `head`; step 7 refused a cleanup that reads the written variable, and Recipe I needs two written variables. A probe: bumps 7 to 6, 5.09 to 5.11 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | D | Casts to the variable's own type: a cast of a plain variable to the exact type it is declared with (`(Image *) image` of an `Image *` parameter) is not a differing value; the script removes such casts, each type checked by reading, before comparing, and the helper is written without them. A block whose every path ends in `return(<expr>);` becomes `return(<call>);` (Recipe E step 7) | `colorspace.c`'s `SetImageGray` and `SetImageMonochrome` (*Code Duplication*): their closing blocks differ in the type set (`type`, `BilevelType`: one value) and in `SyncImagePixelCache((Image *) image,...)`, a no-op cast; a probe of the shared helper, 3.01 to 3.15, the file leaving *Code Duplication* | mandate (2026-10-24) |
| 2026-10-30 | AMEND | rule 1 | The exception for a set of steps that each lower the score and together raise it covers several blocks of one function as well as several functions; the message names each block with the score it gave alone | `paint.c`'s `FloodfillPaintImage`: its two depth-4 sites, the right-hand fill and the skip scan, each alone 5.10 to 4.84 (the file passes the 1,000-line threshold), both 5.77 (Brain Method and Deep, Nested Complexity leave) | mandate (2026-10-24) |
| 2026-10-30 | AMEND | rule 1 | A set of steps that each lower the score and together leave it flat is decided by rule 2, as a single flat step is: kept when a targeted function leaves a finding category, with the finding and the next step named in the message | `attribute-depth.c`'s `FloydSteinbergImageDepth` (task W09): Recipe N on its error distribution (the right and bottom neighbours) alone 6.37 to 5.83, and on its row body alone 5.49 (the new function inherits the depth-5 nesting); any new function in this six-function file switches on *Overall Code Complexity*. Both together leave the file at 6.37 while the function leaves *Deep, Nested Complexity*, *Bumpy Road Ahead* and *Large Method*; the next step (Recipe E on `SetImageDepth`'s row) raises it. The 2026-10-28 wording asked the set to raise the score, which only the set plus a step of another recipe does | mandate (2026-10-24) |
| 2026-10-30 | AMEND | E | OpenMP regions: a block inside a parallel loop body may write the thread's own slot of a per-thread array through the pointer it already uses (`current_depth[id]`, `id` the loop body's `GetOpenMPThreadId()`, not written by the block); the pointer passes by value. The section named only pixel pointers; Recipe U's *Careful* already accepts the same write (`pfx->fxrts[id]`) | `attribute-depth.c`'s `GetImageDepth` (task W09): its HDRI row loop raises `current_depth[id]` in a `while` loop at depth 5 inside `omp parallel for`; the loop as a function writes nothing else. A probe: the function's nesting 5 to 4, bumps 7 to 6, the file 6.60 to 6.64 | mandate (2026-10-24) |
| 2026-10-30 | NEW | LB | Extract Block Returning Success for a Loop Break: a block that leaves its enclosing loop through `break;` moves to a `static MagickBooleanType` function, each such `break;` becoming `return(MagickFalse);`, and the caller writes `if (Helper(...) == MagickFalse) break;`; inputs by value or in a `const` struct (Recipe Y), one written variable by pointer, temporaries declared in the helper | `paint.c`'s `FloodfillPaintImage`: its two depth-4 sites end the `do`-loop through `break;` when a fetch or a sync fails, which V, Y and I refuse (V step 1) and E step 7 admits only for a `switch`; a probe of both, 5.10 to 5.77 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | T, U | T step 4 (U step 1): a block that ends the parallel loop body may contain `continue;` of the parallel loop; each becomes `return(status);` and the call `if (Helper(...) == MagickFalse) status=MagickFalse;` is the body's last statement (Recipe E step 7's loop-tail form); *Careful* gives the argument and excludes a block followed by a progress block | `paint.c`'s `FloodfillPaintImage`: its tiling loop body after the status test is a fetch that `continue`s on failure, a fill and a sync; a probe with Recipe U, 6.06 to 6.10 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | LB | Step 7: a block whose every exit is the same run `S` followed by `continue;` of `Lp` moves with `S continue;` left in the caller, `if (Helper(...) == MagickFalse) { S continue; }`, as Recipe V step 7 leaves its cleanup; the block writes nothing `S` reads or writes. *Careful* gives the argument, including `S` writing the loop counter | `statistic.c`'s `GetImageStatistics`: the depth check inside the channel loop (nesting 5) ends in `IncreaseChannelDepth(...); i--; continue;`, which E step 7 refuses (its counter goes by value; the block is not the body's tail) and LB step 1 refuses (a `continue`); its temporaries `depth`, `range` and `status` move with it. A probe: nesting 5 to 4, bumps 4 to 3, 4.73 to 4.76 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | K | Step 6: a block in a function with no OpenMP region (a sequential pass, as Recipe Q step 1 admits) moves the same way, writing at most four plain locals of the caller, none whose address is taken elsewhere (Recipe O step 2); the written locals may be cursors the caller reads afterwards. *Careful* gives the argument | `statistic.c`'s `GetMedianPixel` (cc 10, two bumps): its partition loop writes the cursors `l` and `h`, which E step 4 refuses (two written variables), O refuses (a loop, not a selection) and Q refuses (two inputs, so K's form has four parameters). A probe: the function leaves Complex Method and Bumpy Road Ahead, 4.91 to 5.07 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | Q | Step 8: the `Targets` local may be declared `const` with the addresses as its initialiser, `{ &v, ... }` in member order, in the block holding the call or inside the scope of every `v`, instead of the `targets.v=(&v);` assignments before the call (Recipe B step 5's `const` form). *Careful* gives the argument: an address does not change while its variable lives, and the initialiser reads no value | `vision.c`'s four moment-threshold functions: the second-moment column loop (nesting 4 in each, inside the parallel object loop) writes `M11`, `M20`, `M02` and the cursor `p` and reads five inputs, so step 7 gives five parameters; with step 8's four assignments per call `MajorAxisThreshold` and `MinorAxisThreshold` enter Large Method (71 lines) and the other two grow. A probe of all four in the `const` form: every caller shorter, 4.30 to 4.73 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | LB | Step 8: a block writing two to four variables of the caller that the caller reads after it passes their addresses in one `const` struct, Recipe I's form (steps 3, 5, 6), after the input struct; the caller fills it before the call or declares it `const` with the addresses (Recipe Q step 8). A variable whose address the caller passes elsewhere only to calls that keep no copy of it still qualifies (Recipe I step 8). *Careful* gives the argument | `vision.c`'s `ConnectedComponentsImage`: the label pass's per-pixel body (the function's last depth-4 site but one) writes `status`, `n`, `p` and `q` and leaves the column loop through `break;` when `n` reaches `MaxColormapSize`; step 2 allows one variable, Q and I refuse the `break`, and `n`'s address goes to `SetMatrixElement` in the initialisation loop. Moving the column loop whole (Recipe Q step 8) leaves its helper a Bumpy Road (2 bumps). A probe of the body in this form: 4.73 to 4.76; with the census column loop (Recipe Q step 8), 5.07 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | O | Step 2: a written variable whose address the caller passes elsewhere, outside the block, only to calls that keep no copy of it (checked by reading the callee) still qualifies, as in Recipe I step 8; Recipes Z and SD, which rest on O steps 1 to 3, take the same reading. *Careful* gives the argument | `transform.c`'s `TrimImage`: its gravity `switch` (cc 19 in the caller, 11 labels) writes only `geometry`, whose address the caller later passes to `CropImage`, which reads `*geometry` through `GetCropPage` and `ClampCropExtent` and stores no pointer. Recipe SD on the `switch`, then E on the min-size block: 4.76 to 4.83 to 5.00 in a probe, TrimImage leaving Complex and Large Method, neither helper with a finding | mandate (2026-10-24) |
| 2026-10-30 | AMEND | O | Step 2: a parameter the caller receives by value, of struct or arithmetic type (not a pointer or array), counts as a plain local, under the same rule on its address; Recipes Z and SD read step 2 the same way. *Careful* gives the argument | `transform.c`'s `ApplySpliceGravity` (cc 12): its whole body is the gravity `switch` (eleven labels), which writes only `splice_geometry`, a `RectangleInfo` received by value and returned. Recipe SD (the default group and North, NorthEast and West to a helper taking `&splice_geometry`): 5.15 to 5.31 in a probe, ApplySpliceGravity leaving Complex Method, the helper with no finding | mandate (2026-10-24) |
| 2026-10-30 | AMEND | I | Step 8: a block whose every exit is the same run of cleanup statements `S` followed by `return(c);` of one constant (Recipe V step 7's `S return(c);`) moves with `S return(c);` left in the caller, as the macro form leaves its call; `S` neither reads nor writes a written variable. Step 1 points to it. *Careful* gives the argument (V step 7's, with the members) | `layer.c`'s `OptimizeLayerFrames`: the duplicate-frame trial (`if ( add_frames )`, nesting 4, one of the function's two depth-4 sites) writes three variables read later (`dup_image`, `dup_bounds`, `cleared`), so V step 7 (one written variable) refuses it, and its failure exit is three plain cleanup statements and `return((Image *) NULL);`, which I step 1 refused and step 8 named only as a macro. A probe with the other depth-4 site (Recipe E): Deep, Nested Complexity leaves, 5.47 to 5.82 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | I | Step 2: a written variable whose address the caller takes only to fill a member of another helper's targets struct under this recipe (filled directly before that call, read by nothing else) still qualifies, as Recipe LB step 2 allows for its own helpers. *Careful* gives the argument | `layer.c`'s `OptimizeLayerFrames`: the finalisation of the dispose choice writes `bgnd_image`, `prev_image`, `dup_image` and `i`, and leaves through the release of `bounds` and `disposals` and `return((Image *) NULL);`; `&dup_image` is already taken for `TryDuplicateFrame`'s targets. A probe: bumps 5 to 3, cc 39 to 33, 5.96 to 5.98 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | E | Step 4: a variable the block reads on entry and then writes, and that nothing after the block reads, goes by value as an input, its text unchanged; its writes land in the new function's copy | `effect.c`'s `AdaptiveBlurImage`: its release of the kernels allocated so far (`for (w-=2; w >= 0; w-=2)`, a bump inside the failure branch) counts `w` down just before `ThrowImageException` returns; step 4 counts no output, but says only that a written temporary is declared in the new function. A probe with the kernel-building loop (Recipe LB): bumps 6 to 4, 3.12 to 3.19 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | V | Step 2: a written variable whose address the caller passes elsewhere, outside the block, only to calls that keep no copy of it (checked by reading the callee) still qualifies, as Recipe O step 2 and Recipe I step 8 already allow. *Careful* gives the argument | `fx.c`'s `GetProperty`: the "not a number" branch (`*val = strtold (text, &tailptr);`, then destroy `text`, throw and `return -1;`) writes only `text`, whose address the caller passes to `GetPropertySeconds` in the other branch; that callee reads and replaces `*text` and keeps no pointer. With the bracket scan moved (Recipes LB and K), a probe: GetProperty leaves Bumpy Road Ahead, 3.23 to 3.25 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | V, I | V step 7 (and I step 8 by reference): the exit may be a single `return(<expr>);` whose expression is a call on the caller's variables (`return(DestroyKernelInfo(kernel));`), the same text at every exit, left unchanged in the caller after `if (Helper(...) == MagickFalse)`; the block writes no variable the expression reads. *Careful* gives the argument (the cleanup form's, with the cleanup folded into the return) | `morphology.c`'s `ParseKernelArray` (cc 25, 2 bumps after three Recipe E steps): its `:` geometry parse writes `flags` and leaves through `return(DestroyKernelInfo(kernel));` twice, and the `:`/odd-square selection around it writes `flags` and `p`; neither is `S return(v);` nor `return(c);`. A probe of both: ParseKernelArray leaves Bumpy Road Ahead, 2.70 to 2.71 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | E | Step 7, `switch` form: a block with no output variable (it writes only through pointers it already uses, or into temporaries step 4 declares) moves to a `void` function; each `break;` of the `switch` becomes `return;`, and the block becomes the call alone, before the case's closing `break;` | `resample.c`'s `ResamplePixelColor`: the whole-image average in the `default` group of the limit-reached `switch` (nesting 4) leaves the `switch` through `break;` when the resize or the pixel fetch fails, after writing `*pixel` and `resample_filter->average_pixel`, and writes no local the caller reads; the form's `v` does not exist. A probe: 5.13 to 5.19, 228 to 197 lines | mandate (2026-10-24) |
| 2026-10-30 | AMEND | D | *One value at several positions; conditions*: the single differing value may stand at several positions of a copy when each copy has the same value at all of them (the helper takes it once); and the copies may be whole parenthesised operands of a logical or or a logical and, the helper then a predicate in Recipe P's form | `resample.c`'s `IsAreaOutsideCorner` and `IsAreaOutsideDitherCorner` (both cc 9, reported as duplicates): their first corner tests, `( u0 + resample_filter->Ulimit < 0.0 && v0 + resample_filter->Vlimit < 0.0 )` and the same with `-32.0`, differ only in that one value, at two positions. A probe: Complex Method leaves the file, 5.21 to 5.46 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | E | OpenMP regions: a block inside the structured block of `#pragma omp single` may be extracted by Recipe E under the parallel-loop proviso (it writes no variable declared outside the structured block, and contains no pragma); the pragma and its braces stay, holding the call | `resample.c`'s `SetResampleFilter` (Large Method, 87 lines): its `resample:verbose` dump of the filter LUT sits in an orphaned `omp single` block and writes only its own `Q` and `r_scale` and stdout. The section names only parallel loop bodies; Recipe O step 1 refuses `single` and points to E. A probe: Large Method leaves the function, 5.46 to 5.72 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | T | Step 2: the loop's status may have another name or type, a variable `v` declared outside the loop body that the parallel loop writes only as `v=c;` of one constant and the body reads only in its leading `if (v == c) continue;`; the helper is step 5's and the caller writes `if (Helper(...) == MagickFalse) v=c;`; *Careful* gives the argument and refuses a flag written another value | `attribute.c`'s `IsImageOpaque` (`opaque=MagickFalse;`) and `IdentifyImageMonochrome` (`type=UndefinedType;`), row loops of T's shape whose flag is not called `status`. A probe on `IsImageOpaque`: bumps 2 to 1, cc 10 to 9, Code Duplication leaves the file, 6.08 to 6.52 | mandate (2026-10-24) |
| 2026-10-30 | WORDING | Wa | Campaign A's Recipe W (Extract Accumulating Block Returning the Loop Status Taking an Input Struct, 2026-10-29) is renamed Wa: campaign B's M112 added a different Recipe W (Extract Block Returning Success for an Early Return) the same day on its own branch, and both reached `main`. B's reached `main` first and keeps W. Campaign A commits made before this rename whose trailer reads `Recipe: W` (W08's `effect.c` steps and the steps that cite Recipe W step 3's member form) mean Wa | Both headings stood in `main` after #75 and #78, which `playbook_lint` refuses as a duplicate code | owner |
| 2026-10-30 | AMEND | V | Step 6: another copy of a step-6 block, in another function and identical under Recipe D's renaming (checked by script), may call the function the first copy became, with its own arguments and its own `c`, one commit per caller, as Recipe W step 7 allows for W; *Careful* gives the argument | `resize-image.c`'s `HorizontalFilter` and `VerticalFilter`: the contribution-buffer allocation, which throws and returns `MagickFalse` when it fails, is the same text in both. Recipe D moves copies whole and may not leave the caller, and a second helper of the same text would be a duplicate. A probe: 6.29 to 6.31 for the first copy, 6.32 with the second | mandate (2026-10-24) |
| 2026-10-30 | AMEND | LB | Step 6: a block whose step-3 text equals, under Recipe D's renaming (checked by script), the body of a function Recipe LB, Y or W already made in the file calls that function instead of a second copy, with its own struct local and its own `break;`, as Recipe W step 7 allows for W; *Careful* gives the argument | `histogram.c`'s `CheckImageColors`: its child-allocation block in the tree descent ends the level loop through `break;` where `ClassifyImageColors`' identical block returns `0`, which Recipe Y moved into `AcquireHNodeChild`; a second copy would be a *Code Duplication*. A probe: nesting 5 to 4, 6.41 to 6.42 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | I | Step 2: a written variable whose address the caller passes elsewhere only as an argument of calls that keep no copy of it (checked by reading the callee) still qualifies, as Recipe V step 2 and Recipe LB step 8 allow; in the block, `&v` becomes `v`, as step 8 already says for its form; *Careful* gives the argument | `histogram.c`'s `ClassifyImageColors`: its tree descent (the level loop at depth 3, its exit at depth 4) writes the node cursor and `pixel`, whose address the caller also passes to `GetPixelInfo` and `IsPixelInfoColorMatch`, neither of which keeps it, and leaves through `return(0);`. A probe: 6.43 to 6.47 | mandate (2026-10-24) |
| 2026-10-30 | NEW | TW | Extract Twin Blocks into One Function Taking an Input Struct: two functions' copies of one block, the same text once each copy's variables are renamed consistently and differing in at most one value (checked by script), move to one `static` function taking a `const` struct of the inputs (Recipe N), its members named for their roles and filled in each caller from that copy's variables; read-then-written inputs go by value, temporaries written before read are declared in the function; both copies in one commit (rule 1) | `shear.c`'s `XShearImage` and `YShearImage`: the left/up and right/down pixel-transfer cases of their direction `switch`es hold the depth-4 sites; Recipe N names members after one copy's variables, Recipe D needs a reported duplication and would take seven parameters. A probe: one function's cases alone 6.47 to 5.99, both 6.47 to 6.63 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | K | Step 3: a variable of the caller that the block writes before reading it, and that the caller does not read after the block before writing it again, is declared in the new function and is not one of the four written variables (Recipe J step 3, Recipe Q step 2); *Careful* gives the argument | `shear.c`'s `CropToFitImage`: its minimum/maximum scan over the four extent corners (a bump: four `if`s in a `for`) writes `min` and `max` and the loop counter `i`, which the caller also uses for the corner loop before it; step 6 would pass `i` by pointer as a third written local. A probe of the scan as a helper (step 6): 7.29 to 7.53 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | H | *Use when*: also a function whose leading `#define` run holds helper macros with branches or many lines, which CodeScene counts in that function's cyclomatic complexity and length; the step is then kept for its own measured effect under rule 2. *Careful* says when the step is flat and when it is measured | `magnify.c` (W10): `Fish2XBottomRightPixel`'s `Corner` (one `if`, 16 lines), `Epbx2X`'s `HelperCond` (a conjunction and three disjunctions) and `Hq2XHelper`'s `caseA`/`caseB`; each hoist preprocessed identically. Probes from 6.90: `Fish2XBottomRightPixel` 7.19 (leaves *Large Method*), `Epbx2X` 7.11, `Hq2XHelper` 6.97 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | SD | Step 6: when the moved groups read `const` locals the caller initialises from its parameters only, the new function may take the caller's parameters and repeat the declarations of the locals it reads, initialisers unchanged; a local the caller no longer reads leaves its declaration; moved groups need not be adjacent, and the step may repeat on the same `switch`. *Careful* gives the argument, the chain, and the duplication to avoid | `magnify.c`'s `Hq2XHelper` (W10): the six neighbour offsets come from its `Hq2XNeighbours` parameter; Recipe Z's struct for nine inputs took the file over *Lines of Code in a Single File* (6.39 in a probe), step 6 twice gives 7.48 to 8.13 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | H | Step 1: a run that follows only declarations at the top of the body, none naming its macros, may be hoisted the same way, the declarations staying; *Careful*: the reach widens over those declarations too, which `pp_identical.sh` covers | `magnify.c`'s `Xbr2X` (W10): its `WeightVar` macro (a `?:`) follows the declaration of `magnify`; the hoist preprocessed identically, cc 10 to 9, 8.13 to 8.15 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | O | Step 2: a variable the block does not read before writing it, and that nothing after the block reads, is declared in the new function (Recipe E step 4) and is no out-parameter. Step 5: `&v` of a written variable, passed on to a callee, is written `v` (Recipe Q step 5) | `MontageImageList`'s three copies of `x_offset=0; y_offset=0; if (montage_info->tile != (char *) NULL) GetMontageGeometry(...,&x_offset,&y_offset,&sans,&sans);`: `sans` is a discarded out-argument that nothing reads, and as a third pointer it would give five parameters. A probe of the first copy: 6.63 to 6.65; all three, 6.68 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | E | Step 3: a single output that is one member of a caller's struct local, whose other members are not all set, goes by passing the struct as a pointer (`(*v).m` at each use, Recipe Q step 5's form) to a `void` function; the struct is a plain local whose address is not taken elsewhere (Recipe O step 2) | `MontageImageList`'s row end in the bounding-box pass, `if (y_offset > (ssize_t) bounds.height) bounds.height=(size_t) y_offset;`, its last depth-4 site: `bounds.x` and `bounds.y` are never set, so returning `bounds.height` through a by-value copy of `bounds` would read indeterminate members. A probe: Deep, Nested Complexity leaves, 6.87 to 7.30 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | I | Step 2: the exception for another helper's targets struct covers a struct of Recipe I, LB step 8 or Q step 8, filled directly before its own call or declared `const` with the addresses (Q step 8's `const` form), read by nothing but that call; LB step 8, which rests on step 2, takes the same reading. *Careful* gives the argument | `resize-scale.c`'s `ScaleImage` (W10): Recipe Q moved the Y direction's row loop to `AccumulateScaleRows`, whose `const` targets struct holds `&n`, `&status`, `&number_rows` and `&next_row` among six; the pending read after the loop (nesting 4, the function's last depth-4 site) writes those four and ends the row loop through `break;`. Recipe LB step 8: Deep, Nested Complexity leaves `ScaleImage`, 7.40 to 7.88 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | O | Step 2: a written variable whose address the caller takes only to fill a member of another helper's targets struct (Recipe I step 2's exception: I, LB step 8 or Q step 8, the `const` form included) still qualifies; Recipes K, Q, Z and SD, which rest on O step 2, take the same reading. *Careful* gives the argument | `resize-scale.c`'s `ScaleImage` (W10): the row-span restart `if (scale.y <= 0) { ... }` writes `scale` and `next_row`, whose addresses only the `const` targets structs of `AccumulateScaleRows` (Q) and `ReadPendingScaleRow` (LB) hold. Recipe O: cc 27 to 26, 7.88 to 7.90 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | N | Step 2: a plain struct local that meets (b) to (d) but not (a), a member of it never set on the path to the call, may be one of the group by address: a `const T *v` member filled `name.v=(&v);`, each `v.m` in the block written `(*v).m` (Recipe E step 3's form); Recipes Q, U, Wa, Y, Z and LB, which take their input struct from N step 2, take the same reading. *Careful* gives the argument | `resize-scale.c`'s `ScaleScanlineX` (W10): its partial-column block `if (scale.x > 0) { ... }` (cc 11 in the caller, the bumps that keep Complex Method) reads `scale.x` of a `PointInfo` whose `y` is never set, so `name.scale=scale;` would read an indeterminate member, and writes `pixel`, `next_column`, `t` and `span`. A probe of Recipe Q step 8 with `scale` by address: Complex Method leaves `ScaleScanlineX`, 7.90 to 8.15 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | I | Step 1: in a `void` function a plain `return;` at every exit stands for `return(c);`, each becoming `return(MagickFalse);` with `return;` in the caller (step 7). Step 7: a block of another function whose step-5 text equals, under Recipe D's renaming (checked by script), the body of a function this recipe made calls that function, one commit per caller, as W step 7 and LB step 6 allow. *Careful* gives both arguments | `shear-deskew.c`'s `RadonProjection` (W10): its pair-merge row loops (nesting 5) share a head (two reads, a sum, a write) that leaves through `continue;`; once the first loop's body moved to the `void` `MergeRadonPair` (Recipe N with E step 7) the head leaves through `return;` and writes `element` and `neighbor`, which the tail reads, so W step 3 and LB step 1 refuse it. Moving the second loop's body as it is gives *Code Duplication* with `MergeRadonPair` (7.39 in a probe); with the head moved first and the second body calling it, `RadonProjection` leaves Complex Method and Bumpy Road Ahead, 7.78 to 7.86 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | T | Step 2: the other name's one read may be a leading guard that tests the loop's `status` and `v` together and `continue`s on either; *Careful* gives the argument | `channel-alpha.c`'s `SetImageAlphaChannel` (W10): the `OffIfOpaqueAlphaChannel` row loop guards on `status` and `opaque` together and writes `opaque=MagickFalse;` in its column scan (nesting 4). Probe of the scan as `IsAlphaRowOpaque`: *Deep, Nested Complexity* leaves, bumps 8 to 7, but as the file's seventh function it switches on *Overall Code Complexity*: 6.96 to 6.77 alone, 7.00 with Recipe E on the `AssociateAlphaChannel` column loop | mandate (2026-10-24) |
| 2026-10-30 | AMEND | rule 1 | The blocks of one function in a set may take different recipes when what lowers each alone is a finding of the whole file that the first new function switches on (*Overall Code Complexity*, from a file's seventh function); the message names each block with its recipe and its score alone, and the `Recipe:` trailer names the recipe of the block that leaves a finding category | `channel-alpha.c`'s `SetImageAlphaChannel` (W10), six functions: Recipe T on the `OffIfOpaqueAlphaChannel` column scan alone 6.96 to 6.77 (*Deep, Nested Complexity* leaves, *Overall Code Complexity* arrives), Recipe E on the `AssociateAlphaChannel` column loop alone 6.38, both 7.00. No one recipe reaches both blocks: E refuses the scan's write of the shared `opaque`, and T has nothing to return for a loop that writes no status | mandate (2026-10-24) |
| 2026-10-30 | NEW | SV | Extract Sibling Blocks Differing in One Value: copies of one block in one function, the same text except for one literal or enumeration constant and not reported as *Code Duplication*, move to one `static` function taking the value as a `const` parameter of the type it is assigned to; each copy becomes the call with its own value | `channel-alpha.c`'s `SetImageAlphaChannel` (W10): the `DeactivateAlphaChannel`, `DiscreteAlphaChannel` and `OnAlphaChannel` cases differ only in the trait assigned. Recipe E on one: 7.56 to 7.58; one helper for the three: 7.62 (cc 66 to 63) | mandate (2026-10-24) |
| 2026-10-30 | AMEND | SV | Step 2: the copies may read different variables, the same token sequence under Recipe D's consistent renaming, each copy passing its own; the value may then also be a member access of a variable the block does not write (`image->columns`, `image->rows`), of one type in every copy, as in Recipe TW step 2. Step 3: its parameter has the member's declared type. *Careful* gives the argument | `shear-rotate.c`'s `IntegralRotateImage` (W10): its 90- and 270-degree tile loops clip the width (`width=tile_width;` and an `if` on `tile_x` and `image->columns`) and the height (`tile_height`, `tile_y`, `image->rows`) with the same text, four copies in one function, not reported as *Code Duplication*. SV refuses the renaming, TW wants two functions and five inputs, Recipe E on the first copy alone gives 7.55 to 7.56 and would need a second helper for the height. All four as one helper: 7.55 to 7.60 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | Q | *Use when*: in a sequential function the block may also be a running selection over an array (an argmax loop updating the best value and its index together), which folds the array into its written locals as a sum does; the steps are unchanged | `threshold-auto.c`'s `TriangleThreshold` (W10): its split-point search below the peak reads `a`, `b`, `c`, `inverse_ratio`, `histogram`, `max` and `start` and writes `threshold` and `max_distance`, refused by Recipe E step 4 (two written variables, `threshold` read after) and outside Recipe O's and Z's selection (a `switch` or `if` chain) and Q's per-pixel sum. A probe in Q's form: TriangleThreshold leaves Complex Method (cc 11), bumps 3 to 2, 8.11 to 8.36 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | P | *Leading operands of a chain*: the predicate may hold the leading operands of a chain of one operator (`a && b` of `a && b && c`, which C groups as `(a && b) && c`) when the whole chain would take five or more parameters; the site becomes `(Pred(...) != MagickFalse) && c` | `attribute-bounds.c`'s `GetImageBoundingBox` (W10): its *Complex Conditional* `(x < (ssize_t) bounding_box.width) && (y > (ssize_t) bounding_box.height) && (IsFuzzyEquivalencePixelInfo(&pixel,&target[3]) == MagickFalse)` reads five variables, so the whole chain as one predicate carries *Excess Number of Function Arguments*; its first two operands as a three-parameter predicate: 7.51 to 7.66, the file's sixth function | mandate (2026-10-24) |
| 2026-10-30 | AMEND | E | Step 4: a variable declared without an initialiser, which the block writes before it reads it on every path and which nothing after the block reads, is declared in the new function even when the caller writes it before the block (as Recipe TW step 3 and Recipe J step 3 say for their blocks); the 2026-10-28 condition stays for a variable with an initialiser | `decorate.c`'s `FrameImage` (W10): its bevel loops share the function's `y` (and the top band's `width`) with the loops before them; each loop sets `y` in its `for` before any read, and the next loop sets it again. The 2026-10-28 wording ("when no statement between that declaration and the block writes it") is there to keep an initialiser's value, which such a variable does not have | mandate (2026-10-24) |
| 2026-10-30 | AMEND | SV | Step 2: the value may also be an object-like macro of the file expanding to a call, with constant arguments, of a `static inline` function that reads and writes nothing but its parameters (`AccentuateModulate`, `ScaleCharToQuantum(80)`), its parameter of the function's return type; and a copy's one written variable, assigned whole before it is read and read by the caller afterwards, becomes the new function's local and return value (`v=Helper(...);`, Recipe E steps 3 and 4). *Careful* gives the argument | `decorate.c`'s `FrameImage` (W10): its four 3-D colours are two pairs of copies, five statements each on `matte`, differing only in the modulate macro (`AccentuateModulate`/`HighlightModulate`, `ShadowModulate`/`TroughModulate`), each copy writing its own colour. The macro is not a literal, an enumeration constant or a member access, so step 2 refused it. The lighter pair as one helper: 8.08 to 8.20; both pairs: 8.24 | mandate (2026-10-24) |
| 2026-10-30 | NEW | CN | Combine Nested Conditions: an `if` whose whole body is a second `if`, neither with an `else`, becomes one `if` whose condition is the two joined by `&&` in their order; proven by `tools/obj_identical.sh` printing `IDENTICAL` | `delegate.c`'s `GetDelegateInfo` (campaign B, task M115): three such match tests, a probe gave bumps 6 to 3 and 3.26 to 3.32 with identical object code; the same shape in `IsExecutableToken` and `InvokeDelegate`. No recipe removes a nesting level that carries no statement: G and P keep both tests, E-family helpers inherit the nesting | mandate (2026-10-24) |
| 2026-10-30 | AMEND | E | Step 4: a variable the block writes before reading it on every path, and that nothing after the block reads, is declared in the new function with its declaration and initialiser even when the caller wrote it before the block, as Recipe Y step 2, Q step 2 and J step 3 already allow for their blocks; a struct filled by a call counts as written where the block reads it only after the call succeeded | `delegate.c`'s `CopyDelegateFile` (campaign B, task M115): its buffer-size block reads `attributes` only after `fstat(source_file,&attributes) == 0`, but `IsKeptDestination` fills `attributes` earlier, which the founding wording of step 4 refuses although the block never sees that value. The block is one decision short of taking the function out of *Complex Method* (cc 9) | mandate (2026-10-24) |
| 2026-10-30 | NEW | OC | Extract Block Ending the Iteration with Out-Parameters: a block followed directly by `continue;` of its loop, which leaves early through that `continue;` and writes the loop's cursors, moves to a `static void` function: each `continue;` becomes `return;`, written variables go by pointer as in Recipe O, and the `continue;` after the block stays after the call | `delegate.c`'s `InterpretDelegateProperties` (campaign B, task M115): the backslash case's inner `switch` advances `p` and `q` and continues the scan loop from every case; O refuses its `continue`s, E step 7 its two written cursors and its position, LB would need a struct of out-pointers and a status that always leads to the same `continue;`. Probe: nesting 4 gone, 3.83 to 4.05 | mandate (2026-10-24) |
| 2026-10-30 | AMEND | V | Step 7: `return(v);` after the shared cleanup `S` may be `return(e);` of a side-effect-free expression (no call, assignment, increment or decrement), the same text at every exit (checked by script), reading no variable the block writes; Recipe I step 8 follows. *Careful* gives the argument | `xml-tree.c`'s `NewXMLTree` (campaign B, task M115; the owner left the ruling to the session, 2026-10-30): about twelve exits end in `utf8=DestroyString(utf8); return(&root->root);`, which is neither a local, a constant nor a call | owner |
| 2026-10-30 | AMEND | O | Step 2: a parameter of pointer type (declared `T *p`) whose own value the block assigns counts as a plain local, passed as `T **p` and written `(*p)`; writes through it stay writes through it. Recipes V, W, LB, I and OC read "plain local" the same way (V step 2 says so). *Careful* gives the argument | `xml-tree.c`'s `ParseEntities` and `ParseInternalDoctype` (campaign B, task M115; the owner left the ruling to the session, 2026-10-30): their blocks advance the `xml` parameter itself, which step 2 refused although the pointer object is the function's own (C11 6.9.1p9) | owner |
| 2026-10-30 | NEW | NS | Name a Constant Selection: a recurring `(c ? k1 : k2)` that picks one of two literals of one type, with a side-effect-free `c`, becomes a call of a `static inline` function returning that type; all sites in one function per commit; proven by `tools/obj_identical.sh` printing `IDENTICAL` | `annotate.c`'s `AnnotateImage` (campaign B, task M115): eighteen gravity signs such as `(geometry.width == 0 ? -1.0 : 1.0)` in one `switch`, cc 56 to 38 in a probe with identical object code; P names only boolean conditions, and SD's helpers would inherit the decisions (a three-case gravity helper would be a *Complex Method*) | mandate (2026-10-24) |
| 2026-10-31 | AMEND | TW | Step 2: a `const` struct local of pointers that the block passes only as `&s` to a pointer-to-`const` parameter of a callee writing through its members, and neither assigns nor writes a member of, goes by value as a parameter of its exact type, its text unchanged (N step 2 (d) keeps it out of the struct). *Careful* gives the argument | `compare.c`'s `GetDPCSimilarity` and `GetNCCSimilarity` (W10): their column loops are the same text (eight inputs, depth 3, a bump each), passing the row's `CorrelationSums` as `&sums`; both as one helper, then their identical statistics blocks (Recipe I and I step 7), 2.43 to 2.54 in a probe | mandate (2026-10-24) |
| 2026-10-31 | NEW | AR | Extract Block Through a Struct of Array Pointers: a block outside OpenMP that reads or updates, element by element, several local arrays of the caller (each used only as `v[e]` or passed whole to a pointer parameter) moves to a `static` function taking one `const` struct of pointers to the arrays' first elements, initialised once in the caller (`const Name name = { v1, ... };`) and bound `T *const v = name->v;`, the block's text unchanged; other inputs and at most one returned variable as in Recipe E, or an input struct as in Recipe N | `statistic-moments.c`'s `GetImageMoments` (W10): the second pixel pass writes eight moment arrays per pixel (Q step 8 admits six), the composite average and the normalisation write eleven, the ellipse and Hu loops read four and seven; E, N, K and Q refuse them. A probe with this recipe, E and Q: 8.05 to 10.0 | mandate (2026-10-24) |
| 2026-10-31 | AMEND | N | Step 2: when the caller binds `const` locals from its own `const Name *name` parameter (a helper of N, Y or LB's struct form) and the whole group is such locals, the new function may take that `name` unchanged instead of a new struct: no typedef, no local, no assignment; it binds the members its block reads. *Careful* gives the argument | `quantize-posterize.c`'s `PopulatePosterizeMap` (W10): its column loop reads `image`, `map_image`, `levels` and `number_columns`, all bound from its `PosterizeMapInfo`, plus `scale` and `q`. Step 3's letter would add a second struct of the same four members and refill it from the locals just bound from the first. With the column loop as a helper the file has twelve functions and its mean cyclomatic complexity falls under 4: *Overall Code Complexity* leaves, 8.89 to 9.47 in a probe | mandate (2026-10-24) |
| 2026-10-31 | AMEND | E | Step 4: a variable the block writes before reading on every path, but only through a test a compiler cannot follow (a first-pass `(i == 0)` that short-circuits the comparison reading it), so that declaring it in the new function draws a may-be-uninitialised warning, goes by value when the caller sets it before the block and nothing after the block reads it, as the read-on-entry form already allows | `attribute-hull.c`'s `GetImageMinimumBoundingBox` (W10): its edge loop sets `distance` on the first pass and compares against it on the others; declared in the helper as step 4's letter says, clang reports `-Wconditional-uninitialized` and MSVC C4701 (an error in the Windows jobs, the reason for commit 151be9bde). The loop with `MoveBoxToOrigin` as `FindNearestEdge`: 9.31 to 9.42 in a probe | mandate (2026-10-24) |
| 2026-10-31 | AMEND | N | Step 2's pass-through form: the caller may also be a function whose parameters Recipe B bundled into `const Name *name`, binding a `const` local of each old name from its member (B step 3). Through Recipe Q step 3, which builds its struct as N step 2 does, the form serves Q too. *Careful* gives the argument | `threshold-adaptive.c`'s `AdaptiveThresholdPixel` (W10): after Recipe B (11 arguments to 4) its sliding-window update (cc 9) reads `image`, `width`, `height`, `channel_bias` and `channel_sum`, all bound from its `AdaptiveThresholdPixelInfo`, plus `p`, `i` and `channel`. The letter would add a second struct of those five members, filled from the locals just bound from the first. The update as a helper: 8.88 to 9.11 in a probe | mandate (2026-10-24) |
| 2026-10-31 | AMEND | rule 1 | A set whose blocks lie in several functions of the file may take different recipes when *Overall Code Complexity* is what lowers each block alone and no smaller set rule 1 already admits raises the score; the message names each block with its function, recipe and score alone, and each smaller set's score | `layer-merge.c`'s `MergeImageLayers` (W10), six functions at 9.63 (its one finding Complex Method, cc 13): Recipe Z on the method `switch` alone 9.38, Recipe E on the virtual-canvas block alone 9.09, Recipe E on the loop bodies of `ExtendToLayerBounds` and `ExtendToMosaicBounds` alone 9.04 each; every pair and every set of three 9.04 to 9.38 (the blocks of `MergeImageLayers` together 9.38, the two loop bodies together 9.04); all four 10.00 (mean cc under 4, no finding). The two loop bodies are what bring the mean under the threshold, and they are not blocks of `MergeImageLayers` | mandate (2026-10-24) |
| 2026-10-31 | AMEND | E | Step 7, `switch` form: when the block writes `v` before it reads it on every path, the new function declares `v` without an initialiser instead of taking it by value (step 4's rule for a variable written before it is read); the block still becomes `v=Helper(...);` and each `break;` `return(v);` | `gem.c`'s `GenerateDifferentialNoise` (W10): its Laplacian case leaves the `switch` through `break;` on its `alpha <= 0.5` branch and sets `noise` on every path, but the caller never sets `noise` before the `switch`, so taking it by value would read an uninitialised variable. With the Laplacian, impulse, multiplicative-Gaussian and Poisson cases as helpers and `GetOptimalKernelWidth2D`'s normalisation loop (rule 1's set), *Overall Code Complexity* leaves and Bumpy Road Ahead and Large Method go: 9.21 to 9.68 in a probe | mandate (2026-10-24) |
| 2026-10-31 | AMEND | V, Y | V step 7: the cleanup run `S` may be followed by a `return(<expr>);` of the step's last form in place of `return(v);`, both left in the caller. Y step 1: the block may leave only through exits of one of V step 7's forms, written in the caller as V step 7 writes them (step 6); Y step 4: after `v`, the pointers the block writes through that the struct leaves out (Recipe N step 2), at most four parameters. *Careful* gives the argument | `resize-image.c`'s `ResizeImage` (W10, cc 12, 88 lines): the filter-image clone and the two filter passes read eight variables and write `status` and the temporaries `filter_image`, `offset` and `span`; the one exit is `resize_filter=DestroyResizeFilter(resize_filter); return(DestroyImage(resize_image));`, which V step 7 names only as `S return(v);` or a lone `return(<expr>);`, and Recipe Y admits only `goto L;` or `return(c);`. A probe: `ResizeImage` leaves *Large Method* (cc 12 to 10), 6.46 to 6.59 | mandate (2026-10-24) |
| 2026-10-31 | AMEND | B | Step 4: the address of a plain local, `(&v)`, may fill a pointer member; writes to `v` between the fill and a call do not count against step 5. Step 6: a struct the function reaches only through its `const Name *` parameter, filled by the caller before the call and written by nothing until it returns, counts as `const`; a member whose target the region writes is bound to a `const` local at the top of the body (step 3) and the region uses the local. *Careful* gives the argument | `resize-image.c`'s `HorizontalFilter` and `VerticalFilter` (W10, 7 arguments each): `resize_filter`, the scale factor, `span` and `&offset` are the same at every call in `ApplyResizeFilterPass`'s branch. `&offset` is neither an identifier nor arithmetic (step 4's letter), and the struct, filled member by member after `span` is computed, cannot be a `const` local (step 6's letter), though the filters read `resize_filter`, the factor and `span` in their parallel loops. Binding every member to a local instead grows each filter (102 to 108 lines) while it keeps *Large Method*. A probe: `HorizontalFilter` leaves *Excess Number of Function Arguments*, 6.63 to 6.83 | mandate (2026-10-24) |
| 2026-10-31 | AMEND | E | Step 3: a `const` struct local of the caller, initialised at its declaration, which the block neither assigns nor writes a member of and whose address it takes only as `&s` passed to a pointer-to-`const` parameter of a callee (which reads the members and may write through its pointer members), goes by value as a parameter of its exact type, the block's `&s` unchanged: Recipe TW step 2's form, for one block | `layer.c`'s `CompareImagesBounds` (W10): after Recipe B bundled `FindDifferingPixel`'s arguments into the `const DifferingPixelScan scan` (with `&alpha_pixel` and `&beta_pixel` as members), the first column scan passes `&scan`; a probe moving that scan loop to a function taking `scan` by value, 6.57 to 6.62, `CompareImagesBounds` leaving Large Method (75 lines), bumps 4 to 3, cc 15 to 12 | mandate (2026-10-24) |
| 2026-10-31 | AMEND | Y | Step 4: a variable the block reads on entry and then writes, and that the caller does not read after the block before writing it again, goes by value right after the struct, its text unchanged (Recipe E step 4's by-value input, as Recipe TW step 2 has it); it is neither a member nor `v` | `layer.c`'s `CompareImagesLayers` (W10): its first frame loop, `i=0; next=GetNextImageInList(next); for ( ; next != ...; ...) { ... }`, advances `next`, which the caller sets again (`next=GetFirstImageInList(image);`) before any later read, and leaves through V step 7's `image_a=DestroyImage(image_a); bounds=... RelinquishMagickMemory(bounds); return((Image *) NULL);`. A probe with `next` by value: 6.62 to 6.70, `CompareImagesLayers` leaving Bumpy Road Ahead | mandate (2026-10-24) |
| 2026-10-31 | NEW | RF | Reuse a Struct Fill Across Calls: a fill of a struct local that repeats an earlier fill of the same members from the same side-effect-free expressions, with nothing in between that writes the local or what the expressions read, is deleted; the later calls read the earlier fill | `paint.c`'s `FloodfillPaintImage` (W10): three LB steps left `FloodfillRow row` filled with the same eight assignments before each of `FillFloodplaneLeft`, `FillFloodplaneRun` and `SkipFloodplaneRun` in one iteration; a probe deleting the second and third fills, 6.19 to 6.24 (172 to 156 lines), object code `IDENTICAL` | mandate (2026-10-24) |
| 2026-10-31 | NEW | LT | Extract Loop Whose Early Exit Is Tested After It: a `for` loop leaving itself through `break;`, followed directly by `if (<cond>) X;` testing its own condition, moves to a `static MagickBooleanType` function whose `break;`s become `return(MagickFalse);` and which drops the test; the caller writes `if (Helper(...) == MagickFalse) X;`. A run equal to an existing I/LB/W/Y function calls it | `histogram.c`'s `CheckImageColors` (W10): its tree descent (`if (level < MaxTreeDepth) break;`) is `ClassifyImageColors`' `DescendHCube` with a `break` for the `return`, and its column loop is tested `x < (ssize_t) image->columns` after it; probes 8.58 to 8.64 (descent), 9.68 with the column loop, the function leaving every finding | mandate (2026-10-24) |
| 2026-10-31 | AMEND | SD | Step 7: a `switch` without `default` may be split too; the new function's `switch` ends with `default: break;` and the caller's gains the `default:` group holding the call. *Careful* argues the values that match no label and names the cost (`-Wswitch` no longer sees a constant added later) | `resample.c`'s `ResamplePixelColor` (W10): its `hit` switch over every `VirtualPixelMethod` constant has no `default`, so step 1 refuses it; without the added group the new function's partial `switch` draws `-Wswitch`. Probes: the vertical-tile and dither groups, 7.30 to 7.34; then the corner and horizontal-tile groups, 7.37 (cc 34 to 26) | mandate (2026-10-24) |
| 2026-10-31 | AMEND | V | Step 6: in a function returning `void`, `return;` plays the part of `return(c);`: each becomes `return(MagickFalse);` and the caller writes `if (Helper(...) == MagickFalse) return;`. Applies to Recipe Y through its step 1 | `resample.c`'s `ScaleResampleFilter` (W10, public, `void`, *Large Method* of 100 lines, most of them in uncompiled `#if` branches): its compiled ellipse-fitting block leaves through `return;` once the ellipse is too large and writes `F`, which the caller reads after; Recipe Y (inputs `resample_filter`, `A`, `B`, `C`) refuses it only because `return;` has no constant. A probe: 7.40 to 7.41 | mandate (2026-10-24) |
| 2026-10-31 | AMEND | SV | Step 2: a copy whose one write is `v=c;` of one constant under its test (a path leaves `v` alone), `v` set by the caller on every path to the copy and read after it, takes `v` by value as its last parameter and returns it, the copy becoming `v=Helper(...,v);` (Recipe E step 7's `switch` form); a member-access value may reach the member through an element of an array member (`image->colormap[i].red`) that the block does not write. *Careful* gives the argument | `attribute-depth.c`'s `RaiseColormapEntryDepth` (W10, cc 11, Bumpy Road 3): its red, green and blue tests differ in two values (the trait accessor and the colormap member), and the first lacks the `atDepth != MagickFalse` operand, so SV refuses them whole; their inner `if`s differ only in the member (`.red`, `.green`, `.blue`), but each writes `atDepth=MagickFalse;` only when its test holds, which step 2's returned-variable form (assigned whole first) refuses. Recipe E on the red copy alone: 7.03 to 7.05; the three as one helper: 7.03 to 7.30, the function leaving *Complex Method* (cc 8) and *Bumpy Road Ahead* | mandate (2026-10-24) |
| 2026-10-31 | AMEND | N | Step 8, *Sibling blocks*: the cases of one `switch`, each moved under this recipe in its own commit, may share one `Name` (the union of their groups; each function reads only its own members) and one local filled once, where nothing on any path from the fill to a call writes a member's variable (Recipe B step 5's condition) | `pixel.c`'s `InterpolatePixelChannels` (campaign B, task M116): eight cases read the same eight inputs; a fill before each call adds nine lines per case to a function the step is meant to shrink | mandate (2026-10-24) |
| 2026-10-31 | AMEND | T | Step 2: with the other name, the body may also read `v` after the block in the same iteration (the test that ends an inner loop the block sits in); *Careful* gives the argument: the caller's `v=c;` lands as the call returns, before that read | `image.c`'s `IsHighDynamicRangeImage` (campaign B, task M116): its channel loop sets `hdri=MagickTrue;` and breaks, then the column loop breaks on `hdri`, which step 2 refused; it holds the file's only *Deep, Nested Complexity*, 2.77 to 2.90 in a probe | mandate (2026-10-24) |
| 2026-10-31 | AMEND | OC | Step 1: a block that is the tail of `Lp`'s body counts as followed by `continue;` (the end of the body is where `continue;` jumps); the call is then the body's last statement. *Careful* gives the argument | `image.c`'s `AppendImages` (campaign B, task M116): its scan loop ends in the stacked/side-by-side extent update, which `continue`s on one branch and falls off the end on the other, writing `width` and `height`; O refuses its `continue`, E step 7's tail form its two outputs | mandate (2026-10-24) |
| 2026-10-31 | NEW | VC | Extract Loop-Tail Block Returning Success for a Cleanup Exit: a block at the tail of a loop body (or followed by its `continue;`) whose exits are `continue;` of that loop and one cleanup return of the function in a form Recipe V step 6 or 7 or Recipe I step 8 admits moves to a `static MagickBooleanType` function: each `continue;` becomes `return(MagickTrue);`, each exit `return(MagickFalse);`, the caller keeps the exit's text | `xml-tree.c`'s `ParseInternalDoctype` (campaign B, task M116): its `<!ATTLIST` branch and the attribute loop's body mix `continue;` with `predefined_entities=...RelinquishMagickMemory(...); return(MagickFalse);`, which V, Y, I and W (no `continue`) and OC (no `return`) refuse; they hold the function's nesting of 5; a probe: 4.05 to 4.21 | mandate (2026-10-24) |
