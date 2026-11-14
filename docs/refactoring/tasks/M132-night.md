# M132 - campaign B, night of 2026-11-14, third batch

Stacked on M131 (#111). Scores come from cs-mcp 1.5.16.

Campaign B: **mean 8.77 over 94 files -> 8.92 over 95 files; files at 10.0: 36 -> 38**.

## What is in it
- **Lane c, round 7: property.c 3.21 -> 3.62.**
  - InterpretImageProperties: the buffer macros became functions behind one-call wrapper macros
    (new Recipe MC). The glob loops merged through a selector (MT). Braced escapes and the '@' file
    prologue moved to helpers.
  - SetImageProperty: the option-parsing arms became a keyed table of setters (new Recipe KM).
  - GetMagickProperty: two groups merged, papersize extracted.
- **Lane a, round 6:**
  - image-mask.c 8.81 -> 10.0 and image-append.c 8.54 -> 10.0. Recipe MX merges the horizontal
    and vertical smush chains through a side selector. The main session read it: the vertical
    side's swapped coordinates match the original, and 17 cases cover both axes.
  - image-clone.c 9.09 -> 9.68, log.c 7.93 -> 8.81, type.c 8.22 -> 8.81, policy.c 8.18 -> 8.55.
  - utility.c 6.64 -> 7.53, with the new split utility-unique.c (9.92).
  - New Recipes NC, SK, SA, MX and HF; amendment to LF.
- **Lane b, round 8:**
  - **xml-tree.c 5.34 -> 9.02.** Folding twin functions (new Recipes FG and FW) took the file
    under the function-count threshold. After that, Code Duplication, Excess Arguments, Complex
    Conditional, Large Method, Complex Method and Overall Code Complexity all cleared.
  - **profile.c 5.15 -> 7.29.** Recipe G turned ProfileImage's non-ICC branch into an early
    return, with object code IDENTICAL; that cleared Brain Method and Deep Nesting.
  - profile-sync.c 8.81 -> 9.38 (it includes lane a's 0e7db3f93, cherry-picked).
  - annotate.c 5.22 -> 5.69, where it plateaus: the RAQM code is not compiled, and the file sits
    at 113 functions.

## Gate on the pushed tip (baseline c980213164)
- Whole catalogue: 15,961 cases, 6 diverged.
  - Three are the known ones (info/d333f9b818, gaps/2857e5295a, gaps/c5651ffad8).
  - resize/6ee45e9cfa (load-sensitive) passes alone with `--no-cache`.
  - The two hough matrix-on-disk cases are baseline load timeouts. By hand, base and candidate
    give byte-identical output.
- `cxx_warnings --all`: OK on 191 files. `ci_warnings`: OK. The no-delegate clang build:
  0 warnings.

## For the owner
- **Owner question: may a Recipe S split widen a `static` function to `MagickPrivate`**
  (internal, not exported)? quantum-export.c (198 functions; its only public function is
  ExportQuantumPixels) can split no other way. It would also unblock constitute-codec.c (shared
  IsCoderAuthorized) and utility.c (shared IsPathDirectory).
- **Code that moved or changed shape:**
  - InterpretImageProperties' buffer macros are now functions behind one-call macros (MC).
  - KM puts SetImageProperty's table lookup ahead of its switch. Only failing key comparisons
    run before the arms; checked by reading.
  - MT writes two void reset calls as one `?:` statement.
  - VC step 7 collapses `if (G()==MagickFalse) return(MagickFalse); return(MagickTrue);` to
    `return(G())`.
- **xml-tree.c:** IsUnclosedComment keeps the side effect `*p+=2` inside the predicate, copied
  character for character. The W and AT helpers bind the caller's existing input structs.
- **OpenMP:** image-mask.c's SetImageMaskRow extraction touches a parallel row; the
  shared/private reasoning is quoted in its commit.
- **Weak reach:**
  - Reached by no case: `%[papersize:...]`, AppendRootInstructions (two guard-level E steps),
    and the font substitution loop in GetTypeInfoByFamily (not confirmed reached).
  - Reached by 1 case: RemoveRegionMaskChannel.
- **Error and trace names:** they now name helpers in property, utility (Base64Decode,
  ExpandFilenames with its fatal throw, ListFiles, GetPathComponent) and policy (rulings of
  2026-11-12 and 2026-11-13).
