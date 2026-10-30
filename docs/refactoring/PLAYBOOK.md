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
   step, and the message names each block with the score it gave alone.
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
whose proof is an identical object file, not the guard.

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
   bump. Do not invent an out-parameter struct. A variable that nothing after the block
   reads does not count (a block that runs to the end of the function, step 7, advancing
   the row pointers `p` and `q`): the new function declares it, with the initialiser it
   had, when no statement between that declaration and the block writes it.
   One the block reads on entry and then writes, and that nothing after the block
   reads (`AdaptiveBlurImage`'s `w`, counted down to release the kernels allocated so
   far, just before the caller throws), is an input like any other: it goes by value,
   its text unchanged, and its writes land in the new function's copy, which nothing
   reads once the function returns. Check by reading every path out of the block.
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
   A parameter the caller receives by value, of a struct or arithmetic type (not a
   pointer, and not an array parameter, which is one), counts as a plain local
   (`ApplySpliceGravity`'s `RectangleInfo splice_geometry`): it is the caller's own
   copy, and the same rule on its address applies.
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
- Under step 2's exception the other call receives `&v` at a point outside the block,
  before or after the new function runs, and keeps no copy of it once it returns; so
  when the new function runs, its out-parameter is the only pointer to `v`, and no two
  of its pointers name one object. A callee that stores the address (in a struct, a
  global, a cache) gives `v` a second name: the variable does not qualify.
- A parameter received by value is an object of the function's own, initialised from
  the argument when the call starts and gone when it returns (C11 6.9.1p9); a write to
  it, directly or through `&v`, reaches no object of its caller. A parameter of
  pointer type is refused: what it points to is the caller's, and writing through
  it is not a write to a local. An array parameter is a pointer.
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
   reads only in its leading `if (v == c) continue;`, is the loop's status. The new
   function is step 5's, `status=MagickFalse;` standing where the block wrote `v=c;`,
   and the caller writes `if (Helper(...) == MagickFalse) v=c;` (step 7).
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
  with `status`. A variable the loop also writes another value (`IdentifyImageGray`'s
  `type=GrayscaleType;`) or reads elsewhere in the body is not a status: skip it.
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
the flag the function returns in place of `status`.

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
   that runs the block, so it is reset as often as before.
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
- Recipe E's limit on what the block writes stands: one value returned, or writes
  through a pointer the block already used for pixel data. A block writing more is
  skipped, as in Recipe E.
- In the member form of step 4 each `name->member` reads the same object the `const`
  local would have been initialised from, at each use instead of once; the struct is
  the caller's local, filled before the call and written by nothing in the block, so
  every read gives the value the block's own read of the variable gave. `->` binds
  tighter than a cast or an arithmetic operator, so `(double) name->i-x` groups as
  `(double) i-x` did.
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

---

## Recipe Q - Extract Accumulating Block Taking an Input Struct

**Use when:** CodeScene reports *Deep, Nested Complexity*, *Bumpy Road Ahead*, *Brain
Method*, *Complex Method* or *Large Method*, and the block that would fix it is a
per-pixel accumulation inside a parallel loop body (or in a sequential function, step 1)
that meets Recipe K in every respect but one: with its inputs as parameters, the new
function would take five or more arguments (*Excess Number of Function Arguments*, which
the pre-commit gate refuses on a new function). The typical case is a filter's per-channel weighted sum (`pixel`, `gamma`
and a cursor), reading the window pointer, the window size, the weights and the channel.

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
the struct `error` and the cursor `node_info`.

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
   parameter, a global or a `static`) whose address is not taken elsewhere. Writes
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
   values, is not this recipe, except under step 7.
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
   through `return(c);` of one constant `c` (Recipe V step 6); it writes at most one
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
   caller, taking `const Name *name` first and then `v` as a pointer. At the top of its
   body declare one `const` local per member, as Recipe N step 4 does (or write
   `name->member` at every use, its member form), then the variables of step 2. In the
   text write `*v` for every use of `v`, reads included (Recipe V step 4); each `goto L;`
   or `return(c);` becomes `return(MagickFalse);`, one for one; the function ends with
   `return(MagickTrue);`. Nothing else in the text changes.
5. In the caller, declare the `Name` local among the declarations at the top of the block
   that holds the call. Directly before the call, assign every member from the variable
   of its name, in declaration order, with nothing between the assignments and the call.
6. Where the block stood, write `if (Helper(&name,&v) == MagickFalse) goto L;` (or
   `return(c);`), and check every member and argument by reading.

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
   `continue` or `break` leaves it. Recipe V step 7's cleanup form is not this recipe,
   except as step 8 admits it.
2. List the variables of the caller the block writes: two to four, each a plain local of
   the caller or a by-value parameter of it (`FunctionE fe`), none a global, a `static`
   or an array, and none whose address `&v` the caller takes anywhere (check by
   `grep`), except to fill a member of another helper's targets struct under this
   recipe, filled directly before that helper's call and read by nothing else (Recipe
   LB step 2's "helpers of this recipe"). Writes through pointers the block already uses stay as they are; variables
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
   `return(c);`), and check every argument, member and address by reading.
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
   variable.

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
  local, filled directly before its own call and read by nothing else, so its copy of
  `&v` is used only while that helper runs, which is never while this one runs (C calls
  do not overlap here: no OpenMP region, step 1). While this helper runs, its member is
  the only pointer to `v` in use, and no two of its own members name one object.
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
   5). Check every member and argument by reading.
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

**Evidence level:** oracle, with `refactor_guard.py --calls` (`+2` for the new function,
nothing else moved), a case for each moved label where one exists, and steps 1 and 5
checked by reading.

**Provenance:** 2026-10-30, from `fourier.c`'s `ComplexImages`, whose operator `switch`
(seven operators and `default`) sits at depth 4 inside the parallel row loop. Recipe Z on
the whole `switch` gives a helper of cc 9, a *Complex Method* the gate refuses (5.09 in a
probe); the unary group (conjugate with `default`, magnitude-phase, real-imaginary) as its
own function, then Z on the binary rest, leaves neither helper with a finding: 4.83 to
4.85 to 5.21 in a probe. Added by Claude Code (Claude Opus 5.5) during task W09, under the
mandate of 2026-10-24.

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

## Known plateaus

None recorded yet. When a file stops improving and no recipe in this catalogue applies,
and you do not add one, record it here: the file, its score, and the smells left.

| File | Score | Smells left, and why no recipe reaches them | Recorded |
| --- | ---: | --- | --- |
| `MagickCore/resize.c` | 2.89 | Brain Method and Deep Nesting (`InterpolativeResizeImage`'s channel loop assigns and tests the shared `status` inside `omp parallel for`, which even Recipe T refuses; `ScaleImage`'s bookkeeping writes six variables on survivor lines); Bumpy Road, Complex and Large Method (OpenMP-bound: the filters `continue` the parallel loop on failure; survivor lines; LQR not compiled); Excess Arguments (public signatures; the filter kernels' context is computed per iteration, and bundling it costs the filters more than the kernels gain). 2.45 before Recipe S, 2.87 before Recipe T on `SampleImage`. See [M01's report](tasks/M01-resize.md) | 2026-10-24, Mac |
| `MagickCore/magnify.c` | 6.57 | Complex and Large Method: `Hq2XHelper` is a rule table written as a `switch` (splitting its cases yields clones with 7-8 arguments, 5.16 in a probe); `Fish2XBottomRightPixel`'s ladder shares six flags that only a struct declared after statements could carry; the 3x3 kernels and `IsEpbx2XBlendable` split into clones of each other (5.93 in a probe). Bumpy Road and Large Method in `MagnifyImage`: its last bumps are the parallel loop's `continue` and the atomic progress block. Overall Code Complexity: the mean of the above. From 6.21, with Recipes E, T, M and B. See [M01's report](tasks/M01-resize.md) | 2026-10-24, Mac |
| `MagickCore/compare.c` | 2.43 | FFTW is compiled now (`--with-fftw`, W08), so the `SIM*` region is measured and reached. Deep Nesting stays in `SimilarityImage` (its `omp critical` best-match `switch` nests two `if`s at depth 4 and cannot move; extracting its channel loop, metric `switch` and best-match `switch` (L then Z) left it flat, and the frequency-domain dispatch returns a variable, which no recipe takes) and in `SIMFilterImageNaNs` (no case; its channel or row loop as a helper joins the `SIM*` shell duplication). Brain Method in `SimilarityImage` (seven public arguments, depth 4). Complex Method in the `SIM*` row shells (cc 10 from the parallel loop's `status`/`continue` paths: moving the row tail needs a `continue` Recipe T does not take, and was flat on `SIMSquareImage`) and in `MSESimilarityImage`/`NCCSimilarityImage` (cleanup-throw macros whose blocks write what the cleanup reads, refusing V step 7). Code Duplication: the row shells, the `Accumulate*Pixel` helpers and the `GetPSNR`/`GetRMSE`/`GetDSSIM` wrappers differ in more than one value. From 2.43 on, steps that took a function out of a category (`AccumulateCorrelationPixel`, Excess Arguments; `CompareImages`, Bumpy Road) and probes that did (`SIMSquareImage`, Complex Method; the trait test as `IsChannelSkipped` at 33 sites) left the score flat. See [W08's report](tasks/W08-night-session.md) and M02's | 2026-10-30, WSL (W09) |
| `MagickCore/colorspace.c` | 3.15 | Bumpy Road (10 each) and Deep Nesting (4) in `sRGBTransformImage` and `TransformsRGBImage`: every row loop's `if (status == MagickFalse) continue;`, `if (q == NULL) { status=MagickFalse; continue; }` and `if (sync == MagickFalse)` stay in the parallel body (T and U refuse a block that reads `status` or `continue`s the parallel loop), the depth-4 site is the `omp atomic` progress block, and each `switch` case holds its own `#pragma omp parallel for`. Probes: removing those three `if`s in every row loop would take `sRGBTransformImage` from 10 bumps to 3 (3.12), but moving the column loops alone is flat (M06) and as helpers adds *Code Duplication* (2.80); the property parsing (`white-luminance`, `film-gamma`, `reference-black`, `reference-white`, 8 sites) as one helper is flat; removing 140 lines is flat. Complex and Large Method and Excess Arguments in the public `ConvertGenericToRGB`/`ConvertRGBToGeneric` (Recipe SD chains to bring each piece under cc 9 give five functions per side, reported as *Code Duplication*: 2.83); Excess Arguments in the public `ConvertHSLToRGB`/`ConvertRGBToHSL`. From 2.80 with D, V, E, O, N (W08), then D (W09, `SetImageGray`/`SetImageMonochrome`, leaving *Code Duplication*). See [W08's report](tasks/W08-night-session.md) and [M06's](tasks/M06-colorspace.md) | 2026-10-30, WSL (W09) |
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
