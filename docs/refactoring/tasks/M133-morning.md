# M133 - campaign B, morning of 2026-11-14

Stacked on M132 (#112). Scores come from cs-mcp 1.5.16.

Campaign B: **mean 8.92 -> 8.96 over 95 files; files at 10.0: 38 -> 40** (coder.c, exception.c).

## What is in it
- **Lane c, round 8: draw.c 2.99 -> 3.27.**
  - RenderMVGContent's primitive tail moved out in CS steps (new Recipe CS: extract a loop-body
    segment through a state struct of the caller's addresses).
  - Its keyword switch became a table of arm handlers (new Recipe KH): bumps 69 -> 6, and Deep
    Nesting gone.
  - Probes show 3.27 is draw.c's ceiling even with its five largest functions removed. What holds
    it is many medium-sized findings across its 212 functions.
- **Lane b, round 9:**
  - image-filename.c 8.81 -> 9.38, delegate.c 7.96 -> 8.81, delegate-invoke.c 7.64 -> 8.28,
    compress.c 7.70 -> 8.06, cipher.c 8.69 -> 8.81, locale.c 8.85 -> 8.91.
  - New Recipe CE: chain a block that ends in `continue` into an else. CodeScene counts every
    `continue`.
- **Lane b, round 10:**
  - coder.c 9.38 -> 10.0, exception.c 9.68 -> 10.0, configure.c 9.61 -> 9.68.
  - annotate.c 5.69 -> 5.81: two pairs of twins folded (new Recipes FC and FM) to free
    function-count room for RenderFreetype's glyph-load block.

## Gate on the pushed tip (baseline c980213164)
- Whole catalogue: 15,961 cases, 3 diverged, all known (info/d333f9b818, gaps/2857e5295a,
  gaps/c5651ffad8).
- `cxx_warnings --all`: OK on 191 files. `ci_warnings`: OK. The no-delegate clang build:
  0 warnings.

## For the owner
- **KH and `do { ... } while (0);`.** KH wraps arms that have inner exits in `do { ... } while (0);`.
  Lane c raised MSVC C4127 as a risk. Upstream already uses this idiom in MagickWand
  (magick-cli.c, operation.c), which the Windows jobs build with the same /W4 /WX, so it is
  expected to pass.
- **annotate.c Recipe F commit.** `obj_same_code.sh` printed DIFFERENT only because the Trace*
  functions are emitted in a different order. Compared per function, with branch targets masked,
  all 28 functions are identical. The 198 oracle cases also agree.
- **`tools/obj_modulo.sh` gave a false pass.** It printed MODULO-IDENTICAL on annotate.c while
  comparing zero functions. Lane b is fixing the tool and re-checking every commit that cites it as
  proof. The result comes in the next PR.
- **ExternalDelegateCommand:** `return(-1)` became `return(status)`. `status=(-1)` is set just
  before, so the value is the same; the main session read it.
- **Decisions needed:**
  - (1) May a split widen a `static` to `MagickPrivate`? This is the question from #112.
  - (2) May annotate.c's `-debug` AnnotateEvent log block move into a helper, so that the log
    names the helper? It is not a trace opening; worth about +0.02.
  - (3) geometry-meta.c needs a struct for its width/height pointers plus the stasis pair to get
    below five arguments. Is that the context struct ruled out on 2026-10-26?
- **Weak reach:**
  - Reached by no case: GetMnemonicProperty (guard only).
  - Reached by 1 to 5 cases: the compress.c helpers 5, GetCoderInfoList 1.
