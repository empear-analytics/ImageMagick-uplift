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

## Constitute family (2026-11-23/24)

After the policy.c work, the branch took the constitute family (S split of `WriteImage` into
`constitute-encode.c`, `92495ad7c`; `constitute-codec.c` to 10.0 by `a0c0ed5be`).

| File | Before | After | Steps |
| --- | --- | --- | --- |
| constitute-encode.c | 8.78 | 10.0 | E (`GetEncoderMagickInfo`), GL step 6 (`GetBimodalDelegateInfo`), E (`IdentifyVerboseWrite`), N (`EncodeWriteImage`) |
| constitute-pixels.c | 9.68 | 9.68 | plateau: *Excess Number of Function Arguments* on `ConstituteImage`, a `MagickExport` signature (6 arguments) |
| constitute-pixels-private.h | 9.5 | 9.5 | not taken: `SetImageTraitsFromMap` (cc 18) is reached by 1 case (`encode/47309673f6`) |

- Recipe GL gained step 6 (`c500c4bb7`): the inner test may be a same-file predicate whose `&&`
  chain starts with `v != NULL`, and `D` may assign a local declared at the caller's top.
- The `IdentifyVerboseWrite` step alone cleared *Overall Code Complexity* (8.89 to 9.48).
- The tail extraction left the GetBlobError `ThrowWriterException` in `WriteImage`: the macro
  writes `image` and names the function through `GetMagickModule()`.
- **Gap:** only one case reaches `ConstituteImage`, and no case reaches `IsDirectDelegate`
  (the `delegate:bimodal` path of `WriteImage`). Cases for `ConstituteImage` maps with alpha
  (`RGBA`, `O`), CMYK (`CMYK`), intensity (`I`) and a one-letter unknown map would put
  `SetImageTraitsFromMap` within reach of a label-set recipe (EM, LP or DG).

## Config loaders and oracle cases (2026-11-24, later)

| File | Before | After | Steps |
| --- | --- | --- | --- |
| type.c | 8.81 | 9.09 | OC (`ParseTypeConfigureToken`), Complex Method cleared |
| color.c | 8.76 | 9.09 | OC (`ParseColorConfigureToken`), Complex Method cleared |
| locale.c | 8.96 | 9.38 | S (`GetLocaleOptions` to `locale-options.c`, 10.0), then OC (`ParseLocaleConfigureToken`); Complex and Large Method cleared |

- **Recipe OC amended** (`ef98f2310`): step 4's input struct may be read as `name->member` and
  filled in Recipe Z step 5's FB form. The bound-local, member-by-member form crossed type.c's
  *Lines of Code in a Single File* (8.81 to 8.54); the member and FB forms score 9.09.
- **The XML loaders share one shape.** The loop-body tail after `if (*token == '\0') break;`
  (markup skip, include, element) is one OC helper: `continue;` becomes `return;`, and the
  written cursors (`q`, `status`, the current info node) go as pointers. Every `Load*Cache` in
  MagickCore is now below cc 9.
- **locale.c needed line budget first.** OC alone crossed the line limit (8.96 to 8.81). Each of
  three dry-run splits (`GetLocaleOptions`, `ListLocaleInfo`, the case-mapping group) left
  locale.c at 8.96 and let OC reach 9.38. The smallest was taken.
- **Oracle cases:** imdriver `constitute` and 21 driver cases (`edac7599b`) put 22 cases on
  `ConstituteImage`/`SetImageTraitsFromMap` (from 1).
- **Unreached:** `LoadLocaleIncludeElement` (no case includes a locale file), checked by
  reading in `749c3ac28`.

### Not taken (probed or read)
- **configure-paths.c `GetConfigurePaths`** (cc 18): the complexity is in
  `!MAGICKCORE_INSTALLED_SUPPORT` and Windows branches, which the oracle build does not compile.
- **deprecate.c:** both findings are in Windows-only HBITMAP functions.
- **resource.c** (Large Method on `RelinquishMagickResource`, at the line limit): every public
  root refuses a split (shared `LockResourceSemaphore`/`UnlockResourceSemaphore`,
  `FormatResourceSize`, and the file-scope `resource_info`, `resource_semaphore` and
  `temporary_resources`). Freeing budget needs the shared-header and accessor rulings.
- **pixel.c:** `IntensityFromSamples` is an always-inline per-pixel function, and
  `SortImagePixels`' bump is the progress block of an OpenMP loop; both need timing.
- **constitute-pixels-private.h `SetImageTraitsFromMap`:** taken later, see the next section.
- **CodeScene C thresholds** (cs-mcp `rules_config_list_thresholds`): *Lines of Code in a
  Single File* warns at 1000, *Overall Code Complexity* at a mean of 4, Complex Method at cc 9,
  Large Method at 70 lines, arguments above 4.

## Label folds and Overall Code Complexity (2026-11-24, evening)

| File | Before | After | Steps |
| --- | --- | --- | --- |
| constitute-pixels-private.h | 9.5 | 10.0 | FD (alpha), FD (CMYK), TB (`IsCMYKMapChannel`) |
| delegate.c | 8.81 | 9.38 | E (`FormatDelegateName`), E (`DestroyDelegateCommandList`) |
| matrix.c | 9.09 | 9.68 | DT (three ternaries), E (`SetMatrixSynchronize`), E (`RelinquishMatrixMemory`) |
| utility-unique.c | 9.92 | 10.0 | RV step 6 (f) (`LinkUniqueRelativePath`) |

- **Recipe FD added** (`731167b0b`), and TB's character form widened to its predicates
  (`397ca7c23`). FD folds a `switch` group that does work and breaks into the head of the
  `default:` group, behind an LP-style predicate of its labels. Peeling the groups after the
  `switch` (PL style, with `continue;`) brought *Bumpy Road* and scored lower (9.4, 9.29, 9.53).
  With the tests at the head of `default:`, every step rose: 9.5, 9.58, 9.68, 10.0.
- **Oracle cases:** five more `driver constitute` maps (`cd6bbf3f4`: `A`, `o`, `C`, `y`,
  `Ka`). A one-letter alpha map must stay sRGB, which tells a correct fold from one that lets
  the default group's one-letter gray test run. `SetImageTraitsFromMap` reach went from 22 to 27.
- **Overall Code Complexity, planned by stub probe.** Six cc-1 stubs cleared it in both files
  and gave the ceiling (delegate.c 9.38, matrix.c 9.68) before any step was taken. The finding
  cleared earlier than lizard's mean predicted: lizard counts both sides of every `#if`, and
  delegate.c cleared at a lizard mean of 4.02. matrix.c's three DT steps were object-identical.
  The Code Duplication finding M141 saw behind matrix.c's helpers did not appear with these two.

- **Recipe RV amended** (`dcffa06c6`): a block in a compiled `#if defined(M)` feature branch
  gets its helper under its own `#if defined(M)`; checked by deleting every `M` region from
  both versions and comparing.
- **Whole catalogue** at `66b97f518`, working-tree candidate: 16,005 cases, 1 diverged, the
  known `gaps/c5651ffad8` logcfg tie-break.
- **Disk:** an LLVM profile run of the 37 `AcquireUniqueSymbolicLink` cases (`oracle.py exec`
  with `LLVM_PROFILE_FILE`) filled the volume. Use `linecov.py` or a single case for line
  reach.

### Plateaus recorded
- **delegate.c (9.38):** *Excess Number of Function Arguments* on `ExternalDelegateCommand`
  (`MagickExport`, 5 arguments) and *String Heavy Function Arguments*.
- **matrix.c (9.68):** `LeastSquaresAddTerms` has 6 arguments; all its callers are in campaign
  A's `distort-private.h` and `distort-linear-private.h`. **Question for the owner/campaign
  A:** a struct for its `(terms, rank, number_vectors)`-style arguments would touch those files.
- **log.c (8.81):** at the line limit (1304 code lines), and every public root that Recipe S
  could split off (the list family, `LogMagickEventList`, the handler setters) uses
  `log_cache`, `log_semaphore` or the event mask, as resource.c's do. Its two argument
  findings are `LogMagickEventList` (public, 6) and `TranslateLogFormat` (5).
- **geometry-parse.c (9.09):** clearing *Overall Code Complexity* needs about 18 extractions
  (lizard mean 5.19 over 36 functions).
- **cipher.c (8.81):** the duplication groups pair the `!MAGICKCORE_CIPHER_SUPPORT` stubs
  (not compiled here) and the per-block `PasskeyEncipherImage`/`PasskeyDecipherImage` twins
  (a merge would need timing).
- **magick.c (9.61):** `GetMagickInfo`'s complexity is in `MAGICKCORE_BUILD_MODULES` branches
  and the signal handlers', so only one side of each is compiled here.
- **memory-quantum.c (9.63):** `CopyMagickMemory` is a hot `memcpy` path that no case reaches.
- **random.c (9.45), configure-paths.c (9.18):** the Large and Complex Methods are platform
  entropy and path branches (`/dev/urandom`, Windows, `!MAGICKCORE_INSTALLED_SUPPORT`).
- **identify.c (9.38), option.c (9.38):** line and declaration limits (option.c's are its
  option tables).
